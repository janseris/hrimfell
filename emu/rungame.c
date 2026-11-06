/* (v0.4.75) emu/run_game.py in C: the renderer core (the phone's GCC 2.9 -O2 build, emu/build_game.sh) run in Unicorn,
   its ARM instructions counted a frame - the same scenes, numbers, profile and pictures as the Python driver, with the
   per-block counting (and the data cache's per-read model) in C instead of a Python call each time.
     rungame [prof]          the environment as run_game.py's: SC, MODE, VIEW, SKIP, TEXMIP, ITEM, PROFFN, CACHE,
                             CACHEMAP, MISSCYC
   Build: cc -O2 -o emu/rungame emu/rungame.c -I/home/claude/unicorn/include -L/home/claude/unicorn/build -lunicorn -lz
          -lpthread -lm  (run with LD_LIBRARY_PATH=/home/claude/unicorn/build) - emu/build_rungame.sh */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include <unicorn/unicorn.h>

#define OUT "/home/claude/game/emu/gout"
#define DATA "/home/claude/game/data/"
enum { CODE = 0x10000, STOP = 0x00F00000, MEM = 0x01000000, MEMSZ = 0x01800000, FB = 0x05000000, LAND = 0x06000000,
       STACK = 0x04000000, STACKSZ = 0x00100000 };

static uc_engine* uc;
/* the symbols (core.sym), and the functions among them sorted by address for the profile */
typedef struct { uint32_t a; char* n; } Sym;
static Sym* syms; static int nsyms;
static Sym* fns; static int nfns;
static uint64_t count;
static int prof;
static uint64_t* fprof; static uint64_t* fcalls;            /* per function: instructions, entries */
static const char* proffn; static int proffi = -1;
static uint32_t* bad; static uint64_t* bcnt; static int nb, cb;    /* PROFFN's blocks */
/* the data cache: 256 sets of 2 ways, 16-byte lines */
static int64_t ctag[256][2]; static int clru[256]; static uint64_t miss, reads;
static int cachemap; static uint64_t* mpage; static int nmpage; static uint64_t* fmiss;   /* misses by 16 KB page (index = page) */

static int cmpfn(const void* x, const void* y)
    {
    const Sym* a = x; const Sym* b = y;
    if (a->a != b->a) return a->a < b->a ? -1 : 1;
    return strcmp(a->n, b->n);
    }
static void* rdfile(const char* fn, long* len)
    {
    FILE* f = fopen(fn, "rb"); *len = 0;
    if (!f) return calloc(1, 1);
    fseek(f, 0, SEEK_END); *len = ftell(f); fseek(f, 0, SEEK_SET);
    void* p = malloc(*len + 1); if (fread(p, 1, *len, f) != (size_t)*len) { fprintf(stderr, "short read %s\n", fn); exit(1); }
    fclose(f); return p;
    }
static uint32_t sym(const char* name)
    {
    int i;
    for (i = 0; i < nsyms; i++)
        if (!strcmp(syms[i].n, name) || (syms[i].n[0] == '_' && !strcmp(syms[i].n + 1, name))) return syms[i].a;
    fprintf(stderr, "no symbol %s\n", name); exit(1);
    }
/* which function holds addr: the last one starting at or before it (bisect_right - 1) */
static int fnof(uint32_t addr)
    {
    int lo = 0, hi = nfns;
    while (lo < hi) { int mid = (lo + hi) >> 1; if (fns[mid].a <= addr) lo = mid + 1; else hi = mid; }
    return lo - 1;
    }
static void blk(uc_engine* u, uint64_t addr, uint32_t size, void* ud)
    {
    count += size / 4;
    if (!prof) return;
    int i = fnof((uint32_t)addr);
    if (i < 0) return;
    fprof[i] += size / 4;
    if (addr == fns[i].a) fcalls[i]++;
    if (i == proffi)
        {
        int k;
        for (k = 0; k < nb; k++) if (bad[k] == (uint32_t)addr) break;
        if (k == nb)
            {
            if (nb == cb) { cb = cb ? cb * 2 : 256; bad = realloc(bad, cb * 4); bcnt = realloc(bcnt, cb * 8); }
            bad[nb] = (uint32_t)addr; bcnt[nb] = 0; nb++;
            }
        bcnt[k] += size / 4;
        }
    }
static void rd(uc_engine* u, uc_mem_type t, uint64_t addr, int size, int64_t val, void* ud)
    {
    reads++;
    uint64_t line = addr >> 4; int st = line & 255; int64_t tg = (int64_t)(line >> 8);
    if (ctag[st][0] == tg) { clru[st] = 1; return; }
    if (ctag[st][1] == tg) { clru[st] = 0; return; }
    miss++;
    if (cachemap)
        {
        uint64_t pg = addr >> 14; if (pg < (uint64_t)nmpage) mpage[pg]++;
        uint32_t pc; uc_reg_read(u, UC_ARM_REG_PC, &pc); int f = fnof(pc); if (f >= 0) fmiss[f]++;      /* (v0.4.75) who missed */
        }
    int v = clru[st]; ctag[st][v] = tg; clru[st] = 1 - v;
    }
/* (v0.4.83) the writes: a line not in the cache isn't brought in (no write-allocate) - the store goes out through the
   write buffer (~63 MB/s on the phone); counted, with their bytes */
static uint64_t writes, wmiss, wbytes; static uint64_t* fwb;    /* (CACHEMAP) bytes by function */
static void wr(uc_engine* u, uc_mem_type t, uint64_t addr, int size, int64_t val, void* ud)
    {
    writes++;
    uint64_t line = addr >> 4; int st = line & 255; int64_t tg = (int64_t)(line >> 8);
    if (ctag[st][0] == tg || ctag[st][1] == tg) return;
    wmiss++; wbytes += size;
    if (fwb) { uint32_t pc; uc_reg_read(u, UC_ARM_REG_PC, &pc); int f = fnof(pc); if (f >= 0) fwb[f] += size; }
    }
/* (ALIGN=all|ui: emu/check_align_*.py) the reads and writes of 2 or 4 bytes not on their own alignment - which the
   9300's ARM9 does not do as the PC does: where (function, pc, size) and how often */
typedef struct { int f; uint32_t pc; int size; uint64_t n; } Hit;
static Hit hits[256]; static int nhits;
static void alignhk(uc_engine* u, uc_mem_type t, uint64_t addr, int size, int64_t val, void* ud)
    {
    if (size <= 1 || !(addr & (size - 1))) return;
    uint32_t pc; uc_reg_read(u, UC_ARM_REG_PC, &pc);
    int f = fnof(pc), k;
    for (k = 0; k < nhits; k++) if (hits[k].pc == pc && hits[k].size == size) { hits[k].n++; return; }
    if (nhits < 256) { hits[nhits].f = f; hits[nhits].pc = pc; hits[nhits].size = size; hits[nhits].n = 1; nhits++; }
    }
static void puthits(void)
    {
    int k;
    if (!nhits) { printf(" {}\n"); return; }
    for (k = 0; k < nhits; k++) printf("%s %s %x size %d: %llu", k ? "," : "", hits[k].f >= 0 ? fns[hits[k].f].n : "?", hits[k].pc, hits[k].size, (unsigned long long)hits[k].n);
    printf("\n");
    }
static uint32_t call(const char* name, int n, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
    {
    uint32_t r[4] = { a0, a1, a2, a3 }, sp = STACK + STACKSZ - 0x100, lr = STOP, r0;
    static const int KR[4] = { UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3 };
    int i;
    for (i = 0; i < n; i++) uc_reg_write(uc, KR[i], &r[i]);
    uc_reg_write(uc, UC_ARM_REG_SP, &sp); uc_reg_write(uc, UC_ARM_REG_LR, &lr);
    count = 0;
    uc_err e = uc_emu_start(uc, sym(name), STOP, 0, 0);
    if (e) { fprintf(stderr, "%s: %s\n", name, uc_strerror(e)); exit(1); }
    uc_reg_read(uc, UC_ARM_REG_R0, &r0);
    return r0;
    }
/* a PNG of the 640 x 200 RGB565 frame (as PIL saved it: 8-bit RGB, the low bits zero) */
static void png_chunk(FILE* f, const char* t, const unsigned char* d, uint32_t n)
    {
    unsigned char h[8] = { n >> 24, n >> 16, n >> 8, n, t[0], t[1], t[2], t[3] };
    fwrite(h, 1, 8, f); if (n) fwrite(d, 1, n, f);
    uint32_t c = crc32(0, h + 4, 4); c = crc32(c, d, n);
    unsigned char cc[4] = { c >> 24, c >> 16, c >> 8, c }; fwrite(cc, 1, 4, f);
    }
static void save_png(const char* fn, const uint16_t* px)
    {
    static unsigned char raw[200 * (1 + 640 * 3)];
    int x, y;
    for (y = 0; y < 200; y++)
        {
        unsigned char* r = raw + y * (1 + 640 * 3); *r++ = 0;
        for (x = 0; x < 640; x++) { uint16_t p = px[y * 640 + x]; *r++ = (p >> 8) & 0xF8; *r++ = (p >> 3) & 0xFC; *r++ = (p << 3) & 0xF8; }
        }
    uLongf zn = compressBound(sizeof raw); unsigned char* z = malloc(zn);
    compress2(z, &zn, raw, sizeof raw, 6);
    FILE* f = fopen(fn, "wb"); if (!f) { perror(fn); exit(1); }
    static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    unsigned char ihdr[13] = { 0, 0, 640 >> 8, 640 & 255, 0, 0, 0, 200, 8, 2, 0, 0, 0 };
    fwrite(sig, 1, 8, f); png_chunk(f, "IHDR", ihdr, 13); png_chunk(f, "IDAT", z, (uint32_t)zn); png_chunk(f, "IEND", 0, 0);
    fclose(f); free(z);
    }
static const char* const KNames[] = { "village", "wolves/forest", "word stone", "dragon", "inventory", "city market",
    "city approach", "tree rise", "stables inside", "stables yard", "rain 3rd pers", "hall of dead", "catacombs", "apothecary",
    "inn", "highhall", "barrow outside", "barrow entry", "spider lair", "word wall", "sunlit cavern", "rw trader",
    "rw trader up", "sleeping giant", "rw by the inn", "gerdur house", "sven house", "alvor forge out", "trader out",
    "rw bridge", "mill wheel", "faendal chops", "rift forest", "rift forest e", "fh stronghold", "fh crypt", "fh refectory",
    "city at 250m", "city at 400m", "city at 600m", "plains away" };
static int byprof(const void* x, const void* y)
    {
    uint64_t a = fprof[*(const int*)x], b = fprof[*(const int*)y];
    return a < b ? 1 : a > b ? -1 : 0;
    }
static uint64_t* sortkey;
static int bykey(const void* x, const void* y)
    {
    uint64_t a = sortkey[*(const int*)x], b = sortkey[*(const int*)y];
    return a < b ? 1 : a > b ? -1 : 0;
    }
static uint32_t align64(uint32_t v) { return (v + 63) & ~63u; }
int main(int argc, char** argv)
    {
    prof = argc > 1 && !strcmp(argv[1], "prof");
    proffn = getenv("PROFFN"); const char* cache = getenv("CACHE"); cachemap = getenv("CACHEMAP") != 0;
    int misscyc = getenv("MISSCYC") ? atoi(getenv("MISSCYC")) : 24;
    /* the symbols */
    { FILE* f = fopen(OUT "/core.sym", "r"); char l[512]; int cap = 4096; syms = malloc(cap * sizeof(Sym));
      if (!f) { fprintf(stderr, "no " OUT "/core.sym (emu/build_game.sh)\n"); return 1; }
      while (fgets(l, sizeof l, f))
          {
          char a[64], t[16], n[400];
          if (sscanf(l, "%63s %15s %399s", a, t, n) != 3) continue;
          if (nsyms == cap) { cap *= 2; syms = realloc(syms, cap * sizeof(Sym)); }
          syms[nsyms].a = (uint32_t)strtoul(a, 0, 16); syms[nsyms].n = strdup(n); nsyms++;
          }
      fclose(f); }
    { int i; fns = malloc(nsyms * sizeof(Sym));
      for (i = 0; i < nsyms; i++)
          {
          const char* n = syms[i].n;
          if (syms[i].a < CODE || syms[i].a >= CODE + 0x100000 || n[0] == '.' || (n[0] == 'L' && n[1] >= '0' && n[1] <= '9')) continue;
          fns[nfns++] = syms[i];
          }
      qsort(fns, nfns, sizeof(Sym), cmpfn);
      fprof = calloc(nfns, 8); fcalls = calloc(nfns, 8);
      if (proffn) for (i = 0; i < nfns; i++) if (!strcmp(fns[i].n, proffn)) proffi = i; }
    if (uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc)) { fprintf(stderr, "uc_open\n"); return 1; }
    /* the program (a PE image) */
    { long n; unsigned char* exe = rdfile(OUT "/core.exe", &n);
      uint32_t pe = *(uint32_t*)(exe + 0x3c); int nsec = *(uint16_t*)(exe + pe + 6), opt = *(uint16_t*)(exe + pe + 20);
      uint32_t base = *(uint32_t*)(exe + pe + 24 + 28); int i;
      uc_mem_map(uc, CODE, 0x400000, UC_PROT_ALL);
      for (i = 0; i < nsec; i++)
          {
          unsigned char* o = exe + pe + 24 + opt + 40 * i;
          uint32_t vsize = *(uint32_t*)(o + 8), va = *(uint32_t*)(o + 12), rawsize = *(uint32_t*)(o + 16), rawptr = *(uint32_t*)(o + 20);
          if (rawsize) uc_mem_write(uc, base + va, exe + rawptr, vsize && vsize < rawsize ? vsize : rawsize);
          }
      free(exe); }
    uc_mem_map(uc, STOP, 0x1000, UC_PROT_ALL); uc_mem_map(uc, MEM, MEMSZ, UC_PROT_ALL); uc_mem_map(uc, FB, 0x40000, UC_PROT_ALL);
    uc_mem_map(uc, STACK, STACKSZ, UC_PROT_ALL);
    /* the data files, as run_game.py lays them out */
    { long nl, na, ni, nbf, nlib, nwr, nfh;
      void* land = rdfile(DATA "hrimfell.land", &nl); void* area = rdfile(DATA "riverwood.har", &na); void* items = rdfile(DATA "items.hit", &ni);
      void* bfb = rdfile(DATA "bleakfalls.har", &nbf); void* lib = rdfile(DATA "common.hlb", &nlib); void* wr = rdfile(DATA "whiterun.har", &nwr);
      void* fh = rdfile(DATA "forelhost.har", &nfh);
      uint32_t aat = align64(LAND + 64 + nl), iat = align64(aat + na), bat = align64(iat + ni), cat = align64(bat + nbf);
      uint32_t wat = align64(cat + nlib), fat = align64(wat + nwr);
      uc_mem_map(uc, LAND, ((fat - LAND + nfh + 0x10000) + 0xFFFF) & ~0xFFFFu, UC_PROT_ALL);
      uint32_t hdr[16] = { LAND + 64, nl, LAND + 64, nl, aat, na, iat, ni, bat, nbf, cat, nlib, wat, nwr, fat, nfh };
      uc_mem_write(uc, LAND, hdr, sizeof hdr);
      uc_mem_write(uc, bat, bfb, nbf); uc_mem_write(uc, cat, lib, nlib); uc_mem_write(uc, wat, wr, nwr); uc_mem_write(uc, fat, fh, nfh);
      uc_mem_write(uc, LAND + 64, land, nl); uc_mem_write(uc, aat, area, na); uc_mem_write(uc, iat, items, ni);
      free(land); free(area); free(items); free(bfb); free(lib); free(wr); free(fh); }
    if (cachemap) { nmpage = (int)(0x08000000 >> 14); mpage = calloc(nmpage, 8); fmiss = calloc(nfns, 8); fwb = calloc(nfns, 8); }
    { int i, j; for (i = 0; i < 256; i++) ctag[i][0] = ctag[i][1] = -1; (void)j; }
    const char* align = getenv("ALIGN");
    if (align)
        {
        /* (v0.4.75) emu/check_align_all.py / check_align_ui.py: every access watched (no block counting) */
        uc_hook ha; int k;
        if (!strcmp(align, "selftest"))
            {
            /* one misaligned word load (ldr r0, [r1] with r1 = MEM + 2): must be caught */
            uint32_t ins = 0xE5910000u, r1 = MEM + 2, at = 0x00E00000;
            uc_mem_map(uc, at, 0x1000, UC_PROT_ALL); uc_mem_write(uc, at, &ins, 4); uc_reg_write(uc, UC_ARM_REG_R1, &r1);
            uc_hook_add(uc, &ha, UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, (void*)alignhk, 0, 1, 0);
            uc_emu_start(uc, at, at + 4, 0, 0);
            printf("selftest"); puthits();
            printf(nhits == 1 && hits[0].size == 4 ? "caught: ok\n" : "NOT CAUGHT\n");
            return nhits != 1;
            }
        if (!strcmp(align, "ui"))
            {
            call("emu_init", 2, MEM, LAND, 0, 0);
            uc_hook_add(uc, &ha, UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, (void*)alignhk, 0, 1, 0);
            uc_ctl_remove_cache(uc, CODE, CODE + 0x400000);         /* (code translated before the hook: translated again, watched) */
            for (k = 0; k < 14; k++) { call("emu_interact", 3, MEM, FB, k, 0); printf("case %d", k); puthits(); fflush(stdout); }
            return nhits != 0;
            }
        uc_hook_add(uc, &ha, UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, (void*)alignhk, 0, 1, 0);
        for (k = 0; k < 6; k++) { call("emu_step", 3, MEM, k, LAND, 0); printf("step %d", k + 1); puthits(); fflush(stdout); }
        call("emu_title", 1, MEM, 0, 0, 0);
        for (k = 0; k < 2; k++) { call("emu_tick", 3, MEM, 66000, 0, 0); call("emu_render", 2, MEM, FB, 0, 0); }
        printf("title"); puthits();
        int last = (int)(sizeof KNames / sizeof KNames[0]);
        for (k = 0; k < last; k++)
            {
            int j;
            call("emu_scene", 2, MEM, k, 0, 0);
            for (j = 0; j < 2; j++) { call("emu_tick", 3, MEM, 66000, 0, 0); call("emu_render", 2, MEM, FB, 0, 0); }
            printf("scene %d", k); puthits(); fflush(stdout);
            }
        return nhits != 0;
        }
    uc_hook hb;
    uc_hook_add(uc, &hb, UC_HOOK_BLOCK, (void*)blk, 0, 1, 0);
    uint32_t need = call("emu_memneeded", 0, 0, 0, 0, 0);
    if (need >= MEMSZ) { fprintf(stderr, "memneeded %u\n", need); return 1; }
    { int pr = prof; prof = 0; call("emu_init", 2, MEM, LAND, 0, 0); prof = pr; }
    printf("init: %.1fM instructions, mem %u bytes\n", count / 1e6, need); fflush(stdout);
    int mode = getenv("MODE") ? atoi(getenv("MODE")) : 0;
    call("emu_mode", 2, MEM, mode, 0, 0);
    if (getenv("VIEW")) call("emu_view", 2, MEM, atoi(getenv("VIEW")), 0, 0);
    if (getenv("GAMMA")) call("emu_gamma", 2, MEM, atoi(getenv("GAMMA")), 0, 0);      /* (v0.4.83) the Brightness setting */
    if (getenv("OCC")) call("emu_occ", 2, MEM, atoi(getenv("OCC")), 0, 0);
    if (getenv("SKIP")) call("emu_skip", 2, MEM, atoi(getenv("SKIP")), 0, 0);
    if (getenv("TEXMIP")) call("emu_texmip", 2, MEM, atoi(getenv("TEXMIP")), 0, 0);
    if (getenv("ITEM")) call("emu_item", 2, MEM, atoi(getenv("ITEM")), 0, 0);
    if (getenv("BENCH"))
        {
        /* (v0.4.82) the menu's benchmark, a frame's time from its instructions (3 cycles each at 150 MHz) */
        int fr, r = 100000, bl = 5000, run = 1;
        call("emu_bench_start", 1, MEM, 0, 0, 0);
        for (fr = 0; fr < 40000 && run; fr++)
            {
            run = call("emu_bench_frame", 4, MEM, r, bl, r + bl);
            call("emu_tick", 3, MEM, r + bl, 0, 0);
            uint64_t c1 = count;
            call("emu_render", 2, MEM, FB, 0, 0);
            r = (int)((c1 + count) / 50);
            }
        uint32_t la = call("emu_bench_log", 2, MEM, 0, 0, 0), ln = call("emu_bench_log", 2, MEM, 1, 0, 0);
        static char lg[20000]; if (ln > sizeof lg - 1) ln = sizeof lg - 1;
        uc_mem_read(uc, la, lg, ln); lg[ln] = 0;
        printf("%s(%d frames)\n", lg, fr);
        return 0;
        }
    int sc[256], nsc = 0;
    if (getenv("SC")) { char* s = strdup(getenv("SC")); char* t; for (t = strtok(s, ","); t && nsc < 256; t = strtok(0, ",")) sc[nsc++] = atoi(t); }
    else for (nsc = 0; nsc < 11; nsc++) sc[nsc] = nsc;
    static uint16_t px[640 * 200];
    int si;
    for (si = 0; si < nsc; si++)
        {
        int s = sc[si], step, n = 0, cn = 0; uint64_t tot = 0, cm = 0;
        { uint64_t c0 = count; call("emu_scene", 2, MEM, s, 0, 0); if (getenv("LOADI")) printf("scene %d set-up: %.1fM instructions\n", s, (count - c0) / 1e6); }
        if (prof) { memset(fprof, 0, nfns * 8); memset(fcalls, 0, nfns * 8); }
        for (step = 0; step < 12; step++)
            {
            if (prof && step == 4) { memset(fprof, 0, nfns * 8); memset(fcalls, 0, nfns * 8); nb = 0; }
            uc_hook hk = 0, hw = 0; int on = 0;
            if (cache && step >= 10)
                {
                miss = 0; reads = 0;
                if (cachemap) { memset(mpage, 0, nmpage * 8); memset(fmiss, 0, nfns * 8); }
                uc_hook_add(uc, &hk, UC_HOOK_MEM_READ, (void*)rd, 0, 1, 0); on = 1;
                writes = wmiss = wbytes = 0; if (fwb) memset(fwb, 0, nfns * 8);
                uc_hook_add(uc, &hw, UC_HOOK_MEM_WRITE, (void*)wr, 0, 1, 0);
                uc_ctl_remove_cache(uc, CODE, CODE + 0x400000);
                }
            call("emu_tick", 3, MEM, 66000, 0, 0);
            call("emu_render", 2, MEM, FB, 0, 0);
            if (on) { uc_hook_del(uc, hk); uc_hook_del(uc, hw); uc_ctl_remove_cache(uc, CODE, CODE + 0x400000); cm += miss; cn++; }
            if (step >= 4) { tot += count; n++; }
            }
        uint32_t tris = call("emu_tris", 1, MEM, 0, 0, 0);
        uint64_t ins = tot / n;
        printf("%-14s %7.2fM instr/frame  tris %4u  -> est. %4.1f fps on 9300 (3 cyc/instr + 5 ms copy)\n",
               s < (int)(sizeof KNames / sizeof KNames[0]) ? KNames[s] : "?", ins / 1e6, tris, 1e3 / (ins * 3.0 / 150e3 + 5));
        if (cachemap)
            {
            uint64_t t = 0; int i, k, ord[12], no = 0;
            for (i = 0; i < nmpage; i++) t += mpage[i];
            for (k = 0; k < 12; k++)
                {
                int best = -1;
                for (i = 0; i < nmpage; i++) { int u, used = 0; if (!mpage[i]) continue; for (u = 0; u < no; u++) if (ord[u] == i) used = 1; if (!used && (best < 0 || mpage[i] > mpage[best])) best = i; }
                if (best < 0) break; ord[no++] = best;
                }
            printf("   misses by 16 KB page (last frame): ");
            for (k = 0; k < no; k++) printf("%s%x %d%%", k ? ", " : "", ord[k] << 14, (int)(100 * mpage[ord[k]] / (t ? t : 1)));
            printf("\n   zbuffer at %x, area at %x\n", call("emu_addr", 2, MEM, 0, 0, 0), call("emu_addr", 2, MEM, 1, 0, 0));
            /* (v0.4.75) the hot pages named by the buffer they are in, and who missed */
            {
            static const char* const KB[] = { "zbuffer", "colour buffer", "engine pictures", "area pictures", "library pictures",
                "reciprocals", "objects", "shade palettes", "area shades", "library shades", "R3D", "Game", "transformed vertices", "interior objects", "vertex light", "face list", "sort keys", "lazy list", "area memory" };
            uint32_t ba[19], bs[19]; int b, q;
            uint32_t sz = STACK + STACKSZ - 0x200;      /* (an argument slot for the size, in the stack's spare top) */
            for (b = 0; b < 19; b++) { ba[b] = call("emu_buf", 3, MEM, b, sz, 0); uc_mem_read(uc, sz, &bs[b], 4); }
            if (getenv("BUFLIST")) for (b = 0; b < 19; b++) printf("   buffer %-22s %08x size %u\n", KB[b], ba[b], bs[b]);
            printf("   hot pages: ");
            for (k = 0; k < no; k++)
                {
                uint32_t a = ord[k] << 14; const char* nm = (a >= STACK && a < STACK + STACKSZ) ? "stack" : "?";
                for (q = 0; q <= 18; q++) if (ba[q] && a + 0x4000 > ba[q] && a < ba[q] + bs[q]) { nm = KB[q]; break; }
                printf("%s%x %s", k ? ", " : "", a, nm);
                }
            /* the misses by buffer (all pages, not only the hot ones) */
            { uint64_t tb[20] = { 0 }, tt = 0; int pg;
              for (pg = 0; pg < nmpage; pg++)
                  {
                  if (!mpage[pg]) continue;
                  uint32_t a = (uint32_t)pg << 14; int w = 19; tt += mpage[pg];
                  if (a >= STACK && a < STACK + STACKSZ) w = 18;
                  else for (q = 0; q < 18; q++) if (ba[q] && a + 0x4000 > ba[q] && a < ba[q] + bs[q]) { w = q; break; }
                  tb[w] += mpage[pg];
                  }
              printf("\n   misses by buffer: ");
              for (q = 0; q < 20; q++) if (tb[q] * 100 >= tt) printf("%s %d%%, ", q == 19 ? "other" : q == 18 ? "stack" : KB[q], (int)(100 * tb[q] / tt));
            }
            printf("\n   misses by function: ");
            { int i, m = 0, *fo = malloc(nfns * sizeof(int)); uint64_t tf = 0;
              for (i = 0; i < nfns; i++) { tf += fmiss[i]; if (fmiss[i]) fo[m++] = i; }
              sortkey = fmiss; qsort(fo, m, sizeof(int), bykey);
              for (i = 0; i < m && i < 12; i++) printf("%s%s %d%%", i ? ", " : "", fns[fo[i]].n, (int)(100 * fmiss[fo[i]] / (tf ? tf : 1)));
              printf("\n"); free(fo); }
            }
            }
        if (cache)
            {
            uint64_t mf = cm / (cn ? cn : 1);
            printf("               data cache: %llu misses a frame (%.1f%% of %llu reads) -> est. %4.1f fps with them (%d cycles each, 1.5 cyc/instr)\n",
                   (unsigned long long)mf, 100.0 * mf / (reads ? reads : 1), (unsigned long long)reads, 1e3 / ((ins * 1.5 + mf * misscyc) / 150e3 + 5), misscyc);
            printf("               writes (last frame): %llu, %llu past the cache = %llu KB (%.1f ms at 63 MB/s)\n",
                   (unsigned long long)writes, (unsigned long long)wmiss, (unsigned long long)(wbytes >> 10), wbytes / 63e3);
            if (fwb)
                {
                int i, m = 0; static int fo[65536];
                for (i = 0; i < nfns; i++) if (fwb[i]) fo[m++] = i;
                sortkey = fwb; qsort(fo, m, sizeof(int), bykey);
                printf("               written past the cache, by function: ");
                for (i = 0; i < m && i < 10; i++) printf("%s%s %lluK", i ? ", " : "", fns[fo[i]].n, (unsigned long long)(fwb[fo[i]] >> 10));
                printf("\n");
                }
            }
        uc_mem_read(uc, FB, px, sizeof px);
        { char fn[256]; snprintf(fn, sizeof fn, OUT "/scene%d.png", s); save_png(fn, px); }
        if (prof)
            {
            uint64_t t = 0; int i, *ord = malloc(nfns * sizeof(int)), m = 0;
            for (i = 0; i < nfns; i++) { t += fprof[i]; if (fprof[i]) ord[m++] = i; }
            qsort(ord, m, sizeof(int), byprof);
            for (i = 0; i < m && i < 25; i++)
                {
                int f = ord[i]; uint64_t k = fcalls[f] / n, c = fprof[f];
                printf("      %5.1f%%  %-44s %6llu calls/frame %6llu instr/call\n", 100.0 * c / t, fns[f].n, (unsigned long long)k,
                       (unsigned long long)((c / n) / (k ? k : 1)));
                }
            if (proffn && nb)
                {
                int* bo = malloc(nb * sizeof(int));
                for (i = 0; i < nb; i++) bo[i] = i;
                sortkey = bcnt; qsort(bo, nb, sizeof(int), bykey);
                for (i = 0; i < nb && i < 15; i++) printf("        block %x: %llu\n", bad[bo[i]], (unsigned long long)(bcnt[bo[i]] / 8));
                free(bo); nb = 0;
                }
            free(ord);
            }
        fflush(stdout);
        }
    return 0;
    }

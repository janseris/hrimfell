#!/usr/bin/env python3
"""Runs the renderer core (phone GCC 2.9 -O2 build) in Unicorn and counts ARM instructions per frame.
   run.py [prof]   -> per mode: instructions/frame (+ per-function profile with 'prof')"""
import os, re, struct, sys, bisect
sys.path.insert(0, '/home/claude/unicorn/bindings/python')
os.environ.setdefault('LIBUNICORN_PATH', '/home/claude/unicorn/build')
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_BLOCK, UC_HOOK_MEM_READ
from unicorn.arm_const import *
OUT = '/home/claude/game/emu/gout'
CODE, STOP = 0x10000, 0x00F00000
MEM, MEMSZ = 0x01000000, 0x01800000
FB = 0x05000000
LAND = 0x06000000
STACK, STACKSZ = 0x04000000, 0x00100000
PROFFN = os.environ.get('PROFFN')
CACHE = os.environ.get('CACHE'); CACHEMAP = os.environ.get('CACHEMAP'); MISSCYC = int(os.environ.get('MISSCYC', '24'))

class Emu:
    def __init__(self, prof=False):
        self.syms = {}
        for line in open(OUT + '/core.sym'):
            p = line.split()
            if len(p) == 3: self.syms[p[2]] = int(p[0], 16)
        mu = self.mu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        exe = open(OUT + '/core.exe', 'rb').read()
        pe = struct.unpack_from('<I', exe, 0x3c)[0]
        nsec = struct.unpack_from('<H', exe, pe + 6)[0]
        opt = struct.unpack_from('<H', exe, pe + 20)[0]
        base = struct.unpack_from('<I', exe, pe + 24 + 28)[0]
        mu.mem_map(CODE, 0x400000)
        for i in range(nsec):
            o = pe + 24 + opt + 40 * i
            vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', exe, o + 8)
            if rawsize: mu.mem_write(base + va, exe[rawptr:rawptr + min(rawsize, vsize or rawsize)])
        mu.mem_map(STOP, 0x1000); mu.mem_map(MEM, MEMSZ); mu.mem_map(FB, 0x40000); mu.mem_map(STACK, STACKSZ)
        land = open('/home/claude/game/data/hrimfell.land', 'rb').read() if os.path.exists('/home/claude/game/data/hrimfell.land') else b''
        area = open('/home/claude/game/data/riverwood.har', 'rb').read() if os.path.exists('/home/claude/game/data/riverwood.har') else b''
        items = open('/home/claude/game/data/items.hit', 'rb').read() if os.path.exists('/home/claude/game/data/items.hit') else b''
        rd = lambda n: open('/home/claude/game/data/' + n, 'rb').read() if os.path.exists('/home/claude/game/data/' + n) else b''
        bfb = rd('bleakfalls.har'); lib = rd('common.hlb'); wr = rd('whiterun.har'); fh = rd('forelhost.har')
        aat = (LAND + 64 + len(land) + 63) & ~63
        iat = (aat + len(area) + 63) & ~63
        bat = (iat + len(items) + 63) & ~63
        cat = (bat + len(bfb) + 63) & ~63
        wat = (cat + len(lib) + 63) & ~63
        fat = (wat + len(wr) + 63) & ~63
        mu.mem_map(LAND, (fat - LAND + len(fh) + 0x10000 + 0xFFFF) & ~0xFFFF)
        mu.mem_write(LAND, struct.pack('<16I', LAND + 64, len(land), LAND + 64, len(land), aat, len(area), iat, len(items), bat, len(bfb), cat, len(lib), wat, len(wr), fat, len(fh)))
        mu.mem_write(bat, bfb); mu.mem_write(cat, lib); mu.mem_write(wat, wr); mu.mem_write(fat, fh)
        mu.mem_write(LAND + 64, land)
        mu.mem_write(aat, area)
        mu.mem_write(iat, items)
        fn = sorted((v, k) for k, v in self.syms.items() if CODE <= v < CODE + 0x100000 and not k.startswith('.') and not re.match(r'L[0-9]', k))
        self.addrs = [a for a, _ in fn]; self.names = [n for _, n in fn]
        self.prof = {} if prof else None
        self.ctag = [[-1, -1] for _ in range(256)]; self.clru = [0] * 256; self.miss = 0; self.reads = 0; self.mpage = {}
        self.blocks = {}
        self.calls = {}
        self.count = 0
        mu.hook_add(UC_HOOK_BLOCK, self._blk)
    def _rd(self, mu, acc, addr, size, val, ud):
        self.reads += 1
        line = addr >> 4; st = line & 255; tg = line >> 8
        w = self.ctag[st]
        if w[0] == tg: self.clru[st] = 1; return
        if w[1] == tg: self.clru[st] = 0; return
        self.miss += 1
        if CACHEMAP: self.mpage[addr >> 14] = self.mpage.get(addr >> 14, 0) + 1
        v = self.clru[st]; w[v] = tg; self.clru[st] = 1 - v
    def _blk(self, mu, addr, size, ud):
        self.count += size // 4
        if self.prof is not None:
            i = bisect.bisect_right(self.addrs, addr) - 1
            n = self.names[i] if i >= 0 else '?'
            self.prof[n] = self.prof.get(n, 0) + size // 4
            if addr == self.addrs[i]: self.calls[n] = self.calls.get(n, 0) + 1     # (a function entered: its calls)
            if PROFFN and n == PROFFN: self.blocks[addr] = self.blocks.get(addr, 0) + size // 4
    def sym(self, name):
        for k, v in self.syms.items():
            if k == name or k == '_' + name: return v
        raise KeyError(name)
    def call(self, name, *args):
        mu = self.mu
        for r, v in zip([UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3], args): mu.reg_write(r, v & 0xffffffff)
        mu.reg_write(UC_ARM_REG_SP, STACK + STACKSZ - 0x100); mu.reg_write(UC_ARM_REG_LR, STOP)
        self.count = 0
        mu.emu_start(self.sym(name), STOP)
        return mu.reg_read(UC_ARM_REG_R0)

def main():
    prof = len(sys.argv) > 1 and sys.argv[1] == 'prof'
    e = Emu(prof)
    need = e.call('emu_memneeded'); assert need < MEMSZ, need
    e.call('emu_init', MEM, LAND)
    print('init: %.1fM instructions, mem %d bytes' % (e.count / 1e6, need))
    names = ['village', 'wolves/forest', 'word stone', 'dragon', 'inventory', 'city market', 'city approach', 'tree rise', 'stables inside', 'stables yard', 'rain 3rd pers', 'hall of dead', 'catacombs', 'apothecary', 'inn', 'highhall', 'barrow outside', 'barrow entry', 'spider lair', 'word wall', 'sunlit cavern', 'rw trader', 'rw trader up', 'sleeping giant', 'rw by the inn', 'gerdur house', 'sven house', 'alvor forge out', 'trader out', 'rw bridge', 'mill wheel', 'faendal chops', 'rift forest', 'rift forest e', 'fh stronghold', 'fh crypt', 'fh refectory', 'city at 250m', 'city at 400m', 'city at 600m', 'plains away']
    import struct
    from PIL import Image
    for mode in [int(os.environ.get('MODE', '0'))]:
        e.call('emu_mode', MEM, mode)
        if os.environ.get('VIEW'): e.call('emu_view', MEM, int(os.environ['VIEW']))
        if os.environ.get('SKIP'): e.call('emu_skip', MEM, int(os.environ['SKIP']))
        if os.environ.get('TEXMIP'): e.call('emu_texmip', MEM, int(os.environ['TEXMIP']))
        if os.environ.get('ITEM'): e.call('emu_item', MEM, int(os.environ['ITEM']))
        for sc in ([int(x) for x in os.environ['SC'].split(',')] if os.environ.get('SC') else range(11)):
            e.call('emu_scene', MEM, sc)
            tot = 0; n = 0
            if prof: e.prof = {}
            cm = 0; cn = 0
            for step in range(12):
                if prof and step == 4: e.prof = {}; e.blocks = {}; e.calls = {}          # (the frames measured only)
                # CACHE=1: the 9300's data cache (ARM925T: 8 KB, 2 ways of 256 lines of 16 bytes) on the last two
                # frames' reads - the misses, each a line read from memory (MISSCYC cycles, 24 by default)
                hk = None
                if CACHE and step >= 10:
                    e.miss = 0; e.reads = 0
                    hk = e.mu.hook_add(UC_HOOK_MEM_READ, e._rd)
                    e.mu.ctl_remove_cache(CODE, CODE + 0x400000)                     # (code translated before the hook: translated again, watched)
                e.call('emu_tick', MEM, 66000, 0)
                e.call('emu_render', MEM, FB)
                if hk is not None: e.mu.hook_del(hk); e.mu.ctl_remove_cache(CODE, CODE + 0x400000); cm += e.miss; cn += 1
                if step >= 4: tot += e.count; n += 1
            tris = e.call('emu_tris', MEM)
            ins = tot // n
            print('%-14s %7.2fM instr/frame  tris %4d  -> est. %4.1f fps on 9300 (2.5 cyc/instr + 5 ms copy)' % (names[sc], ins / 1e6, tris, 1e3 / (ins * 2.5 / 150e3 + 5)))
            if CACHEMAP:
                tot_ = sum(e.mpage.values())
                print('   misses by 16 KB page (last frame): ' + ', '.join('%x %d%%' % (pg << 14, 100 * c // tot_) for pg, c in sorted(e.mpage.items(), key=lambda x: -x[1])[:12]))
                print('   zbuffer at %x, area at %x' % (e.call('emu_addr', MEM, 0), e.call('emu_addr', MEM, 1)))
            if CACHE:
                mf = cm // max(1, cn)
                print('               data cache: %d misses a frame (%.1f%% of %d reads) -> est. %4.1f fps with them (%d cycles each, 1.5 cyc/instr)' % (
                    mf, 100.0 * mf / max(1, e.reads), e.reads, 1e3 / ((ins * 1.5 + mf * MISSCYC) / 150e3 + 5), MISSCYC))
            d = bytes(e.mu.mem_read(FB, 640 * 200 * 2))
            im = Image.new('RGB', (640, 200)); im.putdata([((p >> 8) & 0xF8, (p >> 3) & 0xFC, (p << 3) & 0xF8) for p in struct.unpack('<128000H', d)])
            im.save(OUT + '/scene%d.png' % sc)
            if prof:
                t = sum(e.prof.values())
                for name, c in sorted(e.prof.items(), key=lambda x: -x[1])[:25]:
                    k = e.calls.get(name, 0) // max(1, n)
                    print('      %5.1f%%  %-44s %6d calls/frame %6d instr/call' % (100.0 * c / t, name, k, (c // max(1, n)) // max(1, k)))
                if PROFFN:
                    for a, c in sorted(e.blocks.items(), key=lambda x: -x[1])[:15]: print('        block %x: %d' % (a, c // 8))
                    e.blocks = {}

if __name__ == '__main__': main()

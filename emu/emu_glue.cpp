// glue for running the renderer core (built by the phone's GCC 2.9 -O2) inside Unicorn
#include "r3d.h"
extern "C" void* memcpy(void* d, const void* s, unsigned int n)
    { u8* a = (u8*)d; const u8* b = (const u8*)s; while (n--) *a++ = *b++; return d; }
extern "C" void* memset(void* d, int v, unsigned int n) { r3d_memset(d, v, (int)n); return d; }
extern "C" int emu_memneeded() { return R3D::MemNeeded() + sizeof(R3D) + 64; }
extern "C" void emu_init(u8* mem) { R3D* r = (R3D*)mem; r->Init(mem + ((sizeof(R3D) + 63) & ~63)); }
extern "C" void emu_tick(u8* mem, int dt, int keys) { ((R3D*)mem)->Tick(dt, keys); }
extern "C" void emu_setmode(u8* mem, int m) { R3D* r = (R3D*)mem; r->StartBenchmark(); r->iBenchRun = m; r->iMode = m; }
extern "C" void emu_render(u8* mem, u16* out) { ((R3D*)mem)->Render(out, 640); }
extern "C" void emu_native(u8* mem, int on) { ((R3D*)mem)->SetNative(on); }
extern "C" int emu_tris(u8* mem) { return ((R3D*)mem)->iTris; }
extern "C" int emu_pixels(u8* mem) { return ((R3D*)mem)->iPixels; }

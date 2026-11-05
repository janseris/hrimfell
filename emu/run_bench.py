#!/usr/bin/env python3
"""Runs the renderer core (phone GCC 2.9 -O2 build) in Unicorn and counts ARM instructions per frame.
   run.py [prof]   -> per mode: instructions/frame (+ per-function profile with 'prof')"""
import os, re, struct, sys, bisect
sys.path.insert(0, '/home/claude/unicorn/bindings/python')
os.environ.setdefault('LIBUNICORN_PATH', '/home/claude/unicorn/build')
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_BLOCK
from unicorn.arm_const import *
OUT = '/home/claude/bench/emu/out'
CODE, STOP = 0x10000, 0x000F0000
MEM, MEMSZ = 0x01000000, 0x01800000
FB = 0x03000000
STACK, STACKSZ = 0x04000000, 0x00100000

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
        mu.mem_map(CODE, 0x80000)
        for i in range(nsec):
            o = pe + 24 + opt + 40 * i
            vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', exe, o + 8)
            if rawsize: mu.mem_write(base + va, exe[rawptr:rawptr + min(rawsize, vsize or rawsize)])
        mu.mem_map(STOP, 0x1000); mu.mem_map(MEM, MEMSZ); mu.mem_map(FB, 0x40000); mu.mem_map(STACK, STACKSZ)
        fn = sorted((v, k) for k, v in self.syms.items() if CODE <= v < CODE + 0x80000 and not k.startswith('.') and not re.match(r'L[0-9]', k))
        self.addrs = [a for a, _ in fn]; self.names = [n for _, n in fn]
        self.prof = {} if prof else None
        self.count = 0
        mu.hook_add(UC_HOOK_BLOCK, self._blk)
    def _blk(self, mu, addr, size, ud):
        self.count += size // 4
        if self.prof is not None:
            i = bisect.bisect_right(self.addrs, addr) - 1
            n = self.names[i] if i >= 0 else '?'
            self.prof[n] = self.prof.get(n, 0) + size // 4
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
    need = e.call('emu_memneeded')
    assert need < MEMSZ, need
    e.call('emu_init', MEM)
    print('init: %d instructions, mem %d bytes' % (e.count, need))
    names = ['A smooth', 'B textured', 'C sprites']
    res = {}
    for m in range(3):
        e.call('emu_setmode', MEM, m)
        tot, n = 0, 0
        if prof: e.prof = {}
        for step in range(int(os.environ.get("STEPS","66"))):
            e.call('emu_tick', MEM, 100000, 0)
            if step % 6 == 0:                       # 15 frames spread over the 9 s path
                e.call('emu_render', MEM, FB)
                tot += e.count; n += 1
                if step == 48:
                    open(OUT + '/frame_m%d.raw' % m, 'wb').write(bytes(e.mu.mem_read(FB, 640 * 200 * 2)))
        tris = e.call('emu_tris', MEM)
        res[m] = tot // n
        print('%-11s %9d instructions/frame  (last frame tris %d)' % (names[m], tot // n, tris))
        if prof:
            t = sum(e.prof.values())
            for name, c in sorted(e.prof.items(), key=lambda x: -x[1])[:12]:
                print('      %5.1f%%  %s' % (100.0 * c / t, name))
    print('at 150 MHz, assuming ~1.4 cycles/instruction (cache misses, multiplies, loads):')
    for m in range(3):
        cyc = res[m] * 1.4
        print('  %-11s ~%.1f fps render-only' % (names[m], 150e6 / cyc))


def callers():
    from unicorn import UC_HOOK_CODE
    e = Emu(False)
    e.call('emu_init', MEM)
    cnt = {}
    tgt = [e.sym('__divsi3'), e.sym('__modsi3')]
    def h(mu, addr, size, ud):
        lr = mu.reg_read(UC_ARM_REG_LR)
        i = bisect.bisect_right(e.addrs, lr) - 1
        n = e.names[i]
        cnt[n] = cnt.get(n, 0) + 1
    for t in tgt: e.mu.hook_add(UC_HOOK_CODE, h, begin=t, end=t)
    e.call('emu_setmode', MEM, 0)
    for step in range(30):
        e.call('emu_tick', MEM, 100000, 0)
        if step % 6 == 0: e.call('emu_render', MEM, FB)
    for k, v in sorted(cnt.items(), key=lambda x: -x[1]): print(v // 5, k)

if sys.argv[1:2] == ["callers"]: callers()
else: main()

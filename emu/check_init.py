import sys, os
sys.path.insert(0, '/home/claude/game/emu')
sys.argv=['x','none']
import run_game as rg
from unicorn.arm_const import *
from unicorn import UcError
e = rg.Emu(False)
mu = e.mu
# enable alignment checking: SCTLR (CP15 c1) bit 1
try:
    v = mu.reg_read(UC_ARM_REG_CP_REG, (15, 0, 0, 1, 0, 0, 0, 0))
    mu.reg_write(UC_ARM_REG_CP_REG, (15, 0, 0, 1, 0, 0, 0, v | 2))
    print('SCTLR', hex(v), '->', hex(mu.reg_read(UC_ARM_REG_CP_REG, (15, 0, 0, 1, 0, 0, 0, 0))))
except Exception as ex:
    print('no sctlr', ex)
mu.mem_write(rg.STACK, b'\xAA' * rg.STACKSZ)
try:
    e.call('emu_init', rg.MEM, rg.LAND)
    print('init ok, %.1fM instr' % (e.count / 1e6))
except UcError as ex:
    pc = mu.reg_read(UC_ARM_REG_PC)
    import bisect
    i = bisect.bisect_right(e.addrs, pc) - 1
    print('FAULT', ex, 'pc', hex(pc), e.names[i] if i >= 0 else '?')
d = bytes(mu.mem_read(rg.STACK, rg.STACKSZ))
used = rg.STACKSZ - 0x100 - next(i for i in range(len(d)) if d[i] != 0xAA)
print('stack used during init: %d bytes' % used)

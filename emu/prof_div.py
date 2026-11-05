import os, sys, bisect
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_game as rg
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_LR
e = rg.Emu(False)
d = e.sym('__divsi3'); calls = {}
def hk(mu, addr, size, ud):
    lr = mu.reg_read(UC_ARM_REG_LR) & ~1
    i = bisect.bisect_right(e.addrs, lr) - 1
    n = e.names[i]; calls[n] = calls.get(n, 0) + 1
e.mu.hook_add(UC_HOOK_CODE, hk, begin=d, end=d)
e.call('emu_init', rg.MEM, rg.LAND)
for n, c in sorted(calls.items(), key=lambda x: -x[1])[:10]: print(c, n)

import sys, bisect
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PC
e = rg.Emu(False); mu = e.mu
e.call('emu_init', rg.MEM, rg.LAND)
hits = {}
def hk(mu, access, addr, size, value, ud):
    if size > 1 and (addr & (size - 1)):
        pc = mu.reg_read(UC_ARM_REG_PC)
        i = bisect.bisect_right(e.addrs, pc) - 1
        k = (e.names[i] if i >= 0 else '?', hex(pc), size)
        hits[k] = hits.get(k, 0) + 1
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hk)
for k in range(14):
    e.call('emu_interact', rg.MEM, rg.FB, k); print('case', k, hits, flush=True)

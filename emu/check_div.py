import sys, bisect
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_LR
e = rg.Emu(False); mu = e.mu
a = e.sym('__div0')
hits = {}
def hk(mu, addr, size, ud):
    lr = mu.reg_read(UC_ARM_REG_LR)
    i = bisect.bisect_right(e.addrs, lr) - 1
    hits[e.names[i]] = hits.get(e.names[i], 0) + 1
mu.hook_add(UC_HOOK_CODE, hk, begin=a, end=a + 3)
e.call('emu_init', rg.MEM, rg.LAND); print('init div0:', hits)
e.call('emu_title', rg.MEM)
for sc in [None, 0, 5, 8, 9, 10]:
    if sc is not None: e.call('emu_scene', rg.MEM, sc)
    for k in range(3): e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
print('frames div0:', hits)

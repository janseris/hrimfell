import sys, bisect, time
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PC
e = rg.Emu(False); mu = e.mu
hits = {}
def hk(mu, access, addr, size, value, ud):
    if size > 1 and (addr & (size - 1)):
        pc = mu.reg_read(UC_ARM_REG_PC)
        i = bisect.bisect_right(e.addrs, pc) - 1
        k = (e.names[i] if i >= 0 else '?', hex(pc), size)
        hits[k] = hits.get(k, 0) + 1
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hk)
t = time.time()
for k in range(6):
    e.call('emu_step', rg.MEM, k, rg.LAND); print('step', k + 1, 'done %.0fs' % (time.time() - t), hits, flush=True)
e.call('emu_title', rg.MEM)
for k in range(2): e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
print('title', hits, flush=True)
for sc in [0, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]:
    e.call('emu_scene', rg.MEM, sc)
    for k in range(2): e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
    print('scene', sc, hits, flush=True)

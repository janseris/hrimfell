import sys, bisect
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PC
e = rg.Emu(False); mu = e.mu
need = e.call('emu_memneeded')
hits = {}
def hk(mu, access, addr, size, value, ud):
    pc = mu.reg_read(UC_ARM_REG_PC)
    i = bisect.bisect_right(e.addrs, pc) - 1
    k = e.names[i] if i >= 0 else hex(pc)
    if k not in hits: hits[k] = (hex(addr - rg.MEM), hex(pc))
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hk, begin=rg.MEM + need, end=rg.MEM + rg.MEMSZ - 1)
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, hk, begin=0, end=rg.CODE - 1)
e.call('emu_init', rg.MEM, rg.LAND)
print('init: out-of-block accesses:', hits)
e.call('emu_title', rg.MEM)
for k in range(3):
    e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
print('frames:', hits)

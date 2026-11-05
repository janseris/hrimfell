import sys
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
e = rg.Emu(False); mu = e.mu
def used():
    d = bytes(mu.mem_read(rg.STACK, rg.STACKSZ))
    return rg.STACKSZ - 0x100 - next(i for i in range(len(d)) if d[i] != 0xAA)
mu.mem_write(rg.STACK, b'\xAA' * rg.STACKSZ)
e.call('emu_init', rg.MEM, rg.LAND); print('init', used())
e.call('emu_title', rg.MEM)
for sc in [None, 0, 5, 8, 10]:
    mu.mem_write(rg.STACK, b'\xAA' * rg.STACKSZ)
    if sc is not None: e.call('emu_scene', rg.MEM, sc)
    for k in range(3): e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
    print('scene', sc, 'stack', used())

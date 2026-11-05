import sys
sys.argv=['x']
sys.path.insert(0, '/home/claude/game/emu')
import run_game as rg
e = rg.Emu(False); mu = e.mu
need = e.call('emu_memneeded')
print('need', need)
mu.mem_write(rg.MEM + need, b'\x5A' * (rg.MEMSZ - need))
e.call('emu_init', rg.MEM, rg.LAND)
d = bytes(mu.mem_read(rg.MEM + need, rg.MEMSZ - need))
bad = [i for i in range(0, len(d)) if d[i] != 0x5A]
print('bytes written past the end:', len(bad), (hex(bad[0]), hex(bad[-1])) if bad else '')
for k in range(4):
    e.call('emu_tick', rg.MEM, 66000, 0); e.call('emu_render', rg.MEM, rg.FB)
d = bytes(mu.mem_read(rg.MEM + need, rg.MEMSZ - need))
bad = [i for i in range(0, len(d)) if d[i] != 0x5A]
print('after frames:', len(bad), (hex(bad[0]), hex(bad[-1])) if bad else '')

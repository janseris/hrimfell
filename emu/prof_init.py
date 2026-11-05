import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_game as rg
e = rg.Emu(True)
e.call('emu_init', rg.MEM, rg.LAND)
t = sum(e.prof.values())
print('init %.1fM' % (t / 1e6))
for n, c in sorted(e.prof.items(), key=lambda x: -x[1])[:16]: print('  %5.1f%%  %6.1fM  %s' % (100.0 * c / t, c / 1e6, n))

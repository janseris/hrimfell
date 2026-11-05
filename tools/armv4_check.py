#!/usr/bin/env python3
# (v0.4.74) ARMv4 register rules the assembler doesn't enforce (janseris/symbian-s80-build-tools native9300 4):
# MUL/MLA Rd must differ from Rm; SMULL/UMULL/SMLAL/UMLAL RdLo, RdHi and Rm all different.
#   python3 tools/armv4_check.py src/spans.s ...
import re, sys
bad = 0
for fn in sys.argv[1:]:
    for n, line in enumerate(open(fn), 1):
        code = line.split('@')[0].strip()
        m = re.match(r'(mul|mla)(\w*)\s+(\w+)\s*,\s*(\w+)', code, re.I)
        if m and m.group(3).lower() == m.group(4).lower():
            print('%s:%d: %s: Rd = Rm' % (fn, n, code)); bad += 1
        m = re.match(r'([su]mull|[su]mlal)(\w*)\s+(\w+)\s*,\s*(\w+)\s*,\s*(\w+)', code, re.I)
        if m and len({m.group(3).lower(), m.group(4).lower(), m.group(5).lower()}) < 3:
            print('%s:%d: %s: RdLo, RdHi, Rm not all different' % (fn, n, code)); bad += 1
print('%d problems' % bad)
sys.exit(1 if bad else 0)

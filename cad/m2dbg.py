"""Where two parts overlap, and which solids are loose: python m2dbg.py front encoder"""
import sys
from build123d import *
import m2check
p = m2check.parts()
if len(sys.argv) == 2:
    for s in m2check.solids(p[sys.argv[1]]):
        bb = s.bounding_box(); print(round(s.volume, 2), [round(v, 2) for v in (bb.min.X, bb.min.Y, bb.min.Z, bb.max.X, bb.max.Y, bb.max.Z)])
    sys.exit()
a, b = p[sys.argv[1]], p[sys.argv[2]]
for sa in m2check.solids(a):
    for sb in m2check.solids(b):
        try:
            i = sa & sb
        except Exception:
            continue
        if i is None:
            continue
        for s in m2check.solids(i):
            if s.volume > 1e-3:
                bb = s.bounding_box()
                print(round(s.volume, 3), [round(v, 2) for v in (bb.min.X, bb.min.Y, bb.min.Z, bb.max.X, bb.max.Y, bb.max.Z)])

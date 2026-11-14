#!/usr/bin/env python3
"""Render every panel with its components into one HTML page (for checking by eye).
    python3 scripts/preview_panels.py out.html
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_panels as mp

SIZES = {"knob": (12, 12), "jack": (8.4, 8.4), "switch": (mp.SW_W, mp.SW_H)}
out = ['<html><body style="background:#777;margin:0;display:flex;flex-wrap:wrap;gap:6px;padding:6px">']
comps = {k: v for k, v in mp.components().items()}
S = 4  # px per mm
for p in mp.P:
    out.append(f'<div style="position:relative;width:{p.w*S}px;height:{mp.H*S}px">')
    out.append(f'<img src="../res/{p.slug}.svg" style="position:absolute;left:0;top:0;width:{p.w*S}px;height:{mp.H*S}px">')
    for kind, pid, label, x, y, ex in p.items:
        if kind == "led":
            out.append(f'<div style="position:absolute;left:{(x-1.5)*S}px;top:{(y-1.5)*S}px;width:{3*S}px;height:{3*S}px;border-radius:50%;background:#a00"></div>')
            continue
        w, h = SIZES[kind]
        if kind == "knob": name = "SergeKnob"
        elif kind == "jack": name = mp.JACK_COLOURS[ex["colour"]]
        else: name = "Toggle2_0" if len(ex["positions"]) == 2 else "Toggle3_1"
        out.append(f'<img src="../res/components/{name}.svg" style="position:absolute;left:{(x-w/2)*S}px;top:{(y-h/2)*S}px;width:{w*S}px;height:{h*S}px">')
    out.append('</div>')
out.append('</body></html>')
open(sys.argv[1], 'w').write("\n".join(out))

#!/usr/bin/env python3
"""Generate the Serge panels (res/*.svg), the component SVGs (res/components/*.svg)
and the widget placement header (src/generated/Layout.hpp) from one table.

Every control is placed from the same coordinates in both outputs, every control
gets its printed label, and the script fails if anything overlaps, sits too close,
or leaves the panel.

    python3 scripts/make_panels.py [--font /path/to/DejaVuSans-Bold.ttf]

Needs: fontTools (pip install fonttools). Text is converted to paths here, because
Rack does not render SVG text.
"""
import math
import os
import random
import sys

from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
if "--font" in sys.argv:
    FONT = sys.argv[sys.argv.index("--font") + 1]

# ---------------------------------------------------------------- look ------
GROUND = "#ece6d6"
INK = "#161616"
H = 128.5
HP = 5.08
LABEL_CAP = 2.0       # mm cap height for labels
SMALL_CAP = 1.6       # switch positions, LED labels
TITLE_CAP = 2.8
CONDENSE = 0.86       # horizontal squeeze of the face

KNOB_R = 6.0          # knob body radius
KNOB_SCALE_R = 7.0    # scale arc radius
KNOB_BOX = 7.4
JACK_R = 4.2
JACK_BOX = 4.4
LED_R = 1.5
SW_W, SW_H = 5.6, 9.6

JACK_COLOURS = {"black": "JackBlack", "blue": "JackBlue", "red": "JackRed", "yellow": "JackYellow"}

# ---------------------------------------------------------------- text ------
_font = TTFont(FONT)
_glyphs = _font.getGlyphSet()
_cmap = _font.getBestCmap()
_upm = _font["head"].unitsPerEm
_cap = getattr(_font["OS/2"], "sCapHeight", 0) or 0.729 * _upm
_hmtx = _font["hmtx"]


def text_width(s, cap):
    scale = cap / _cap
    w = 0
    for ch in s:
        g = _cmap.get(ord(ch))
        if g is None:
            raise SystemExit(f"font has no glyph for {ch!r}")
        w += _hmtx[g][0]
    return w * scale * CONDENSE


def text_path(s, x, y, cap, anchor="middle"):
    """Path data for s with baseline at y. anchor: start|middle|end."""
    scale = cap / _cap
    w = text_width(s, cap)
    if anchor == "middle":
        x -= w / 2
    elif anchor == "end":
        x -= w
    d = []
    pen_x = x
    for ch in s:
        g = _cmap[ord(ch)]
        sp = SVGPathPen(_glyphs)
        tp = TransformPen(sp, (scale * CONDENSE, 0, 0, -scale, pen_x, y))
        _glyphs[g].draw(tp)
        d.append(sp.getCommands())
        pen_x += _hmtx[g][0] * scale * CONDENSE
    return " ".join(p for p in d if p)


# ---------------------------------------------------------------- model -----
class Box:
    def __init__(self, x0, y0, x1, y1, what):
        self.x0, self.y0, self.x1, self.y1, self.what = x0, y0, x1, y1, what

    def overlaps(self, o, pad=0.25):
        return not (self.x1 + pad <= o.x0 or o.x1 + pad <= self.x0 or self.y1 + pad <= o.y0 or o.y1 + pad <= self.y0)


class Panel:
    def __init__(self, slug, title, hp):
        self.slug, self.title, self.hp = slug, title, hp
        self.w = hp * HP
        self.items = []      # (kind, id, label, x, y, extra)
        self.sections = []   # (title, x0, y0, x1, y1)
        self.links = []      # (x0, y0, x1, y1)

    def knob(self, pid, label, x, y, bipolar=False):
        self.items.append(("knob", pid, label, x, y, {"bipolar": bipolar}))

    def jack(self, pid, label, x, y, colour):
        assert pid.endswith("_INPUT") or pid.endswith("_OUTPUT"), pid
        self.items.append(("jack", pid, label, x, y, {"colour": colour}))

    def switch(self, pid, positions, x, y, label=None):
        self.items.append(("switch", pid, label, x, y, {"positions": positions}))

    def led(self, pid, label, x, y):
        self.items.append(("led", pid, label, x, y, {}))

    def section(self, title, x0, y0, x1, y1):
        self.sections.append((title, x0, y0, x1, y1))

    def link(self, knob_id, jack_id):
        k = next(i for i in self.items if i[1] == knob_id)
        j = next(i for i in self.items if i[1] == jack_id)
        assert abs(k[3] - j[3]) < 0.01 and j[4] > k[4], (knob_id, jack_id)
        self.links.append((k[3], k[4] + KNOB_SCALE_R + 0.6, j[3], j[4] - JACK_R - 0.6))

    # ------------------------------------------------ geometry for checks --
    def label_lines(self, item):
        kind, pid, label, x, y, ex = item
        out = []   # (text, x, baseline, cap, anchor)
        if label is None:
            return out
        lines = label.split("\n")
        if kind == "knob":
            base = y - KNOB_SCALE_R - 1.4
            for i, ln in enumerate(reversed(lines)):
                out.append((ln, x, base - i * (LABEL_CAP + 0.8), LABEL_CAP, "middle"))
        elif kind == "jack":
            base = y + JACK_R + 1.0 + LABEL_CAP
            for i, ln in enumerate(lines):
                out.append((ln, x, base + i * (LABEL_CAP + 0.8), LABEL_CAP, "middle"))
        elif kind == "led":
            out.append((lines[0], x, y + LED_R + 0.9 + SMALL_CAP, SMALL_CAP, "middle"))
        elif kind == "switch":
            out.append((lines[0], x, y - SW_H / 2 - 1.0, LABEL_CAP, "middle"))
        return out

    def switch_labels(self, item):
        kind, pid, label, x, y, ex = item
        pos = ex["positions"]
        ys = [y - 3.4, y + 3.4 + SMALL_CAP] if len(pos) == 2 else [y - 3.4, y + SMALL_CAP / 2, y + 3.4 + SMALL_CAP]
        return [(p, x + SW_W / 2 + 0.8, yy, SMALL_CAP, "start") for p, yy in zip(pos, ys)]

    def all_labels(self):
        out = []
        for it in self.items:
            out += [(t, it) for t in self.label_lines(it)]
            if it[0] == "switch":
                out += [(t, it) for t in self.switch_labels(it)]
        return out

    def boxes(self):
        b = []
        for it in self.items:
            kind, pid, label, x, y, ex = it
            r = {"knob": KNOB_BOX, "jack": JACK_BOX, "led": LED_R + 0.2}.get(kind)
            if r:
                b.append(Box(x - r, y - r, x + r, y + r, pid))
            else:
                b.append(Box(x - SW_W / 2, y - SW_H / 2, x + SW_W / 2, y + SW_H / 2, pid))
        for (t, x, base, cap, anchor), it in self.all_labels():
            w = text_width(t, cap)
            x0 = x - w / 2 if anchor == "middle" else x if anchor == "start" else x - w
            b.append(Box(x0, base - cap, x0 + w, base, f"label '{t}' of {it[1]}"))
        return b

    def check(self):
        errs = []
        bx = self.boxes()
        for i in range(len(bx)):
            a = bx[i]
            if a.x0 < 1.6 or a.x1 > self.w - 1.6 or a.y0 < 13.0 or a.y1 > 120.5:
                errs.append(f"{a.what} leaves the panel area")
            for j in range(i + 1, len(bx)):
                if a.overlaps(bx[j]):
                    errs.append(f"{a.what} overlaps {bx[j].what}")
        # section title boxes and outlines
        for (title, x0, y0, x1, y1) in self.sections:
            tw = text_width(title, LABEL_CAP) + 2.0
            tb = Box((x0 + x1) / 2 - tw / 2, y0 - LABEL_CAP / 2 - 0.3, (x0 + x1) / 2 + tw / 2, y0 + LABEL_CAP / 2 + 0.3, f"section title {title}")
            for b in bx:
                if tb.overlaps(b):
                    errs.append(f"{b.what} overlaps section title {title}")
                inside = b.x0 >= x0 and b.x1 <= x1 and b.y0 >= y0 and b.y1 <= y1
                outside = b.x1 <= x0 or b.x0 >= x1 or b.y1 <= y0 or b.y0 >= y1
                if not (inside or outside):
                    errs.append(f"{b.what} crosses the outline of section {title}")
        # links must not cross anything but their own ends
        for (lx0, ly0, lx1, ly1) in self.links:
            lb = Box(lx0 - 0.2, ly0, lx1 + 0.2, ly1, "link")
            for b in bx:
                if lb.overlaps(b, pad=0):
                    errs.append(f"link at x={lx0:.1f} crosses {b.what}")
        # minimum spacing from the spec
        comps = [it for it in self.items if it[0] in ("knob", "jack")]
        for i in range(len(comps)):
            for j in range(i + 1, len(comps)):
                a, c = comps[i], comps[j]
                d = math.hypot(a[3] - c[3], a[4] - c[4])
                need = {("knob", "knob"): 15.0, ("jack", "jack"): 10.5}.get((a[0], c[0]), 13.0)
                if d < need - 1e-6:
                    errs.append(f"{a[1]} and {c[1]} are {d:.1f} mm apart (need {need})")
        # every control labelled
        for it in self.items:
            if it[0] != "switch" and not it[2]:
                errs.append(f"{it[1]} has no label")
        return errs

    # ------------------------------------------------------------ output --
    def svg(self):
        w = self.w
        out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:.2f}mm" height="{H}mm" viewBox="0 0 {w:.2f} {H}">']
        out.append(f'<rect x="0" y="0" width="{w:.2f}" height="{H}" fill="{GROUND}"/>')
        rnd = random.Random(self.slug)
        specks = []
        for _ in range(int(w * 3)):
            specks.append(f'<circle cx="{rnd.uniform(0, w):.2f}" cy="{rnd.uniform(0, H):.2f}" r="{rnd.uniform(0.08, 0.22):.2f}"/>')
        out.append(f'<g fill="#5a4a30" fill-opacity="0.06">{"".join(specks)}</g>')
        out.append(f'<rect x="1" y="1" width="{w - 2:.2f}" height="{H - 2}" fill="none" stroke="{INK}" stroke-width="0.4"/>')
        paths = []
        # title, shrunk to fit
        lines = self.title.split("\n")
        cap = TITLE_CAP
        while max(text_width(l, cap) for l in lines) > w - 4.5:
            cap -= 0.1
        base = 7.2 + cap if len(lines) == 1 else 6.2 + cap   # clear of the top screws (0 to 5.08 mm)
        for i, l in enumerate(lines):
            paths.append(text_path(l, w / 2, base + i * (cap + 1.0), cap))
        if self.hp <= 6:   # two screws only: bottom-left corner is free
            paths.append(text_path("SERGE", 2.6, 126.4, SMALL_CAP, "start"))
        else:
            paths.append(text_path("SERGE", w / 2, 126.4, SMALL_CAP))
        # sections
        g = []
        for (title, x0, y0, x1, y1) in self.sections:
            g.append(f'<rect x="{x0:.2f}" y="{y0:.2f}" width="{x1 - x0:.2f}" height="{y1 - y0:.2f}" rx="1.2" fill="none" stroke="{INK}" stroke-width="0.35"/>')
            tw = text_width(title, LABEL_CAP) + 2.0
            g.append(f'<rect x="{(x0 + x1) / 2 - tw / 2:.2f}" y="{y0 - 1.5:.2f}" width="{tw:.2f}" height="3" fill="{GROUND}"/>')
            paths.append(text_path(title, (x0 + x1) / 2, y0 + LABEL_CAP / 2, LABEL_CAP))
        for (lx0, ly0, lx1, ly1) in self.links:
            g.append(f'<line x1="{lx0:.2f}" y1="{ly0:.2f}" x2="{lx1:.2f}" y2="{ly1:.2f}" stroke="{INK}" stroke-width="0.35"/>')
        # knob scales
        for it in self.items:
            kind, pid, label, x, y, ex = it
            if kind == "knob":
                a0, a1 = math.radians(-135 - 90), math.radians(135 - 90)
                r = KNOB_SCALE_R
                sx, sy = x + r * math.cos(a0), y + r * math.sin(a0)
                ex_, ey = x + r * math.cos(a1), y + r * math.sin(a1)
                g.append(f'<path d="M{sx:.2f},{sy:.2f} A{r},{r} 0 1 1 {ex_:.2f},{ey:.2f}" fill="none" stroke="{INK}" stroke-width="0.35"/>')
                g.append(f'<circle cx="{sx:.2f}" cy="{sy:.2f}" r="0.45" fill="{INK}"/><circle cx="{ex_:.2f}" cy="{ey:.2f}" r="0.45" fill="{INK}"/>')
                if ex.get("bipolar"):
                    g.append(f'<line x1="{x:.2f}" y1="{y - r - 0.9:.2f}" x2="{x:.2f}" y2="{y - r + 0.4:.2f}" stroke="{INK}" stroke-width="0.35"/>')
                    # small - and + at the ends of the scale
                    paths.append(text_path("−", sx - 1.0, sy + 2.6, SMALL_CAP))
                    paths.append(text_path("+", ex_ + 1.0, ey + 2.6, SMALL_CAP))
            elif kind == "jack":
                g.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{JACK_R + 0.25:.2f}" fill="none" stroke="{INK}" stroke-width="0.25"/>')
            elif kind == "led":
                g.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{LED_R + 0.3:.2f}" fill="none" stroke="{INK}" stroke-width="0.25"/>')
        for (t, x, base_, cap_, anchor), it in self.all_labels():
            paths.append(text_path(t, x, base_, cap_, anchor))
        out.append("".join(g))
        out.append(f'<path fill="{INK}" d="{" ".join(paths)}"/>')
        out.append("</svg>")
        return "\n".join(out)

    def cpp(self):
        lines = [f"template <class M, class W>\ninline void place{self.slug}(W* w, M* m) {{"]
        for kind, pid, label, x, y, ex in self.items:
            v = f"mm2px(Vec({x:.3f}f, {y:.3f}f))"
            if kind == "knob":
                lines.append(f"\tw->addParam(createParamCentered<SergeKnob>({v}, m, M::{pid}));")
            elif kind == "switch":
                cls = "SergeToggle2" if len(ex["positions"]) == 2 else "SergeToggle3"
                lines.append(f"\tw->addParam(createParamCentered<{cls}>({v}, m, M::{pid}));")
            elif kind == "jack":
                cls = JACK_COLOURS[ex["colour"]]
                fn = "createInputCentered" if pid.endswith("_INPUT") else "createOutputCentered"
                add = "addInput" if pid.endswith("_INPUT") else "addOutput"
                lines.append(f"\tw->{add}({fn}<{cls}>({v}, m, M::{pid}));")
            elif kind == "led":
                lines.append(f"\tw->addChild(createLightCentered<MediumLight<RedLight>>({v}, m, M::{pid}));")
        lines.append("}\n")
        return "\n".join(lines)


# ---------------------------------------------------------------- panels ----
P = []


def cols(n, x0, x1):
    if n == 1:
        return [(x0 + x1) / 2]
    return [x0 + i * (x1 - x0) / (n - 1) for i in range(n)]


# 1. DSG ---------------------------------------------------------------------
p = Panel("DSG", "DUAL UNIVERSAL\nSLOPE GENERATOR", 14)
for s, (x0, x1) in zip("AB", [(2.0, 35.06), (36.06, 69.12)]):
    cx = (x0 + x1) / 2
    l, r = cx - 7.6, cx + 7.6
    p.section(s, x0, 15.5, x1, 119.5)
    p.knob(f"RISE_{s}_PARAM", "RISE", l, 29)
    p.knob(f"FALL_{s}_PARAM", "FALL", r, 29)
    p.knob(f"VC_{s}_PARAM", "VC", l, 49, bipolar=True)
    p.switch(f"MODE_{s}_PARAM", ["RISE", "BOTH", "FALL"], r - 2.2, 49.5)
    p.jack(f"IN_{s}_INPUT", "IN", l, 67, "black")
    p.jack(f"TRIG_{s}_INPUT", "TRIG", r, 67, "red")
    p.jack(f"VC_{s}_INPUT", "VC IN", l, 83, "blue")
    p.jack(f"VOCT_{s}_INPUT", "1V/OCT", r, 83, "blue")
    p.jack(f"OUT_{s}_OUTPUT", "OUT", l, 99, "blue")
    p.jack(f"END_{s}_OUTPUT", "END", r, 99, "red")
    p.led(f"LEVEL_{s}_LIGHT", "LEVEL", cx, 111.5)
P.append(p)

# 2. SSG ---------------------------------------------------------------------
p = Panel("SSG", "SMOOTH & STEPPED\nGENERATOR", 10)
for s, title, (x0, x1), ctl in [("SM", "SMOOTH", (2.0, 25.0), ("HOLD", "HOLD")), ("ST", "STEPPED", (25.8, 48.8), ("SAMPLE", "SAMPLE"))]:
    cx = (x0 + x1) / 2
    l, r = cx - 5.6, cx + 5.6
    p.section(title, x0, 15.5, x1, 101.5)
    p.knob(f"RATE_{s}_PARAM", "RATE", cx, 29)
    p.knob(f"VC_{s}_PARAM", "VC", cx, 49)
    p.jack(f"IN_{s}_INPUT", "IN", l, 64, "black")
    p.jack(f"VC_{s}_INPUT", "VC IN", r, 64, "blue")
    p.jack(f"{ctl[0]}_INPUT", ctl[1], l, 78.5, "red")
    p.jack(f"CYCLE_{s}_OUTPUT", "CYCLE", r, 78.5, "yellow")
    p.jack(f"OUT_{s}_OUTPUT", "OUT", l, 93, "blue")
    p.led(f"LEVEL_{s}_LIGHT", "LEVEL", r, 91.5)
p.jack("COUPLER_OUTPUT", "COUPLER", 14.0, 108.5, "red")
p.jack("HOT_OUTPUT", "HOT\nCOUPLER", 36.8, 108.5, "yellow")
P.append(p)

# 3. NTO ---------------------------------------------------------------------
p = Panel("NTO", "NEW TIMBRAL OSCILLATOR", 16)
c4 = cols(4, 11.5, 69.8)
p.knob("FREQ_PARAM", "FREQ", c4[0], 29)
p.knob("FINE_PARAM", "FINE", c4[1], 29, bipolar=True)
p.knob("PORT_PARAM", "PORT", c4[2], 29)
p.knob("FM_PARAM", "FM", c4[3], 29)
p.knob("VCFREQ_PARAM", "VC FREQ", c4[0], 50, bipolar=True)
p.knob("WAVE_PARAM", "WAVE", c4[2], 50)
p.knob("VCWAVE_PARAM", "VC WAVE", c4[3], 50, bipolar=True)
p.jack("FREQCV_INPUT", "FREQ CV", c4[0], 67, "blue")
p.jack("VOCT_INPUT", "1V/OCT", c4[1], 67, "blue")
p.jack("WAVECV_INPUT", "WAVE CV", c4[3], 67, "blue")
p.jack("SYNC_INPUT", "SYNC", c4[2], 67, "red")
p.jack("PORTVOCT_INPUT", "1V/OCT\nPORT", c4[1], 84, "blue")
p.jack("LINFM_INPUT", "LIN FM", c4[2], 84, "black")
p.jack("FMVC_INPUT", "FM VC", c4[3], 84, "blue")
p.section("OUT", 3.0, 96.0, 78.28, 119.5)
for x, (pid, lab, col) in zip(c4, [("SAW_OUTPUT", "SAW", "blue"), ("TRI_OUTPUT", "TRI", "blue"), ("SINE_OUTPUT", "SINE", "black"), ("VAR_OUTPUT", "VAR", "black")]):
    p.jack(pid, lab, x, 106.5, col)
p.link("VCFREQ_PARAM", "FREQCV_INPUT")
p.link("VCWAVE_PARAM", "WAVECV_INPUT")
P.append(p)

# 4. VCM ---------------------------------------------------------------------
p = Panel("VCM", "WAVE MULTIPLIERS", 14)
W = 14 * HP
p.section("1", 2.0, 15.5, W - 2.0, 41.5)
p.knob("VC1_PARAM", "VC", 12.0, 29)
p.switch("HILO_PARAM", ["HI", "LO"], 24.0, 29.5)
p.jack("IN1_INPUT", "IN", 39.0, 28, "black")
p.jack("VC1_INPUT", "VC", 51.0, 28, "blue")
p.jack("OUT1_OUTPUT", "OUT", 63.0, 28, "black")
p.section("2", 2.0, 44.0, W - 2.0, 81.0)
p.knob("FOLD_PARAM", "FOLD", 12.0, 57.5)
p.jack("OUT2_OUTPUT", "OUT", 63.0, 57.5, "black")
p.jack("IN2A_INPUT", "IN 1", 12.0, 72.5, "black")
p.jack("IN2B_INPUT", "IN 2", 27.0, 72.5, "black")
p.jack("VC2_INPUT", "VC", 42.0, 72.5, "blue")
p.section("3", 2.0, 83.5, W - 2.0, 119.5)
p.knob("VC3_PARAM", "VC", 12.0, 97)
p.jack("OUT3_OUTPUT", "OUT", 44.0, 97, "black")
p.jack("SQUARED_OUTPUT", "SQUARED", 59.0, 97, "blue")
p.jack("IN3A_INPUT", "IN 1", 12.0, 111.5, "black")
p.jack("IN3B_INPUT", "IN 2", 27.0, 111.5, "black")
p.jack("VC3_INPUT", "VC", 42.0, 111.5, "blue")
P.append(p)


# 5./6. filters --------------------------------------------------------------
def filter_panel(slug, title, knobs1, knobs2, jacks1, jacks2, outs):
    p = Panel(slug, title, 12)
    c = cols(3, 11.0, 49.96)
    for y, row in [(29, knobs1), (49, knobs2)]:
        for x, k in zip(c, row):
            if k:
                p.knob(k[0], k[1], x, y, bipolar=len(k) > 2)
    for y, row in [(66, jacks1), (82, jacks2)]:
        for x, j in zip(c, row):
            if j:
                p.jack(j[0], j[1], x, y, j[2])
    p.section("OUT", 3.0, 95.0, 57.96, 119.5)
    oc = cols(len(outs), 10.0, 50.96)
    for x, j in zip(oc, outs):
        p.jack(j[0], j[1], x, 105.5, j[2])
    return p


p = filter_panel("VCFQ", "VARIABLE Q VCF",
                 [("FREQ_PARAM", "FREQ"), ("Q_PARAM", "Q"), ("GAIN_PARAM", "GAIN")],
                 [("VCF_PARAM", "VC F", True), ("VCQ_PARAM", "VC Q"), None],
                 [("FCV_INPUT", "F CV", "blue"), ("QCV_INPUT", "Q CV", "blue"), ("VOCT_INPUT", "1V/OCT", "blue")],
                 [("IN_INPUT", "IN", "black"), ("AGC_INPUT", "AGC IN", "black"), ("TRIG_INPUT", "TRIG", "red")],
                 [("LOW_OUTPUT", "LOW", "black"), ("BAND_OUTPUT", "BAND", "black"), ("HIGH_OUTPUT", "HIGH", "black"), ("NOTCH_OUTPUT", "NOTCH", "black")])
p.link("VCF_PARAM", "FCV_INPUT")
p.link("VCQ_PARAM", "QCV_INPUT")
P.append(p)

p = filter_panel("VCFS", "VARIABLE SLOPE VCF",
                 [("FREQ_PARAM", "FREQ"), ("SLOPE_PARAM", "SLOPE"), ("MIX_PARAM", "MIX")],
                 [("VCF_PARAM", "VC F", True), ("VCSLOPE_PARAM", "VC SLOPE", True), None],
                 [("FCV_INPUT", "F CV", "blue"), ("SLOPECV_INPUT", "SLOPE CV", "blue"), ("VOCT_INPUT", "1V/OCT", "blue")],
                 [("IN1_INPUT", "IN 1", "black"), ("IN2_INPUT", "IN 2", "black"), None],
                 [("LOW_OUTPUT", "LOW", "black"), ("BAND_OUTPUT", "BAND", "black"), ("HIGH_OUTPUT", "HIGH", "black")])
p.link("VCF_PARAM", "FCV_INPUT")
p.link("VCSLOPE_PARAM", "SLOPECV_INPUT")
P.append(p)

# 7. RESEQ -------------------------------------------------------------------
p = Panel("RESEQ", "RESONANT EQUALIZER", 20)
c5 = cols(5, 12.0, 89.6)
labels = ["29", "61", "115", "218", "411", "777", "1.5K", "2.8K", "5.2K", "11K"]
for i, lab in enumerate(labels):
    p.knob(f"BAND{i}_PARAM", lab, c5[i % 5], 31 if i < 5 else 56, bipolar=True)
p.knob("INPUT_PARAM", "INPUT", c5[0], 82)
p.jack("IN_INPUT", "IN", c5[0], 102, "black")
p.section("OUT", c5[1] - 9.0, 92.0, c5[4] + 9.0, 119.5)
p.jack("OUT_OUTPUT", "OUT", c5[2], 102, "black")
p.jack("COMBUP_OUTPUT", "COMB ↑", c5[3], 102, "black")
p.jack("COMBDOWN_OUTPUT", "COMB ↓", c5[4], 102, "black")
p.link("INPUT_PARAM", "IN_INPUT")
P.append(p)

# 8. TWS ---------------------------------------------------------------------
p = Panel("TWS", "TRIPLE WAVESHAPER", 10)
c3 = cols(3, 9.5, 41.3)
for i, top in enumerate([15.5, 50.5, 85.5]):
    n = i + 1
    p.section(str(n), 2.0, top, 48.8, top + 34.0)
    p.knob(f"VC{n}_PARAM", "VC", c3[1], top + 13.0, bipolar=True)
    p.jack(f"IN{n}_INPUT", "IN", c3[0], top + 26.0, "black")
    p.jack(f"VC{n}_INPUT", "VC", c3[1], top + 26.0, "blue")
    p.jack(f"OUT{n}_OUTPUT", "OUT", c3[2], top + 26.0, "black")
P.append(p)

# 9. RING --------------------------------------------------------------------
p = Panel("RING", "RING MODULATOR", 8)
l, r = 11.0, 29.64
p.knob("MOD_PARAM", "MOD", l, 29)
p.knob("CARRIER_PARAM", "CARRIER", r, 29)
p.knob("VCMOD_PARAM", "VC MOD", l, 49, bipolar=True)
p.knob("VCCARRIER_PARAM", "VC\nCARRIER", r, 51, bipolar=True)
p.jack("MODCV_INPUT", "MOD CV", l, 67, "blue")
p.jack("CARRIERCV_INPUT", "CARRIER\nCV", r, 67, "blue")
p.jack("SIGNAL_INPUT", "SIGNAL", l, 86, "black")
p.jack("CARRIER_INPUT", "CARRIER", r, 86, "black")
p.jack("OUT_OUTPUT", "OUT", 20.32, 104, "black")
p.link("VCMOD_PARAM", "MODCV_INPUT")
p.link("VCCARRIER_PARAM", "CARRIERCV_INPUT")
P.append(p)

# 10. NOI --------------------------------------------------------------------
p = Panel("NOI", "NOISE\nSOURCE", 6)
p.jack("WHITE_OUTPUT", "WHITE", 15.24, 34, "black")
p.jack("PINK_OUTPUT", "PINK", 15.24, 56, "black")
p.jack("SH_OUTPUT", "S/H\nSOURCE", 15.24, 78, "blue")
P.append(p)

# 11. ASR --------------------------------------------------------------------
p = Panel("ASR", "ANALOG SHIFT\nREGISTER", 6)
x = 15.24
p.jack("IN_INPUT", "IN", x, 27, "blue")
p.jack("PULSE_INPUT", "PULSE", x, 42, "red")
p.section("OUT", 3.0, 52.5, 27.48, 119.5)
p.jack("OUT1_OUTPUT", "OUT 1", x, 62, "blue")
p.jack("OUT2_OUTPUT", "OUT 2", x, 76, "blue")
p.jack("OUT3_OUTPUT", "OUT 3", x, 90, "blue")
p.jack("PULSE_OUTPUT", "PULSE\nOUT", x, 104, "red")
P.append(p)

# 12. COM --------------------------------------------------------------------
p = Panel("COM", "DUAL\nCOMPARATOR", 8)
c3 = cols(3, 9.0, 31.64)
for s, top in [("A", 15.5), ("B", 56.0)]:
    p.section(s, 2.0, top, 38.64, top + 38.0)
    p.knob(f"THRESH_{s}_PARAM", "THRESHOLD", c3[1], top + 14.0, bipolar=True)
    p.jack(f"PLUS_{s}_INPUT", "+", c3[0], top + 28.0, "black")
    p.jack(f"MINUS_{s}_INPUT", "−", c3[1], top + 28.0, "black")
    p.jack(f"OUT_{s}_OUTPUT", "OUT", c3[2], top + 28.0, "red")
P.append(p)

# 13. STR --------------------------------------------------------------------
p = Panel("STR", "DUAL SCHMITT\nTRIGGER", 6)
x = 15.24
p.section("A", 3.0, 15.5, 27.48, 53.0)
p.jack("INA_INPUT", "IN A", x, 26, "black")
p.jack("OUTA_OUTPUT", "OUT A", x, 41, "red")
p.section("B", 3.0, 56.0, 27.48, 93.5)
p.jack("INB_INPUT", "IN B", x, 66.5, "black")
p.jack("OUTB_OUTPUT", "OUT B", x, 81.5, "red")
p.jack("RS_OUTPUT", "R/S", x, 104, "red")
P.append(p)

# 14. PRC --------------------------------------------------------------------
p = Panel("PRC", "DUAL PROCESSOR", 14)
c4 = cols(4, 11.0, 60.12)
for s, top in [("A", 15.5), ("B", 61.0)]:
    p.section(s, 2.0, top, 69.12, top + 41.0)
    for i in range(3):
        p.knob(f"K{i + 1}_{s}_PARAM", f"IN {i + 1}", c4[i], top + 14.0, bipolar=True)
        p.jack(f"IN{i + 1}_{s}_INPUT", f"IN {i + 1}", c4[i], top + 31.0, "blue")
        p.link(f"K{i + 1}_{s}_PARAM", f"IN{i + 1}_{s}_INPUT")
    p.knob(f"OFFSET_{s}_PARAM", "OFFSET", c4[3], top + 14.0, bipolar=True)
    p.jack(f"OUT_{s}_OUTPUT", "OUT", c4[3], top + 31.0, "blue")
P.append(p)

# 15. XFAD -------------------------------------------------------------------
p = Panel("XFAD", "CROSS-FADER", 8)
l, r = 11.0, 29.64
p.knob("FADE_PARAM", "FADE", l, 29)
p.knob("GAIN_PARAM", "GAIN", r, 29)
p.jack("FADECV_INPUT", "FADE CV", l, 46, "blue")
p.jack("GAINCV_INPUT", "GAIN CV", r, 46, "blue")
p.jack("IN1_INPUT", "IN 1", l, 66, "black")
p.jack("IN2_INPUT", "IN 2", r, 66, "black")
p.jack("OUT_OUTPUT", "OUT", 20.32, 86, "black")
p.link("FADE_PARAM", "FADECV_INPUT")
p.link("GAIN_PARAM", "GAINCV_INPUT")
P.append(p)

# 16. VCA2 -------------------------------------------------------------------
p = Panel("VCA2", "DUAL VCA", 8)
c3 = cols(3, 9.0, 31.64)
for s, top in [("A", 15.5), ("B", 56.0)]:
    p.section(s, 2.0, top, 38.64, top + 38.0)
    p.knob(f"GAIN_{s}_PARAM", "GAIN", c3[1], top + 14.0)
    p.jack(f"IN_{s}_INPUT", "IN", c3[0], top + 28.0, "black")
    p.jack(f"VC_{s}_INPUT", "VC", c3[1], top + 28.0, "blue")
    p.jack(f"OUT_{s}_OUTPUT", "OUT", c3[2], top + 28.0, "black")
P.append(p)

# 17. MIX --------------------------------------------------------------------
p = Panel("MIX", "DUAL AUDIO MIXER", 14)
c4 = cols(4, 11.0, 60.12)
for s, top in [("A", 15.5), ("B", 61.0)]:
    p.section(s, 2.0, top, 69.12, top + 41.0)
    for i in range(3):
        p.knob(f"L{i + 1}_{s}_PARAM", f"IN {i + 1}", c4[i], top + 14.0)
        p.jack(f"IN{i + 1}_{s}_INPUT", f"IN {i + 1}", c4[i], top + 31.0, "black")
        p.link(f"L{i + 1}_{s}_PARAM", f"IN{i + 1}_{s}_INPUT")
    p.jack(f"IN4_{s}_INPUT", "IN 4", c4[3], top + 31.0, "black")
    p.jack(f"OUT_{s}_OUTPUT", "OUT", c4[3], top + 13.0, "black")
P.append(p)

# 18. DCSM -------------------------------------------------------------------
p = Panel("DCSM", "DUAL CHANNEL\nSTEREO MIXER", 12)
c3 = cols(3, 11.0, 49.96)
for s, top in [("1", 15.5), ("2", 57.0)]:
    p.section(s, 2.0, top, 58.96, top + 39.0)
    p.knob(f"GAIN{s}_PARAM", "GAIN", c3[0], top + 14.0)
    p.knob(f"PAN{s}_PARAM", "PAN", c3[2], top + 14.0)
    p.jack(f"GAINCV{s}_INPUT", "GAIN CV", c3[0], top + 30.0, "blue")
    p.jack(f"IN{s}_INPUT", "IN", c3[1], top + 30.0, "black")
    p.jack(f"PANCV{s}_INPUT", "PAN CV", c3[2], top + 30.0, "blue")
    p.link(f"GAIN{s}_PARAM", f"GAINCV{s}_INPUT")
    p.link(f"PAN{s}_PARAM", f"PANCV{s}_INPUT")
c4 = cols(4, 9.0, 51.96)
p.jack("AUXL_INPUT", "AUX L", c4[0], 106.5, "black")
p.jack("AUXR_INPUT", "AUX R", c4[1], 106.5, "black")
p.jack("LEFT_OUTPUT", "LEFT", c4[2], 106.5, "black")
p.jack("RIGHT_OUTPUT", "RIGHT", c4[3], 106.5, "black")
P.append(p)

# 19. PT ---------------------------------------------------------------------
p = Panel("PT", "PEAK &\nTROUGH", 4)
x = 10.16
p.jack("A_INPUT", "A", x, 30, "blue")
p.jack("B_INPUT", "B", x, 46, "blue")
p.jack("PEAK_OUTPUT", "PEAK", x, 70, "blue")
p.jack("TROUGH_OUTPUT", "TROUGH", x, 86, "blue")
P.append(p)


# ---------------------------------------------------------------- components
def components():
    c = {}
    # banana jacks: coloured collar, lighter bevel, dark hole with brass ring
    for name, col, bevel in [("JackBlack", "#1b1b1b", "#3a3a3a"), ("JackBlue", "#1f4fa8", "#3d6cc4"),
                             ("JackRed", "#c0281c", "#d9493c"), ("JackYellow", "#e0b01a", "#f0c94a")]:
        c[name] = f'''<svg xmlns="http://www.w3.org/2000/svg" width="8.4mm" height="8.4mm" viewBox="0 0 8.4 8.4">
<circle cx="4.2" cy="4.2" r="4.2" fill="{col}"/>
<circle cx="4.2" cy="4.2" r="3.5" fill="none" stroke="{bevel}" stroke-width="0.6"/>
<circle cx="4.2" cy="4.2" r="2.25" fill="#b8913a"/>
<circle cx="4.2" cy="4.2" r="2.0" fill="#0c0c0c"/>
<circle cx="4.2" cy="4.2" r="0.9" fill="#2a2a2a"/>
</svg>'''
    # fluted, skirted black knob with a white line
    pts = []
    for i in range(48):
        a = 2 * math.pi * i / 48
        rr = 6.0 if i % 2 == 0 else 5.65
        pts.append(f"{6 + rr * math.sin(a):.3f},{6 - rr * math.cos(a):.3f}")
    c["SergeKnob"] = f'''<svg xmlns="http://www.w3.org/2000/svg" width="12mm" height="12mm" viewBox="0 0 12 12">
<polygon points="{" ".join(pts)}" fill="#111"/>
<circle cx="6" cy="6" r="4.6" fill="#1c1c1c" stroke="#2c2c2c" stroke-width="0.3"/>
<circle cx="5.4" cy="5.2" r="2.8" fill="#ffffff" fill-opacity="0.05"/>
<line x1="6" y1="1.2" x2="6" y2="4.4" stroke="#f2f2f2" stroke-width="0.7" stroke-linecap="round"/>
</svg>'''

    def toggle(frame, n):
        # bat handle: 0 = up, 1 = middle, 2 = down (3-pos); 2-pos uses up/down
        dirs = {0: -1, 1: 0, 2: 1}
        d = dirs[frame]
        cx, cy = 2.8, 4.8
        tip_y = cy + d * 3.6
        lean = 0.0 if d != 0 else 0
        handle = (f'<line x1="{cx}" y1="{cy}" x2="{cx + lean}" y2="{tip_y}" stroke="#d8d8d8" stroke-width="1.3" stroke-linecap="round"/>'
                  f'<circle cx="{cx + lean}" cy="{tip_y}" r="0.9" fill="#f0f0f0" stroke="#999" stroke-width="0.15"/>') if d != 0 else \
                 f'<circle cx="{cx}" cy="{cy}" r="1.3" fill="#f0f0f0" stroke="#999" stroke-width="0.2"/>'
        return f'''<svg xmlns="http://www.w3.org/2000/svg" width="5.6mm" height="9.6mm" viewBox="0 0 5.6 9.6">
<rect x="0.3" y="0.3" width="5.0" height="9.0" rx="0.6" fill="#2b2b2b"/>
<circle cx="{cx}" cy="{cy}" r="2.1" fill="#9a9a9a" stroke="#6a6a6a" stroke-width="0.25"/>
<circle cx="{cx}" cy="{cy}" r="1.4" fill="#bdbdbd"/>
{handle}
</svg>'''
    # frame 0 is value 0. Value 0 is the TOP position on the panel labels (first position).
    c["Toggle2_0"] = toggle(0, 2)
    c["Toggle2_1"] = toggle(2, 2)
    c["Toggle3_0"] = toggle(0, 3)
    c["Toggle3_1"] = toggle(1, 3)
    c["Toggle3_2"] = toggle(2, 3)
    return c


def main():
    errs = []
    for p in P:
        for e in p.check():
            errs.append(f"{p.slug}: {e}")
    if errs:
        print("\n".join(errs))
        raise SystemExit(f"{len(errs)} layout problems")
    os.makedirs(os.path.join(ROOT, "res", "components"), exist_ok=True)
    os.makedirs(os.path.join(ROOT, "src", "generated"), exist_ok=True)
    for p in P:
        with open(os.path.join(ROOT, "res", f"{p.slug}.svg"), "w") as f:
            f.write(p.svg())
    for name, svg in components().items():
        with open(os.path.join(ROOT, "res", "components", f"{name}.svg"), "w") as f:
            f.write(svg)
    with open(os.path.join(ROOT, "src", "generated", "Layout.hpp"), "w") as f:
        f.write("// Generated by scripts/make_panels.py. Do not edit.\n#pragma once\n#include \"../widgets/SergeComponents.hpp\"\n\n")
        for p in P:
            f.write(p.cpp())
    print(f"wrote {len(P)} panels")


if __name__ == "__main__":
    main()

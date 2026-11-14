#!/usr/bin/env bash
# Builds docs/Serge-Manual.pdf from docs/MANUAL.md. Needs pandoc, python3 and Node with Playwright (Chromium).
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
tmp="$(mktemp -d)"
cp "$root/scripts/manual/style.css" "$tmp/style.css"
pandoc "$root/docs/MANUAL.md" -s --metadata title="Serge for VCV Rack: manual" -c style.css -o "$tmp/manual.html"
python3 - "$tmp/manual.html" <<'PY'
import re, sys
p = sys.argv[1]; h = open(p).read()
for name, col in [('Black', '#111'), ('Blue', '#2b6fd6'), ('Red', '#d42a2a'), ('Yellow', '#f2c40f')]:
    h = h.replace('<td><strong>%s</strong></td>' % name,
                  '<td><span class="sw" style="background:%s"></span><strong>%s</strong></td>' % (col, name))
h = re.sub(r'<header id="title-block-header">.*?</header>', '', h, flags=re.S)
open(p, 'w').write(h)
PY
node "$root/scripts/manual/topdf.js" "$tmp/manual.html" "$root/docs/Serge-Manual.pdf"
rm -rf "$tmp"
echo "wrote docs/Serge-Manual.pdf"

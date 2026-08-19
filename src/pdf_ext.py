import sys
import re

if len(sys.argv) < 2:
    sys.exit(1)

try:
    import pymupdf
    doc = pymupdf.open(sys.argv[1])
    if len(doc) > 0:
        page = doc.load_page(0)
        text = page.get_text("text")
        lines = text.split('\n')
        for line in lines:
            line = line.strip()
            if len(line) >= 3 and len(line) <= 100 and re.search(r'[a-zA-Z]', line) and not line.lower().startswith('warning'):
                print(line)
                sys.exit(0)
except Exception:
    pass
print("")

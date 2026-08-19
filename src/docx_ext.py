import sys
import zipfile
import xml.etree.ElementTree as ET
import re

if len(sys.argv) < 2:
    sys.exit(1)

try:
    docx_path = sys.argv[1]
    with zipfile.ZipFile(docx_path) as docx:
        xml_content = docx.read('word/document.xml')
        tree = ET.fromstring(xml_content)
        
        # Define the WordprocessingML namespace
        ns = {'w': 'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
        
        # Find all paragraph elements
        for p in tree.findall('.//w:p', ns):
            # Extract text from all text nodes within the paragraph
            texts = [node.text for node in p.findall('.//w:t', ns) if node.text]
            full_text = "".join(texts).strip()
            
            # Skip empty or symbol-only lines
            if len(full_text) >= 3 and re.search(r'[a-zA-Z]{3,}', full_text):
                print(full_text)
                sys.exit(0)
except Exception:
    pass
print("")

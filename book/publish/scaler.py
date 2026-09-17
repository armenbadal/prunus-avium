import sys
from pypdf import PdfReader, PdfWriter

reader = PdfReader(sys.argv[1])
writer = PdfWriter()

POINT = 2.8346
b5_width = 170 * POINT
b5_height = 240 * POINT

for page in reader.pages:
    page.scale_to(width=b5_width, height=b5_height)
    writer.add_page(page)

with open(sys.argv[2], 'wb') as f:
    writer.write(f)

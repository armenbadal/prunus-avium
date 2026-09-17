#!/usr/bin/env python3

import argparse
import io

from pypdf import PdfReader, PdfWriter
from reportlab.pdfgen import canvas


def create_watermark(width, height, text):
    buffer = io.BytesIO()

    c = canvas.Canvas(buffer, pagesize=(width, height))

    c.saveState()
    c.setFillAlpha(0.15)
    c.setFont("Helvetica-Bold", 48)

    c.translate(width / 2, height / 2)
    c.rotate(45)

    c.drawCentredString(0, 0, text)

    c.restoreState()
    c.save()

    buffer.seek(0)

    return PdfReader(buffer).pages[0]


def add_watermark(input_pdf, output_pdf, text):
    reader = PdfReader(input_pdf)
    writer = PdfWriter()

    for page in reader.pages:
        width = float(page.mediabox.width)
        height = float(page.mediabox.height)

        watermark = create_watermark(width, height, text)

        page.merge_page(watermark)
        writer.add_page(page)

    with open(output_pdf, "wb") as f:
        writer.write(f)


def main():
    parser = argparse.ArgumentParser(
        description="Add a text watermark to every page of a PDF."
    )

    parser.add_argument("input", help="Input PDF")
    parser.add_argument("output", help="Output PDF")
    parser.add_argument(
        "text",
        help="Watermark text",
    )

    args = parser.parse_args()

    add_watermark(args.input, args.output, args.text)


if __name__ == "__main__":
    main()

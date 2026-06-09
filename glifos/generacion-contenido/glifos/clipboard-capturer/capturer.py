#!/usr/bin/env python3
"""Glifo clipboard-capturer: lee el portapapeles, toma el texto de la IA y lo guarda como PDF."""

import json, os, sys, subprocess
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RESPONSE_FILE = os.path.join(PANAL_DIR, "respuesta_ia.txt")
DESKTOP = os.path.join(os.path.expanduser("~"), "Desktop")
GUIONES_DIR = os.path.join(DESKTOP, "guiones")


def paste_from_clipboard():
    try:
        import pyperclip
        return pyperclip.paste()
    except ImportError:
        try:
            if os.name == "nt":
                r = subprocess.run(["powershell", "-command", "Get-Clipboard"],
                                   capture_output=True, text=True, timeout=5)
                return r.stdout.strip()
            elif os.name == "posix":
                r = subprocess.run(["xclip", "-selection", "clipboard", "-o"],
                                   capture_output=True, text=True, timeout=5)
                return r.stdout.strip()
        except: pass
    return ""


def load_topic():
    theme_file = os.path.join(PANAL_DIR, "tema_seleccionado.json")
    if os.path.exists(theme_file):
        with open(theme_file, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data.get("seleccionado", {}).get("topic", "articulo")
    return "articulo"


def generate_pdf(topic, content, output_path):
    try:
        from reportlab.lib.pagesizes import A4
        from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
        from reportlab.lib.enums import TA_JUSTIFY
        from reportlab.lib.units import cm
        from reportlab.lib.colors import HexColor
        from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, HRFlowable

        doc = SimpleDocTemplate(output_path, pagesize=A4,
            leftMargin=2.5*cm, rightMargin=2.5*cm,
            topMargin=2*cm, bottomMargin=2*cm)

        styles = getSampleStyleSheet()
        s_title = ParagraphStyle("t", parent=styles["Title"],
            fontSize=18, leading=22, spaceAfter=12, fontName="Helvetica-Bold")
        s_date = ParagraphStyle("d", parent=styles["Normal"],
            fontSize=9, textColor=HexColor("#888"), spaceAfter=16)
        s_body = ParagraphStyle("b", parent=styles["Normal"],
            fontSize=10.5, leading=14, alignment=TA_JUSTIFY, spaceAfter=8)
        s_h2 = ParagraphStyle("h", parent=styles["Heading2"],
            fontSize=13, leading=16, spaceBefore=14, spaceAfter=6, fontName="Helvetica-Bold")

        story = [Paragraph(topic, s_title),
            Paragraph(f"Capturado el {datetime.now().strftime('%d/%m/%Y %H:%M')}", s_date),
            HRFlowable(width="100%", thickness=0.5, color=HexColor("#ccc")), Spacer(1, 8)]

        for line in content.split("\n"):
            line = line.strip()
            if not line: story.append(Spacer(1, 4))
            elif line.startswith("##") or line.startswith("**"):
                story.append(Paragraph(line.replace("##", "").replace("**", "").strip(), s_h2))
            elif line.startswith("#"): pass
            elif line.startswith("---"):
                story.append(HRFlowable(width="100%", thickness=0.3, color=HexColor("#aaa")))
            elif line.startswith("*"):
                story.append(Paragraph(line, ParagraphStyle("f",
                    parent=s_body, fontSize=8.5, textColor=HexColor("#666"))))
            else: story.append(Paragraph(line, s_body))

        doc.build(story)
        return True
    except ImportError:
        txt_path = output_path.replace(".pdf", ".txt")
        with open(txt_path, "w", encoding="utf-8") as f:
            f.write(f"{topic}\n{'=' * 60}\n\n{content}")
        print(f"[clipboard-capturer] reportlab no disponible. TXT: {txt_path}")
        return True
    except Exception as e:
        print(f"[clipboard-capturer] Error PDF: {e}")
        return False


def main():
    print("[clipboard-capturer] Leyendo portapapeles...")

    text = paste_from_clipboard()
    if not text or len(text) < 50:
        resp_file = os.path.join(PANAL_DIR, "respuesta_ia.txt")
        if os.path.exists(resp_file):
            with open(resp_file, "r", encoding="utf-8") as f:
                text = f.read()
            header = text.find("\n\n")
            if header > 0: text = text[header:].strip()
            print(f"[clipboard-capturer] Leyendo de {resp_file}")
        else:
            print("[clipboard-capturer] Portapapeles vacio. Pega el texto y presiona Enter:")
            try: text = sys.stdin.read()
            except: text = ""

    if not text or len(text) < 50:
        print("[clipboard-capturer] ERROR: no hay texto para guardar")
        sys.exit(1)

    topic = load_topic()
    os.makedirs(GUIONES_DIR, exist_ok=True)

    safe_topic = "".join(c if c.isalnum() or c in " -_" else "_" for c in topic)[:60]
    pdf_name = f"{datetime.now().strftime('%Y%m%d_%H%M')}_{safe_topic}.pdf"
    pdf_path = os.path.join(GUIONES_DIR, pdf_name)

    if generate_pdf(topic, text, pdf_path):
        print(f"[clipboard-capturer] PDF guardado: {pdf_path}")
        print(f"[clipboard-capturer] Palabras: ~{len(text.split())}")
        return 0

    print("[clipboard-capturer] ERROR: no se pudo generar el PDF")
    sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())

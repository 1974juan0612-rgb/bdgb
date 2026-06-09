#!/usr/bin/env python3
"""Glifo content-writer: genera articulo profundo con IA externa y lo guarda como PDF"""

import json, os, sys, subprocess, platform, textwrap
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
THEME_FILE = os.path.join(PANAL_DIR, "tema_seleccionado.json")

AI_API_URL = os.environ.get("BDGB_AI_API_URL", "https://api.openai.com/v1/chat/completions")
AI_API_KEY = os.environ.get("BDGB_AI_API_KEY", "")
AI_MODEL = os.environ.get("BDGB_AI_MODEL", "gpt-4o-mini")

DESKTOP = os.path.join(os.path.expanduser("~"), "Desktop")
GUIONES_DIR = os.path.join(DESKTOP, "guiones")


def call_external_ai(topic):
    if not AI_API_KEY:
        print("[content-writer] BDGB_AI_API_KEY no configurada. Usando modo simulado.")
        return generate_mock_article(topic)

    import urllib.request
    payload = json.dumps({
        "model": AI_MODEL,
        "messages": [
            {"role": "system", "content": "Eres un periodista experto en tecnologia y cultura digital. "
             "Escribe un articulo profundo, bien estructurado y detallado en español "
             "para una publicacion de divulgacion. Extension: 800-1200 palabras. "
             "Incluye introduccion, 3-4 secciones con subtitulos, y conclusion."},
            {"role": "user", "content": f"Escribe un articulo detallado sobre: {topic}"}
        ],
        "max_tokens": 2048,
        "temperature": 0.7
    }).encode("utf-8")

    req = urllib.request.Request(AI_API_URL, data=payload,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {AI_API_KEY}"
        })
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            result = json.loads(resp.read().decode("utf-8"))
            content = result.get("choices", [{}])[0].get("message", {}).get("content", "")
            if content:
                return content
    except Exception as e:
        print(f"[content-writer] Error llamando IA: {e}")

    print("[content-writer] Fallback a modo simulado")
    return generate_mock_article(topic)


def generate_mock_article(topic):
    sections = [
        f"# {topic}\n\n",
        f"## Introduccion\n\n",
        f"En los ultimos anos, '{topic}' ha emergido como una de las tendencias mas "
        f"significativas en el panorama digital actual. Este articulo explora en "
        f"profundidad sus origenes, impacto y proyecciones futuras.\n\n",

        f"## Contexto Historico\n\n",
        f"El fenomeno de '{topic}' no surge de la nada. Sus raices se remontan a "
        f"desarrollos previos en el campo, donde innovadores y comunidades "
        f"tempranas sentaron las bases para lo que hoy conocemos. "
        f"La evolucion ha sido constante, con hitos clave que marcaron "
        f"puntos de inflexion en su adopcion y comprension publica.\n\n",

        f"## Impacto Actual\n\n",
        f"Hoy, '{topic}' esta transformando la manera en que las personas "
        f"interactuan con la tecnologia. Desde aplicaciones practicas en la vida "
        f"cotidiana hasta implicaciones en industrias enteras, su alcance es amplio "
        f"y creciente. Los datos mas recientes muestran un incremento sostenido "
        f"en el interes y la inversion en esta area.\n\n",

        f"## Perspectivas Futuras\n\n",
        f"De cara al futuro, las proyecciones para '{topic}' son prometedoras. "
        f"Expertos del sector anticipan desarrollos que podrian redefinir "
        f"completamente el campo. La convergencia con otras tecnologias "
        f"emergentes abre posibilidades que hoy apenas comenzamos a imaginar.\n\n",

        f"## Conclusion\n\n",
        f"'{topic}' representa no solo una tendencia pasajera, sino un cambio "
        f"profundo en nuestra relacion con la tecnologia. Comprenderlo es "
        f"esencial para navegar el panorama digital del presente y del futuro. "
        f"La invitacion es a mantenernos curiosos, informados y participativos "
        f"en esta conversacion que recien comienza.\n\n",

        f"---\n",
        f"*Articulo generado automaticamente por BDGB Generacion de Contenido*\n",
        f"*Fuente: analisis de tendencias en internet*\n",
        f"*Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M')}*\n"
    ]
    return "".join(sections)


def generate_pdf(topic, content, output_path):
    try:
        from reportlab.lib.pagesizes import A4
        from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
        from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
        from reportlab.lib.units import cm
        from reportlab.lib.colors import HexColor
        from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, HRFlowable

        doc = SimpleDocTemplate(output_path, pagesize=A4,
            leftMargin=2.5*cm, rightMargin=2.5*cm,
            topMargin=2*cm, bottomMargin=2*cm)

        styles = getSampleStyleSheet()
        s_title = ParagraphStyle("ArtTitle", parent=styles["Title"],
            fontSize=18, leading=22, spaceAfter=12,
            fontName="Helvetica-Bold")
        s_date = ParagraphStyle("ArtDate", parent=styles["Normal"],
            fontSize=9, textColor=HexColor("#888"), spaceAfter=16)
        s_body = ParagraphStyle("ArtBody", parent=styles["Normal"],
            fontSize=10.5, leading=14, alignment=TA_JUSTIFY,
            spaceAfter=8)
        s_h2 = ParagraphStyle("ArtH2", parent=styles["Heading2"],
            fontSize=13, leading=16, spaceBefore=14, spaceAfter=6,
            fontName="Helvetica-Bold")

        story = []
        story.append(Paragraph(topic, s_title))
        story.append(Paragraph(
            f"Generado el {datetime.now().strftime('%d/%m/%Y a las %H:%M')} | "
            f"BDGB Panal: generacion-contenido", s_date))
        story.append(HRFlowable(width="100%", thickness=0.5, color=HexColor("#ccc")))
        story.append(Spacer(1, 8))

        lines = content.split("\n")
        for line in lines:
            line = line.strip()
            if not line:
                story.append(Spacer(1, 4))
            elif line.startswith("##"):
                story.append(Paragraph(line.replace("##", "").strip(), s_h2))
            elif line.startswith("#"):
                pass
            elif line.startswith("---"):
                story.append(HRFlowable(width="100%", thickness=0.3, color=HexColor("#aaa")))
            elif line.startswith("*"):
                story.append(Paragraph(line, ParagraphStyle("footer",
                    parent=s_body, fontSize=8.5, textColor=HexColor("#666"))))
            else:
                story.append(Paragraph(line, s_body))

        doc.build(story)
        return True
    except ImportError:
        print("[content-writer] reportlab no disponible. Generando .txt en lugar de PDF.")
        txt_path = output_path.replace(".pdf", ".txt")
        with open(txt_path, "w", encoding="utf-8") as f:
            f.write(f"{topic}\n")
            f.write("=" * 60 + "\n\n")
            f.write(content)
        print(f"[content-writer] Articulo guardado como TXT: {txt_path}")
        return True
    except Exception as e:
        print(f"[content-writer] Error generando PDF: {e}")
        return False


def main():
    print("[content-writer] Leyendo tema seleccionado...")
    if not os.path.exists(THEME_FILE):
        print(f"[content-writer] ERROR: {THEME_FILE} no encontrado. Ejecutar authority-selector primero")
        sys.exit(1)

    with open(THEME_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)

    selected = data.get("seleccionado", {})
    topic = selected.get("topic", "")
    if not topic:
        print("[content-writer] ERROR: no hay tema seleccionado")
        sys.exit(1)

    print(f"[content-writer] Generando articulo sobre: {topic}")

    os.makedirs(GUIONES_DIR, exist_ok=True)
    print(f"[content-writer] Carpeta destino: {GUIONES_DIR}")

    article = call_external_ai(topic)

    safe_topic = "".join(c if c.isalnum() or c in " -_" else "_" for c in topic)[:60]
    timestamp = datetime.now().strftime("%Y%m%d_%H%M")
    pdf_name = f"{timestamp}_{safe_topic}.pdf"
    pdf_path = os.path.join(GUIONES_DIR, pdf_name)

    if generate_pdf(topic, article, pdf_path):
        print(f"[content-writer] Articulo PDF guardado: {pdf_path}")
        print(f"[content-writer] Palabras: ~{len(article.split())}")
        return 0
    else:
        print("[content-writer] ERROR: no se pudo generar el PDF")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())

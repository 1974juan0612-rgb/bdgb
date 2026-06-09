#!/usr/bin/env python3
"""Glifo content-writer: abre la IA en el navegador como un usuario humano.
   La IA no sabe que es un glifo. Escribe el articulo y lo guarda como PDF."""

import json, os, sys, time, subprocess, webbrowser
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
THEME_FILE = os.path.join(PANAL_DIR, "tema_seleccionado.json")
AI_URL = os.environ.get("BDGB_AI_WEB_URL", "https://chat.openai.com")
BROWSER_CMD = os.environ.get("BDGB_BROWSER", "")

DESKTOP = os.path.join(os.path.expanduser("~"), "Desktop")
GUIONES_DIR = os.path.join(DESKTOP, "guiones")


def build_prompt(topic):
    return (
        f"Escribe un articulo detallado y bien estructurado en espanol sobre: {topic}.\n\n"
        f"Debe tener introduccion, 3-4 secciones con subtitulos, y conclusion. "
        f"Extension aproximada: 800-1200 palabras. "
        f"Tono periodistico para una publicacion de divulgacion."
    )


def open_browser(url):
    if BROWSER_CMD:
        subprocess.Popen(BROWSER_CMD.split() + [url], shell=(os.name == "nt"))
    else:
        webbrowser.open(url)


def try_browser_automation(topic):
    """Intenta automatizar el navegador como un humano: abre, pega, envia, copia resultado."""
    try:
        import pyautogui as pg
        import pyperclip
    except ImportError:
        return None

    prompt = build_prompt(topic)

    print("[content-writer] Abriendo navegador como usuario humano...")
    open_browser(AI_URL)
    time.sleep(5)

    pg.hotkey("ctrl", "l")
    time.sleep(0.3)
    pg.write(AI_URL, interval=0.02)
    pg.press("enter")
    time.sleep(4)

    pyperclip.copy(prompt)
    pg.hotkey("ctrl", "v")
    time.sleep(1)
    pg.press("enter")
    print("[content-writer] Prompt enviado. Esperando respuesta de la IA...")
    time.sleep(20)

    pg.hotkey("ctrl", "a")
    time.sleep(0.5)
    pg.hotkey("ctrl", "c")
    time.sleep(0.5)

    result = pyperclip.paste()
    if result and len(result) > 50 and topic.lower() in result.lower():
        print(f"[content-writer] Respuesta capturada ({len(result)} caracteres)")
        return result

    print("[content-writer] No se pudo capturar la respuesta automaticamente.")
    return None


def interact_with_user(topic):
    """Modo interactivo: abre el navegador, muestra el prompt, espera que el usuario pegue el resultado."""
    prompt = build_prompt(topic)

    print("\n" + "=" * 60)
    print("  CONTENT-WRITER: MODO INTERACTIVO")
    print("  La IA se usara como herramienta a traves del navegador.")
    print("  Ella no sabe que es un glifo.")
    print("=" * 60)
    print(f"\n  Tema: {topic}")
    print(f"\n  Prompt copiado al portapapeles. Pega esto en la IA:")
    print(f"  {AI_URL}")
    print(f"\n  {prompt[:120]}...")
    print("\n  [INSTRUCCIONES]")
    print("  1. La IA se abrira en tu navegador")
    print("  2. Pega el prompt (Ctrl+V)")
    print("  3. Espera la respuesta")
    print("  4. Copia TODO el texto de respuesta (Ctrl+A, Ctrl+C)")
    print("  5. Presiona Enter aqui cuando estes listo")
    print("=" * 60)

    import pyperclip
    pyperclip.copy(prompt)
    open_browser(AI_URL)

    input("\n  Presiona Enter despues de copiar la respuesta de la IA...")

    result = pyperclip.paste()
    if result and len(result) > 50:
        print(f"[content-writer] Texto capturado del portapapeles ({len(result)} caracteres)")
        return result

    print("[content-writer] Portapapeles vacio. Pega el texto manualmente y presiona Enter:")
    result = sys.stdin.read()
    return result.strip()


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


def get_article(topic):
    intent = os.environ.get("BDGB_AI_MODE", "browser")

    if intent == "mock":
        print("[content-writer] Modo simulado (BDGB_AI_MODE=mock)")
        return generate_mock_article(topic)

    if intent == "interactive":
        print("[content-writer] Modo interactivo: la IA se usara por el navegador")
        result = interact_with_user(topic)
        if result:
            return result
        print("[content-writer] Fallback a modo simulado")
        return generate_mock_article(topic)

    print("[content-writer] Abriendo IA en el navegador como un usuario humano...")
    print("[content-writer] La IA no sabe que es un glifo")
    result = try_browser_automation(topic)
    if result:
        return result

    result = interact_with_user(topic)
    if result:
        return result

    print("[content-writer] Fallback a modo simulado")
    return generate_mock_article(topic)


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
            fontSize=18, leading=22, spaceAfter=12, fontName="Helvetica-Bold")
        s_date = ParagraphStyle("ArtDate", parent=styles["Normal"],
            fontSize=9, textColor=HexColor("#888"), spaceAfter=16)
        s_body = ParagraphStyle("ArtBody", parent=styles["Normal"],
            fontSize=10.5, leading=14, alignment=TA_JUSTIFY, spaceAfter=8)
        s_h2 = ParagraphStyle("ArtH2", parent=styles["Heading2"],
            fontSize=13, leading=16, spaceBefore=14, spaceAfter=6, fontName="Helvetica-Bold")

        story = []
        story.append(Paragraph(topic, s_title))
        story.append(Paragraph(
            f"Generado el {datetime.now().strftime('%d/%m/%Y a las %H:%M')} | "
            f"BDGB Panal: generacion-contenido", s_date))
        story.append(HRFlowable(width="100%", thickness=0.5, color=HexColor("#ccc")))
        story.append(Spacer(1, 8))

        for line in content.split("\n"):
            line = line.strip()
            if not line:
                story.append(Spacer(1, 4))
            elif line.startswith("##") or line.startswith("**"):
                story.append(Paragraph(line.replace("##", "").replace("**", "").strip(), s_h2))
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
        txt_path = output_path.replace(".pdf", ".txt")
        with open(txt_path, "w", encoding="utf-8") as f:
            f.write(f"{topic}\n{'=' * 60}\n\n{content}")
        print(f"[content-writer] reportlab no disponible. Articulo TXT: {txt_path}")
        return True
    except Exception as e:
        print(f"[content-writer] Error generando PDF: {e}")
        return False


def main():
    print("[content-writer] Leyendo tema seleccionado...")
    if not os.path.exists(THEME_FILE):
        print(f"[content-writer] ERROR: {THEME_FILE} no encontrado")
        sys.exit(1)

    with open(THEME_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)

    selected = data.get("seleccionado", {})
    topic = selected.get("topic", "")
    if not topic:
        print("[content-writer] ERROR: no hay tema")
        sys.exit(1)

    print(f"[content-writer] Tema: {topic}")
    os.makedirs(GUIONES_DIR, exist_ok=True)

    article = get_article(topic)

    safe_topic = "".join(c if c.isalnum() or c in " -_" else "_" for c in topic)[:60]
    timestamp = datetime.now().strftime("%Y%m%d_%H%M")
    pdf_name = f"{timestamp}_{safe_topic}.pdf"
    pdf_path = os.path.join(GUIONES_DIR, pdf_name)

    if generate_pdf(topic, article, pdf_path):
        print(f"[content-writer] PDF guardado: {pdf_path}")
        print(f"[content-writer] Palabras: ~{len(article.split())}")
        return 0
    else:
        print("[content-writer] ERROR: no se pudo generar el PDF")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Glifo content-writer: prepara el prompt, copia al portapapeles, abre la IA en el navegador.
   La interaccion con la IA la hace el usuario como una persona.
   El siguiente glifo (clipboard-capturer) lee el portapapeles y guarda el PDF."""

import json, os, sys, webbrowser, subprocess
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
THEME_FILE = os.path.join(PANAL_DIR, "tema_seleccionado.json")
OUTPUT_FILE = os.path.join(PANAL_DIR, "respuesta_ia.txt")
AI_URL = os.environ.get("BDGB_AI_WEB_URL", "https://chat.openai.com")


def build_prompt(topic):
    return (
        f"Escribe un articulo detallado y bien estructurado en espanol sobre: {topic}.\n\n"
        f"Debe tener introduccion, 3-4 secciones con subtitulos, y conclusion. "
        f"Extension aproximada: 800-1200 palabras. "
        f"Tono periodistico para una publicacion de divulgacion."
    )


def copy_to_clipboard(text):
    try:
        import pyperclip
        pyperclip.copy(text)
        return True
    except ImportError:
        try:
            if os.name == "nt":
                r = subprocess.run(["powershell", "-command", "$input|Set-Clipboard"],
                                   input=text.encode("utf-8"), capture_output=True, timeout=5)
                return r.returncode == 0
            elif os.name == "posix":
                r = subprocess.run(["xclip", "-selection", "clipboard"],
                                   input=text.encode("utf-8"), capture_output=True, timeout=5)
                return r.returncode == 0
        except: pass
    return False


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


def save_response(topic, text):
    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(f"Tema: {topic}\n")
        f.write(f"Capturado: {datetime.now().isoformat()}\n")
        f.write("=" * 60 + "\n\n")
        f.write(text)
    print(f"[content-writer] Respuesta guardada en: {OUTPUT_FILE}")
    print(f"[content-writer] Palabras: ~{len(text.split())}")


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

    prompt = build_prompt(topic)

    print("\n" + "=" * 60)
    print("  CONTENT-WRITER")
    print("  La IA se usara como herramienta desde el navegador.")
    print("  Ella no sabe que es un glifo ni que hay automatizacion.")
    print("=" * 60)
    print(f"\n  Tema: {topic}")
    print(f"\n  Prompt copiado al portapapeles:")
    print(f"  {prompt[:150]}...")
    print(f"\n  1. Abriendo {AI_URL} en tu navegador...")
    print("  2. PEGA el prompt en la IA (Ctrl+V)")
    print("  3. Espera la respuesta de la IA")
    print("  4. COPIA la respuesta (Ctrl+A, Ctrl+C)")
    print("  5. Vuelve aqui y presiona Enter")
    print("=" * 60)

    copy_to_clipboard(prompt)
    webbrowser.open(AI_URL)

    input("\n  Presiona Enter cuando hayas copiado la respuesta de la IA...")

    response = paste_from_clipboard()
    if not response or len(response) < 50:
        print("\n  Portapapeles vacio. Pega el texto de la IA manualmente y presiona Ctrl+Z luego Enter:")
        try: response = sys.stdin.read()
        except: response = ""

    if response and len(response) > 50:
        save_response(topic, response)
        return 0

    print("[content-writer] No se capturo respuesta. Usa BDGB_AI_MODE=mock para simular")
    return 1


if __name__ == "__main__":
    sys.exit(main())

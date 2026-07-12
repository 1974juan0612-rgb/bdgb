import os
import sys
import glob
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_utils import wait_and_focus, calibrate_click, load_calib
from gui_launch import wait_for_window
import pyautogui

def find_pdf():
    guiones_dir = os.path.expanduser("~/Desktop/guiones")
    if not os.path.isdir(guiones_dir):
        return None
    pdfs = glob.glob(os.path.join(guiones_dir, "*.pdf"))
    if not pdfs:
        return None
    return max(pdfs, key=os.path.getmtime)

def upload_via_dialog(pdf_path):
    """Sube PDF escribiendo la ruta en el dialogo nativo de Windows."""
    time.sleep(2)
    pyautogui.write(os.path.abspath(pdf_path), interval=0.03)
    time.sleep(0.5)
    pyautogui.press('enter')
    time.sleep(3)

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[GUION] MOCK")
        return

    pdf = find_pdf()
    if not pdf:
        print("[GUION] No hay PDF en ~/Desktop/guiones/")
        sys.exit(1)
    print(f"[GUION] PDF: {pdf} ({os.path.getsize(pdf)} bytes)")

    w = wait_for_window(timeout=5)
    if not w:
        print("[GUION] NotebookLM no visible")
        sys.exit(1)
    w.activate()
    time.sleep(1)

    # Click boton de subir PDF
    calib = load_calib()
    if calib and "upload_pdf" in calib:
        ok = calibrate_click("upload_pdf")
        print(f"[GUION] Boton upload: {'ok' if ok else 'fallo'}")
    else:
        print("[GUION] No calibrado: upload_pdf")
        sys.exit(1)

    # Escribir ruta en el dialogo nativo
    time.sleep(2)
    pyautogui.write(os.path.abspath(pdf), interval=0.03)
    time.sleep(0.5)
    pyautogui.press('enter')
    time.sleep(3)

    print("[GUION] PDF subido. Esperando procesamiento (30s)...")
    time.sleep(30)
    print("[GUION] Siguiente: notebook-video")

if __name__ == "__main__":
    main()

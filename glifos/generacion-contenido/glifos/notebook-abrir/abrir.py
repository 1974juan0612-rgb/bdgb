import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_launch import launch, wait_for_window, wait_for_login_window
import pyautogui

GOOGLE_EMAIL = "bosoncronon@gmail.com"
GOOGLE_PASS = os.environ.get("GOOGLE_PASS", "")

def do_login():
    print("[ABRIR] Ventana de login detectada, auto-login...")
    time.sleep(2)

    # Email
    pyautogui.write(GOOGLE_EMAIL, interval=0.05)
    time.sleep(0.5)
    pyautogui.press('enter')
    time.sleep(3)

    # Password
    if GOOGLE_PASS:
        pyautogui.write(GOOGLE_PASS, interval=0.05)
        time.sleep(0.5)
        pyautogui.press('enter')
        time.sleep(5)

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[ABRIR] MOCK")
        return

    # Lanzar PWA
    ok = launch()
    if not ok:
        print("[ABRIR] No se pudo lanzar NotebookLM")
        sys.exit(1)

    w = wait_for_window(timeout=10)
    if not w:
        print("[ABRIR] Ventana de NotebookLM no aparecio")
        sys.exit(1)

    w.activate()
    time.sleep(2)

    # Detectar si esta en login
    lw = wait_for_login_window(timeout=3)
    if lw:
        do_login()
        # Re-esperar la ventana principal
        w = wait_for_window(timeout=10)
        if w:
            w.activate()
            time.sleep(2)

    print("[ABRIR] NotebookLM listo. Siguiente: notebook-cuaderno")

if __name__ == "__main__":
    main()

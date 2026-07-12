import os
import sys
import json
import time
import pyautogui
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_launch import wait_for_window

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VISTA_FILE = os.path.join(PANAL_DIR, "pipeline_state", "vista.json")

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[CUADERNO] MOCK")
        return

    if not os.path.isfile(VISTA_FILE):
        print(f"[CUADERNO] No hay vista.json. Ejecuta notebook-vision primero")
        sys.exit(1)

    with open(VISTA_FILE) as f:
        vista = json.load(f)

    pos = vista.get("crear_cuaderno")
    if not pos:
        print("[CUADERNO] vista.json no contiene 'crear_cuaderno'")
        sys.exit(1)

    x, y = pos["x"], pos["y"]
    print(f"[CUADERNO] Click en ({x}, {y}) segun vision...")

    w = wait_for_window(timeout=3)
    if w:
        w.activate()
        time.sleep(1)

    pyautogui.click(x, y)
    print(f"[CUADERNO] Click hecho en ({x}, {y})")
    time.sleep(3)
    print("[CUADERNO] Cuaderno creado. Siguiente: notebook-guion")

if __name__ == "__main__":
    main()

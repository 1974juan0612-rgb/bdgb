import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_utils import wait_and_focus, calibrate_click, load_calib
from gui_launch import wait_for_window

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[VIDEO] MOCK")
        return

    w = wait_for_window(timeout=5)
    if not w:
        print("[VIDEO] NotebookLM no visible")
        sys.exit(1)
    w.activate()
    time.sleep(1)

    calib = load_calib()

    # Click pestaña Studio
    if calib and "studio_tab" in calib:
        ok = calibrate_click("studio_tab")
        print(f"[VIDEO] Studio tab: {'ok' if ok else 'fallo'}")
    else:
        print("[VIDEO] No calibrado: studio_tab")
        sys.exit(1)
    time.sleep(3)

    # Click "Crear video"
    if calib and "crear_video" in calib:
        ok = calibrate_click("crear_video")
        print(f"[VIDEO] Crear video: {'ok' if ok else 'fallo'}")
    else:
        print("[VIDEO] No calibrado: crear_video")
        sys.exit(1)
    time.sleep(2)

    # Click "Crear" en el modal
    if calib and "confirmar_crear" in calib:
        ok = calibrate_click("confirmar_crear")
        print(f"[VIDEO] Confirmar crear: {'ok' if ok else 'fallo'}")
    else:
        print("[VIDEO] No calibrado: confirmar_crear")
        sys.exit(1)

    print("[VIDEO] Video en generacion (~10 min). Siguiente: notebook-descargar")

if __name__ == "__main__":
    main()

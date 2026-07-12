import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_utils import wait_and_focus, calibrate_click, load_calib
from gui_launch import wait_for_window

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[CERRAR] MOCK")
        return

    w = wait_for_window(timeout=5)
    if not w:
        print("[CERRAR] NotebookLM no visible, omite")
        return
    w.activate()
    time.sleep(1)

    calib = load_calib()

    # Click menu opciones (3 puntos)
    if calib and "menu_opciones" in calib:
        ok = calibrate_click("menu_opciones")
        print(f"[CERRAR] Menu opciones: {'ok' if ok else 'fallo'}")
    else:
        print("[CERRAR] No calibrado: menu_opciones")
        sys.exit(1)
    time.sleep(2)

    # Click "Eliminar cuaderno"
    if calib and "eliminar_cuaderno" in calib:
        ok = calibrate_click("eliminar_cuaderno")
        print(f"[CERRAR] Eliminar cuaderno: {'ok' if ok else 'fallo'}")
    else:
        print("[CERRAR] No calibrado: eliminar_cuaderno")
        sys.exit(1)
    time.sleep(2)

    # Click "Eliminar" en dialogo de confirmacion
    if calib and "confirmar_eliminar" in calib:
        ok = calibrate_click("confirmar_eliminar")
        print(f"[CERRAR] Confirmar eliminar: {'ok' if ok else 'fallo'}")
    else:
        print("[CERRAR] No calibrado: confirmar_eliminar")
        sys.exit(1)

    print("[CERRAR] Cuaderno eliminado. Siguiente: youtube-subir")

if __name__ == "__main__":
    main()

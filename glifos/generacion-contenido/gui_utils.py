"""Utilidades PyAutoGUI para automatizar NotebookLM.
Usa reconocimiento de imagenes y coordenadas relativas a la ventana."""

import os
import json
import time
import pyautogui
import pygetwindow as gw

PANAL_DIR = os.path.dirname(os.path.abspath(__file__))
GUIAS_DIR = os.path.join(PANAL_DIR, "guias")
CALIB_FILE = os.path.join(GUIAS_DIR, "notebooklm_calib.json")

pyautogui.FAILSAFE = True
pyautogui.PAUSE = 0.5

def find_window(title_fragment="NotebookLM"):
    """Encuentra la ventana de NotebookLM y retorna (left, top, width, height)."""
    for w in gw.getWindowsWithTitle(title_fragment):
        if w.visible and w.width > 100:
            return (w.left, w.top, w.width, w.height)
    return None

def focus_window(title_fragment="NotebookLM"):
    """Trae la ventana al frente."""
    for w in gw.getWindowsWithTitle(title_fragment):
        if w.visible:
            try:
                w.activate()
            except:
                pass
            return True
    return False

def click_rel(rel_x, rel_y, window_rect=None):
    """Click en coordenadas relativas a la ventana (0-1 range)."""
    if not window_rect:
        window_rect = find_window()
    if not window_rect:
        print("[GUI] Ventana no encontrada")
        return False
    l, t, w, h = window_rect
    abs_x = l + int(w * rel_x)
    abs_y = t + int(h * rel_y)
    pyautogui.click(abs_x, abs_y)
    return True

def click_abs(x, y):
    """Click en coordenadas absolutas."""
    pyautogui.click(x, y)

def save_position(name):
    """Espera 3s y guarda la posicion del mouse para un boton."""
    print(f"[CALIBRA] Mueve el mouse sobre '{name}' y espera...")
    time.sleep(3)
    x, y = pyautogui.position()
    print(f"[CALIBRA] '{name}' guardado en ({x}, {y})")
    return (x, y)

def calibrate():
    """Asistente de calibracion: guarda posiciones de todos los botones."""
    print("=== CALIBRACION NOTEBOOKLM ===")
    print("Abre NotebookLM y colocate en la pagina PRINCIPAL (con los cuadernos).")
    input("Presiona Enter cuando estes listo...")

    btn_names = [
        ("crear_cuaderno", "Boton 'Crear nuevo cuaderno'"),
        ("upload_pdf", "Boton para subir/importar PDF"),
        ("studio_tab", "Pestana 'Studio'"),
        ("crear_video", "Boton 'Crear video'"),
        ("confirmar_crear", "Boton 'Crear' en el modal"),
        ("menu_opciones", "Boton de menu (3 puntos)"),
        ("eliminar_cuaderno", "Opcion 'Eliminar cuaderno'"),
        ("confirmar_eliminar", "Boton 'Eliminar' en dialogo de confirmacion"),
    ]

    calib = {}
    for key, desc in btn_names:
        print(f"\n--- {desc} ---")
        pos = save_position(key)
        calib[key] = {"x": pos[0], "y": pos[1]}

    with open(CALIB_FILE, "w") as f:
        json.dump(calib, f, indent=2)
    print(f"\nCalibracion guardada en {CALIB_FILE}")
    print(f"{len(calib)} posiciones registradas")

def load_calib():
    """Carga posiciones calibradas."""
    if not os.path.isfile(CALIB_FILE):
        return None
    with open(CALIB_FILE, "r") as f:
        return json.load(f)

def calibrate_click(action_name):
    """Hace click en un boton usando calibracion."""
    calib = load_calib()
    if not calib or action_name not in calib:
        print(f"[GUI] '{action_name}' no calibrado. Ejecuta calibrate() primero")
        return False
    pos = calib[action_name]
    pyautogui.click(pos["x"], pos["y"])
    return True

def wait_and_focus(timeout=5):
    """Espera a que la ventana de NotebookLM aparezca y la enfoca."""
    for _ in range(timeout):
        rect = find_window()
        if rect:
            focus_window()
            return rect
        time.sleep(1)
    return None

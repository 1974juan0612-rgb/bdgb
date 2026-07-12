"""Lanza NotebookLM PWA desde el acceso directo del escritorio (sin CDP)."""

import os
import sys
import time
import subprocess

PANAL_DIR = os.path.dirname(os.path.abspath(__file__))

def launch():
    shortcut = os.path.expanduser("~/Desktop/NotebookLM.lnk")
    if not os.path.isfile(shortcut):
        print(f"[GUI-LAUNCH] No existe: {shortcut}")
        return False

    print(f"[GUI-LAUNCH] Abriendo NotebookLM...")
    subprocess.Popen(['cmd', '/c', 'start', '', shortcut], shell=True)
    time.sleep(5)
    print(f"[GUI-LAUNCH] NotebookLM lanzado")
    return True

def focus():
    """Enfoca la ventana de NotebookLM."""
    import pygetwindow as gw
    for w in gw.getWindowsWithTitle("NotebookLM"):
        if w.visible:
            try:
                w.activate()
            except:
                pass
            return w
    return None

def wait_for_window(timeout=10):
    """Espera a que aparezca la ventana de NotebookLM."""
    import pygetwindow as gw
    for _ in range(timeout):
        for w in gw.getWindowsWithTitle("NotebookLM"):
            if w.visible and w.width > 100:
                return w
        time.sleep(1)
    return None

def wait_for_login_window(timeout=5):
    import pygetwindow as gw
    for _ in range(timeout):
        for w in gw.getWindowsWithTitle("Iniciar sesion"):
            if w.visible:
                return w
        for w in gw.getWindowsWithTitle("Sign in"):
            if w.visible:
                return w
        time.sleep(1)
    return None

if __name__ == "__main__":
    launch()
    w = wait_for_window()
    if w:
        print(f"Ventana encontrada: {w.title} ({w.width}x{w.height} en {w.left},{w.top})")
    else:
        print("Ventana no encontrada")

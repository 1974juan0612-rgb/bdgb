#!/usr/bin/env python3
"""Lector de coordenadas de pantalla con overlay transparente.
Uso: python reader.py
- Overlay fullscreen semi-transparente con cuadricula
- Clicke-through: los clicks pasan a la aplicacion debajo
- Click registra coordenadas + abre input para tiempo de espera
- Guarda en pipeline_state/coordenadas.json
- 'q' para salir y guardar"""

import os
import sys
import json
import time
import tkinter as tk
from tkinter import simpledialog
import pyautogui
from pynput import mouse
import ctypes
from ctypes import wintypes

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
OUTPUT_FILE = os.path.join(STATE_DIR, "coordenadas.json")

# Variables globales
puntos = []
pausado = False

# ====== WINDOWS API: click-through ======
WS_EX_TRANSPARENT = 0x00000020
WS_EX_LAYERED = 0x00080000
WS_EX_TOOLWINDOW = 0x00000080
GWL_EXSTYLE = -20

user32 = ctypes.windll.user32
SetWindowLong = user32.SetWindowLongW
GetWindowLong = user32.GetWindowLongW

def make_clickthrough(hwnd):
    ex_style = GetWindowLong(hwnd, GWL_EXSTYLE)
    SetWindowLong(hwnd, GWL_EXSTYLE, ex_style | WS_EX_TRANSPARENT | WS_EX_LAYERED)

def make_normal(hwnd):
    ex_style = GetWindowLong(hwnd, GWL_EXSTYLE)
    SetWindowLong(hwnd, GWL_EXSTYLE, ex_style & ~WS_EX_TRANSPARENT)

# ====== PNPPUT: detector global de clicks ======
def on_click(x, y, button, pressed):
    global pausado
    if not pressed or pausado:
        return
    if button == mouse.Button.left:
        pausado = True
        root.after(0, lambda: registrar_coordenada(x, y))

# ====== TKINTER: overlay ======
root = tk.Tk()
root.title("Coordenadas - Click para registrar")
root.attributes('-fullscreen', True)
root.attributes('-topmost', True)
root.attributes('-alpha', 0.25)
root.configure(bg='black')

# Hacer click-through
root.update_idletasks()
hwnd = ctypes.windll.user32.GetParent(root.winfo_id())
make_clickthrough(hwnd)

# Canvas
sw = root.winfo_screenwidth()
sh = root.winfo_screenheight()
canvas = tk.Canvas(root, highlightthickness=0, bg='black')
canvas.pack(fill=tk.BOTH, expand=True)

# Cuadricula
for x in range(0, sw, 100):
    canvas.create_line(x, 0, x, sh, fill='#333333')
    canvas.create_text(x+3, 3, text=str(x), fill='#666666',
                       font=('Consolas', 8), anchor='nw')
for y in range(0, sh, 100):
    canvas.create_line(0, y, sw, y, fill='#333333')
    canvas.create_text(3, y+3, text=str(y), fill='#666666',
                       font=('Consolas', 8), anchor='nw')

# Info
info = canvas.create_text(sw//2, 30, text="",
    fill='#00ff00', font=('Consolas', 20, 'bold'))

mouse_coords = canvas.create_text(sw//2, 60, text="X: 0  Y: 0",
    fill='#ffff00', font=('Consolas', 16, 'bold'))

# Puntos registrados
puntos_canvas = []

def actualizar_mouse():
    if not pausado:
        x, y = pyautogui.position()
        canvas.itemconfig(mouse_coords, text=f"X: {x}  Y: {y}")
    root.after(100, actualizar_mouse)

def actualizar_info():
    texto = "Click IZQUIERDO en pantalla para registrar coordenada"
    if pausado:
        texto = "Esperando entrada de tiempo..."
    canvas.itemconfig(info, text=texto)
    root.after(500, actualizar_info)

def registrar_coordenada(x, y):
    global pausado
    # Pedir tiempo de espera
    root.attributes('-alpha', 0.5)
    make_normal(hwnd)
    root.focus_force()

    tiempo = simpledialog.askstring(
        "Coordenada",
        f"Coordenada ({x}, {y})\nTiempo de espera (segundos) o nombre:",
        parent=root)

    if tiempo is None:
        tiempo = ""

    punto = {
        "x": x, "y": y,
        "timeout": tiempo,
        "timestamp": time.strftime("%H:%M:%S")
    }
    puntos.append(punto)

    # Dibujar en overlay
    r = 8
    cid = canvas.create_oval(x-r, y-r, x+r, y+r,
        outline='red', width=3, fill='#ff000044')
    tid = canvas.create_text(x, y-18, text=f"#{len(puntos)} ({x},{y}) t:{tiempo}",
        fill='#ff0000', font=('Consolas', 10, 'bold'))
    puntos_canvas.append((cid, tid))

    print(f"  #{len(puntos)}: ({x},{y}) t:{tiempo}")

    # Restaurar overlay
    make_clickthrough(hwnd)
    root.attributes('-alpha', 0.25)
    pausado = False

def guardar_y_salir(event=None):
    os.makedirs(STATE_DIR, exist_ok=True)
    with open(OUTPUT_FILE, "w") as f:
        json.dump({"puntos": puntos, "screen": {"w": sw, "h": sh}}, f, indent=2)
    print(f"\n== Guardado en {OUTPUT_FILE} ==")
    print(json.dumps(puntos, indent=2))
    root.destroy()

# Tecla 'q' para salir
root.bind('q', guardar_y_salir)
root.bind('Q', guardar_y_salir)

def main():
    # Iniciar listener global de clicks
    listener = mouse.Listener(on_click=on_click)
    listener.start()

    actualizar_mouse()
    actualizar_info()

    print(f"== LECTOR DE COORDENADAS ==")
    print(f"Click izquierdo en cualquier lugar -> registra coordenada + pide tiempo")
    print(f"Presiona 'q' para guardar y salir")
    print(f"Sobrescribira: {OUTPUT_FILE}")

    root.mainloop()
    listener.stop()

if __name__ == "__main__":
    main()

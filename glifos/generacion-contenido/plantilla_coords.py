"""Plantilla transparente de coordenadas de pantalla.
Muestra una cuadricula superpuesta y coordenadas del mouse en tiempo real.
Haz click sobre cada boton de NotebookLM para registrar su posicion.
"""

import tkinter as tk
import pyautogui
import json
import os
import sys

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(PANAL_DIR, "glifos", "guias", "coordenadas.json")

class CoordOverlay:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Coordenadas NotebookLM")
        self.root.attributes('-topmost', True)
        self.root.attributes('-alpha', 0.3)  # 30% opacidad - semi-transparente
        self.root.attributes('-fullscreen', True)
        self.root.config(cursor='crosshair')

        self.puntos = {}
        self.step = 0
        self.labels_paso = [
            "crear_cuaderno",
            "upload_pdf",
            "studio_tab",
            "crear_video",
            "confirmar_crear",
            "menu_opciones",
            "eliminar_cuaderno",
            "confirmar_eliminar",
        ]

        # Canvas para dibujar
        self.canvas = tk.Canvas(self.root, highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        # Hacer canvas transparente (click-through en Windows es complejo,
        # mejor hacemos clics registrando posiciones)
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<Motion>", self.on_motion)

        # Info en esquina
        w = self.root.winfo_screenwidth()
        self.info = self.canvas.create_text(
            10, 10, anchor='nw',
            text="CARGANDO...",
            fill='red', font=('Consolas', 14, 'bold'),
        )

        # Dibujar cuadricula
        self.draw_grid()

        # Mouse coords label
        self.mouse_label = self.canvas.create_text(
            w // 2, 10, anchor='n',
            text="X: 0  Y: 0",
            fill='red', font=('Consolas', 16, 'bold'),
        )

        self.root.after(500, self.show_step)

    def draw_grid(self):
        w = self.root.winfo_screenwidth()
        h = self.root.winfo_screenheight()
        for x in range(0, w, 50):
            self.canvas.create_line(x, 0, x, h, fill='gray', stipple='gray12')
        for y in range(0, h, 50):
            self.canvas.create_line(0, y, w, y, fill='gray', stipple='gray12')

    def on_motion(self, event):
        self.canvas.itemconfig(self.mouse_label,
            text=f"X:{event.x}  Y:{event.y}")

    def show_step(self):
        if self.step < len(self.labels_paso):
            nombre = self.labels_paso[self.step]
            self.canvas.itemconfig(self.info,
                text=f"Paso {self.step+1}/{len(self.labels_paso)}: "
                     f"Haz click en '{nombre}'")
        else:
            self.save_and_exit()

    def on_click(self, event):
        if self.step >= len(self.labels_paso):
            return

        nombre = self.labels_paso[self.step]
        x, y = event.x, event.y

        # Marcar con un circulo
        r = 5
        self.canvas.create_oval(x-r, y-r, x+r, y+r,
            outline='red', width=3, fill='yellow')
        self.canvas.create_text(x, y-15, text=nombre,
            fill='red', font=('Consolas', 10, 'bold'))

        self.puntos[nombre] = {"x": x, "y": y}
        print(f"  {nombre}: ({x}, {y})")

        self.step += 1
        self.root.after(300, self.show_step)

    def save_and_exit(self):
        print(f"\nCoordenadas guardadas en: {OUTPUT}")
        print(json.dumps(self.puntos, indent=2))
        with open(OUTPUT, "w") as f:
            json.dump(self.puntos, f, indent=2)
        self.root.destroy()

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    app = CoordOverlay()
    app.run()

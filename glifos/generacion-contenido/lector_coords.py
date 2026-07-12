"""Muestra coordenadas del mouse en tiempo real.
Abre NotebookLM, mueve el mouse sobre cada boton y lee las coordenadas."""

import tkinter as tk
import pyautogui

root = tk.Tk()
root.title("Coords")
root.attributes('-topmost', True)
root.geometry("300x100+5+5")
root.configure(bg='black')

frame = tk.Frame(root, bg='black')
frame.pack(expand=True, fill=tk.BOTH)

tk.Label(frame, text="Mueve el mouse sobre los botones de NotebookLM",
    fg='gray', bg='black', font=('Consolas', 9)).pack()

label = tk.Label(frame, text="X: 0  Y: 0",
    fg='lime', bg='black', font=('Consolas', 24, 'bold'))
label.pack(pady=5)

def update():
    x, y = pyautogui.position()
    label.config(text=f"X: {x}  Y: {y}")
    root.after(50, update)

update()
root.mainloop()

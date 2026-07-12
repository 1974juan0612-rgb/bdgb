"""Diagnostico completo de la ventana de NotebookLM.
Toma screenshot, analiza colores, guarda imagen para inspeccion visual."""

import os
import sys
import json
import pyautogui
import numpy as np
from PIL import Image
from datetime import datetime

DESKTOP = os.path.expanduser("~/Desktop")
DIAG_DIR = os.path.join(DESKTOP, "notebooklm_diag")
os.makedirs(DIAG_DIR, exist_ok=True)
ts = datetime.now().strftime("%H%M%S")

# Encontrar ventana
import pygetwindow as gw
target = None
for w in gw.getWindowsWithTitle("NotebookLM"):
    if w.visible and w.width > 100:
        target = w
        break

if not target:
    print("ERROR: NotebookLM no visible. Abrelo primero.")
    sys.exit(1)

target.activate()
import time; time.sleep(1)

print(f"Ventana: ({target.left},{target.top}) {target.width}x{target.height}")

# Screenshot de la ventana
screenshot = pyautogui.screenshot(region=(
    target.left, target.top, target.width, target.height
))
path_png = os.path.join(DIAG_DIR, f"notebooklm_{ts}.png")
screenshot.save(path_png)
print(f"Screenshot guardado: {path_png}")

# Analisis de color
img = np.array(screenshot)
print(f"\nDimensiones: {img.shape[1]}x{img.shape[0]}")
print(f"Canales: {img.shape[2]}")

# Grid de 3x3 - color promedio de cada celda
h, w = img.shape[:2]
print("\n--- Grid 3x3 (color promedio R,G,B por celda) ---")
for row in range(3):
    for col in range(3):
        y1 = int(h * row / 3)
        y2 = int(h * (row + 1) / 3)
        x1 = int(w * col / 3)
        x2 = int(w * (col + 1) / 3)
        region = img[y1:y2, x1:x2]
        avg = region.mean(axis=(0,1)).astype(int)
        print(f"  [{row},{col}] ({x1},{y1})-({x2},{y2}): RGB({avg[0]},{avg[1]},{avg[2]})")

# Distribucion de grises
gray = np.mean(img, axis=2)
print(f"\nGrises - min: {gray.min()}, max: {gray.max()}, media: {gray.mean():.1f}")
for threshold in [30, 50, 80, 100, 150, 200]:
    pct = (gray < threshold).sum() / gray.size * 100
    print(f"  Pixeles < {threshold}: {pct:.1f}%")

# Buscar pixeles oscuros (valor < 80)
dark_mask = gray < 80
dark_rows = np.any(dark_mask, axis=1)
dark_cols = np.any(dark_mask, axis=0)
band_count = 0
if dark_rows.any():
    row_indices = np.where(dark_rows)[0]
    start = row_indices[0]
    prev = row_indices[0]
    for r in row_indices[1:]:
        if r - prev > 5:
            band_h = prev - start
            if band_h > 15:
                band_count += 1
                print(f"\nBanda oscura #{band_count}: filas {start}-{prev} ({band_h}px)")
                # Columnas en esta banda
                col_mask = np.any(dark_mask[start:prev+1, :], axis=0)
                if col_mask.any():
                    ci = np.where(col_mask)[0]
                    left = ci[0]; right = ci[-1]
                    print(f"  Columnas: {left}-{right} ({right-left}px)")
                    region = img[start:prev+1, left:right+1]
                    avg_c = region.mean(axis=(0,1)).astype(int)
                    print(f"  Color prom: RGB({avg_c[0]},{avg_c[1]},{avg_c[2]})")
                    # Texto blanco?
                    white = (region[:,:,0] > 230) & (region[:,:,1] > 230) & (region[:,:,2] > 230)
                    wr = white.sum() / region.size * 100
                    print(f"  Texto blanco: {wr:.2f}%")
            start = r
        prev = r
    band_h = prev - start
    if band_h > 15:
        band_count += 1
        print(f"\nBanda oscura #{band_count}: filas {start}-{prev} ({band_h}px)")
        col_mask = np.any(dark_mask[start:prev+1, :], axis=0)
        if col_mask.any():
            ci = np.where(col_mask)[0]
            left = ci[0]; right = ci[-1]
            print(f"  Columnas: {left}-{right} ({right-left}px)")
            region = img[start:prev+1, left:right+1]
            avg_c = region.mean(axis=(0,1)).astype(int)
            print(f"  Color prom: RGB({avg_c[0]},{avg_c[1]},{avg_c[2]})")
            white = (region[:,:,0] > 230) & (region[:,:,1] > 230) & (region[:,:,2] > 230)
            wr = white.sum() / region.size * 100
            print(f"  Texto blanco: {wr:.2f}%")

if band_count == 0:
    print("\nNo se encontraron bandas oscuras significativas.")

print(f"\nDiagnostico guardado en: {DIAG_DIR}")
print(f"Abre {path_png} y describeme que ves.")

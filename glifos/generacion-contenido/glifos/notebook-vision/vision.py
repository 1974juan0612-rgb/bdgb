"""Glifo notebook-vision: busca el boton 'Crear nuevo cuaderno'.
Esta en la zona SUPERIOR IZQUIERDA de la ventana de NotebookLM.
Boton ancho, fondo oscuro (RGB ~12,12,12), texto blanco con '+'."""

import os
import sys
import json
import time
import pyautogui
import numpy as np

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
VISTA_FILE = os.path.join(STATE_DIR, "vista.json")

def find_plus_button(img_rgb):
    """Busca un boton oscuro con texto blanco en la zona superior.
    Retorna (cx, cy) en coordenadas de la imagen o None."""
    h, w = img_rgb.shape[:2]

    # Solo escanear la zona superior (primer 25% de la ventana)
    scan_h = int(h * 0.25)
    top_region = img_rgb[:scan_h, :]
    gray = np.mean(top_region, axis=2)

    # Umbral generoso para pixeles oscuros
    dark_mask = gray < 100

    # Encontrar la banda oscura mas grande por filas
    dark_rows = np.any(dark_mask, axis=1)
    if not dark_rows.any():
        return None

    row_idx = np.where(dark_rows)[0]

    # Agrupar filas consecutivas, quedarse con la banda mas grande
    bands = []
    start = row_idx[0]
    prev = row_idx[0]
    for r in row_idx[1:]:
        if r - prev > 3:
            bands.append((int(start), int(prev), int(prev - start)))
            start = r
        prev = r
    bands.append((int(start), int(prev), int(prev - start)))
    bands.sort(key=lambda x: -x[2])

    for top, bottom, bh in bands:
        if bh < 15:
            continue

        # Dentro de esta banda, agrupar columnas oscuras
        band_mask = dark_mask[top:bottom+1, :]
        # Buscar grupos de columnas
        dark_cols = np.any(band_mask, axis=0)
        col_idx = np.where(dark_cols)[0]
        if len(col_idx) < 30:
            continue

        # Agrupar columnas
        c_bands = []
        c_start = col_idx[0]
        c_prev = col_idx[0]
        for c in col_idx[1:]:
            if c - c_prev > 5:
                c_bands.append((int(c_start), int(c_prev), int(c_prev - c_start)))
                c_start = c
            c_prev = c
        c_bands.append((int(c_start), int(c_prev), int(c_prev - c_start)))

        # Cada grupo de columnas es un posible boton
        for left, right, cw in c_bands:
            if cw < 50 or cw > w * 0.6:  # ni muy angosto ni muy ancho
                continue

            # Verificar que tenga texto blanco
            region = top_region[top:bottom+1, left:right+1]
            if region.size == 0:
                continue
            white = (region[:,:,0] > 220) & (region[:,:,1] > 220) & (region[:,:,2] > 220)
            wr = white.sum() / region.size

            if wr > 0.005:
                cx = (left + right) // 2
                cy = top + (bottom - top) // 2
                return (cx, cy, cw, bh, wr)

    return None

def main():
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[VISION] MOCK")
        os.makedirs(STATE_DIR, exist_ok=True)
        with open(VISTA_FILE, "w") as f:
            json.dump({"crear_cuaderno": {"x": 200, "y": 100}, "metodo": "mock"}, f, indent=2)
        return

    os.makedirs(STATE_DIR, exist_ok=True)

    import pygetwindow as gw
    target = None
    for w in gw.getWindowsWithTitle("NotebookLM"):
        if w.visible and w.width > 100:
            target = w
            break
    if not target:
        print("[VISION] NotebookLM no visible")
        sys.exit(1)

    target.activate()
    time.sleep(1)

    print(f"[VISION] Ventana: ({target.left},{target.top}) {target.width}x{target.height}")

    # Capturar ventana
    screenshot = pyautogui.screenshot(region=(
        target.left, target.top, target.width, target.height
    ))
    img = np.array(screenshot)
    h, w = img.shape[:2]

    print(f"[VISION] Buscando boton '+' en zona superior...")
    result = find_plus_button(img)

    if result is None:
        cx, cy, cw = w // 2, 50, 200  # fallback posicion tipica
        print(f"[VISION] No detectado automaticamente. Usando posicion estimada...")
        # Diagnostico rapido
        gray = np.mean(img[:int(h*0.25), :], axis=2)
        dark_pct = (gray < 100).sum() / gray.size * 100
        print(f"[VISION] Pixeles oscuros en zona superior: {dark_pct:.1f}%")
        if dark_pct < 1:
            print("[VISION] MUY POCOS pixeles oscuros - el boton no es oscuro?")
        sys.exit(1)

    cx, cy, bw, bh, wr = result
    abs_x = cx + target.left
    abs_y = cy + target.top

    vista = {
        "crear_cuaderno": {"x": abs_x, "y": abs_y},
        "boton": {"w": bw, "h": bh, "white_ratio": wr},
        "ventana": {"left": target.left, "top": target.top, "width": w, "height": h},
        "metodo": "vision-topbar"
    }

    with open(VISTA_FILE, "w") as f:
        json.dump(vista, f, indent=2)

    print(f"[VISION] Boton encontrado en zona superior: ({abs_x},{abs_y}) {bw}x{bh}")
    print(f"[VISION] Texto blanco: {wr*100:.2f}%")
    print(f"[VISION] Coordenadas guardadas")
    print(f"[VISION] Siguiente: notebook-cuaderno")

if __name__ == "__main__":
    main()

import os
import sys
import json
import time
import subprocess
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from gui_launch import wait_for_window

STATE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "pipeline_state")
YT_URL = "https://studio.youtube.com"

def main():
    video_path = os.path.join(STATE_DIR, "video.mp4")
    if not os.path.isfile(video_path):
        print("[YT-SUBIR] No hay video.mp4")
        sys.exit(1)

    meta_path = os.path.join(STATE_DIR, "guion_meta.json")
    titulo = "Video BDGB"
    if os.path.isfile(meta_path):
        with open(meta_path, "r", encoding="utf-8") as f:
            meta = json.load(f)
            titulo = meta.get("titulo", titulo)

    print(f"[YT-SUBIR] Video: {video_path} ({os.path.getsize(video_path)} bytes)")
    print(f"[YT-SUBIR] Titulo: {titulo}")

    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[YT-SUBIR] MOCK")
        json.dump({"titulo": titulo, "url": "https://youtube.com/watch?v=MOCK", "estado": "mock"},
                  open(os.path.join(STATE_DIR, "publicado.json"), "w"))
        return

    # Abrir YouTube Studio en el navegador predeterminado
    print("[YT-SUBIR] Abriendo YouTube Studio...")
    subprocess.Popen(['cmd', '/c', 'start', '', YT_URL], shell=True)
    time.sleep(5)

    print(f"[YT-SUBIR] YouTube Studio abierto en navegador predeterminado")
    print(f"[YT-SUBIR] Sube el archivo manualmente desde la interfaz de YouTube Studio")
    print(f"[YT-SUBIR] Archivo: {video_path}")
    input("[YT-SUBIR] Presiona Enter cuando el video este PUBLICADO...")

    json.dump({"titulo": titulo, "url": YT_URL, "estado": "publicado", "metodo": "manual"},
              open(os.path.join(STATE_DIR, "publicado.json"), "w"))
    print("[YT-SUBIR] Video publicado!")

if __name__ == "__main__":
    main()

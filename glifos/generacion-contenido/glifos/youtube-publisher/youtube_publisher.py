import os
import sys
import json
import time

STATE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "pipeline_state")
YT_STATE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "remote_state")
USER_DATA_DIR = os.path.join(YT_STATE_DIR, "chrome_data")
CDP_PORT = 9444
YT_URL = "https://studio.youtube.com"

def main():
    os.makedirs(STATE_DIR, exist_ok=True)
    os.makedirs(YT_STATE_DIR, exist_ok=True)

    video_path = os.path.join(STATE_DIR, "video.mp4")
    if not os.path.isfile(video_path):
        print("[YT-PUBLISHER] No hay video.mp4 en pipeline_state")
        sys.exit(1)

    meta_path = os.path.join(STATE_DIR, "guion_meta.json")
    titulo = "Video generado por BDGB"
    if os.path.isfile(meta_path):
        with open(meta_path, "r", encoding="utf-8") as f:
            meta = json.load(f)
            titulo = meta.get("titulo", titulo)
    print(f"[YT-PUBLISHER] Video: {video_path} ({os.path.getsize(video_path)} bytes)")
    print(f"[YT-PUBLISHER] Titulo: {titulo}")

    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[YT-PUBLISHER] MOCK: publicacion simulada")
        pub_path = os.path.join(STATE_DIR, "publicado.json")
        with open(pub_path, "w") as f:
            json.dump({"titulo": titulo, "url": "https://youtube.com/watch?v=MOCK", "estado": "mock"}, f)
        print(f"[YT-PUBLISHER] Mock publicado: {pub_path}")
        return

    import subprocess
    import urllib.request
    import websocket

    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True, timeout=5)
    except: pass
    time.sleep(1)

    chrome_paths = [
        "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        os.path.expandvars("%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe")
    ]
    chrome_exe = None
    for p in chrome_paths:
        if os.path.isfile(p):
            chrome_exe = p
            break
    if not chrome_exe:
        print("[YT-PUBLISHER] Chrome no encontrado")
        sys.exit(1)

    cmd = f'"{chrome_exe}" --remote-debugging-port={CDP_PORT} --remote-allow-origins=* --user-data-dir="{USER_DATA_DIR}" --disable-blink-features=AutomationControlled --no-first-run --no-default-browser-check "{YT_URL}"'
    subprocess.Popen(cmd, shell=True)
    time.sleep(4)

    try:
        resp = urllib.request.urlopen(f"http://localhost:{CDP_PORT}/json/version")
        info = json.loads(resp.read())
        ws_url = info.get("webSocketDebuggerUrl")
        if not ws_url:
            print("[YT-PUBLISHER] No se obtuvo WebSocket URL")
            sys.exit(1)
    except Exception as e:
        print(f"[YT-PUBLISHER] Error CDP: {e}")
        sys.exit(1)

    ws = websocket.create_connection(ws_url, timeout=30)
    ws.send(json.dumps({"id": 1, "method": "Page.enable", "params": {}}))
    ws.recv()
    ws.send(json.dumps({"id": 2, "method": "Runtime.enable", "params": {}}))
    ws.recv()

    print("[YT-PUBLISHER] === YouTube Studio abierto en Chrome ===")
    print("Pasos manuales requeridos:")
    print(f"1. Seleccionar canal 'glifos' (si pide elegir)")
    print("2. Click 'Subir videos' (boton arriba a la derecha)")
    print(f"3. Seleccionar archivo: {video_path}")
    print("4. Llenar titulo, descripcion, configurar")
    print("5. Click 'Publicar' o 'Programar'")
    print("========================")
    print("Cuando el video este publicado, presiona Enter para continuar...")
    input()

    pub_path = os.path.join(STATE_DIR, "publicado.json")
    with open(pub_path, "w") as f:
        json.dump({"titulo": titulo, "estado": "publicado", "metodo": "semi-automatico"}, f, indent=2)
    print(f"[YT-PUBLISHER] Confirmacion guardada: {pub_path}")

    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True, timeout=5)
    except: pass

    print("[YT-PUBLISHER] Listo")

if __name__ == "__main__":
    main()

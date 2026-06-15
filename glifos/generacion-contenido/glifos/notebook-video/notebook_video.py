import os
import sys
import json
import time

STATE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "pipeline_state")
NOTESTATE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "remote_state")
USER_DATA_DIR = os.path.join(NOTESTATE_DIR, "chrome_data")
CDP_PORT = 9333
NOTEBOOK_URL = os.environ.get("BDGB_NOTEBOOK_URL", "https://notebooklm.google.com")

def main():
    os.makedirs(STATE_DIR, exist_ok=True)
    os.makedirs(NOTESTATE_DIR, exist_ok=True)

    # leer guion
    guion_path = os.path.join(STATE_DIR, "guion.txt")
    if not os.path.isfile(guion_path):
        print("[NOTEBOOK-VIDEO] No hay guion.txt en pipeline_state")
        sys.exit(1)
    with open(guion_path, "r", encoding="utf-8") as f:
        guion = f.read()
    print(f"[NOTEBOOK-VIDEO] Guion cargado: {len(guion)} chars")

    # modo mock
    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[NOTEBOOK-VIDEO] MOCK: video simulado")
        mock_path = os.path.join(STATE_DIR, "video.mp4")
        with open(mock_path, "w") as f:
            f.write("MOCK VIDEO")
        print(f"[NOTEBOOK-VIDEO] Mock guardado: {mock_path}")
        return

    import subprocess
    import websocket
    import socket
    import threading
    import urllib.request

    # matar Chrome previo en este puerto
    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True, timeout=5)
    except: pass
    time.sleep(1)

    # lanzar Chrome con CDP
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
        print("[NOTEBOOK-VIDEO] Chrome no encontrado")
        sys.exit(1)

    cmd = f'"{chrome_exe}" --remote-debugging-port={CDP_PORT} --remote-allow-origins=* --user-data-dir="{USER_DATA_DIR}" --disable-blink-features=AutomationControlled --no-first-run --no-default-browser-check'
    subprocess.Popen(cmd, shell=True)
    time.sleep(3)

    # conectar WebSocket
    try:
        resp = urllib.request.urlopen(f"http://localhost:{CDP_PORT}/json/version")
        info = json.loads(resp.read())
        ws_url = info.get("webSocketDebuggerUrl")
        if not ws_url:
            print("[NOTEBOOK-VIDEO] No se obtuvo WebSocket URL")
            sys.exit(1)
        print(f"[NOTEBOOK-VIDEO] Conectado: {ws_url}")
    except Exception as e:
        print(f"[NOTEBOOK-VIDEO] Error conectando CDP: {e}")
        sys.exit(1)

    ws = websocket.create_connection(ws_url, timeout=30)

    def send_cmd(method, params=None):
        if params is None:
            params = {}
        msg = json.dumps({"id": 1, "method": method, "params": params})
        ws.send(msg)
        resp = json.loads(ws.recv())
        return resp

    # navegar a NotebookLM
    send_cmd("Page.enable")
    send_cmd("Runtime.enable")
    send_cmd("Page.navigate", {"url": NOTEBOOK_URL})
    time.sleep(5)

    js_commands = [
        # esperar carga
        "new Promise(r => setTimeout(r, 3000))",
        # buscar selector para nuevo notebook o subir fuente
        # Nota: los selectores de NotebookLM cambian frecuentemente
        # Esta es una plantilla que debe adaptarse a la UI actual
        'console.log("NotebookLM CDP: navegando para crear video")',
        # leer guion del DOM o preparar para arrastrar
        "document.title"
    ]

    for js in js_commands:
        msg = json.dumps({"id": 2, "method": "Runtime.evaluate",
            "params": {"expression": js, "awaitPromise": True}})
        ws.send(msg)
        time.sleep(1)

    print("[NOTEBOOK-VIDEO] === IMPORTANTE ===")
    print("NotebookLM requiere interaccion humana para:")
    print("1. Crear nuevo cuaderno")
    print("2. Subir guion.txt como fuente")
    print("3. Ir a 'Crear video'")
    print("4. Esperar generacion")
    print("5. Descargar video a ~/Desktop/videos/")
    print("========================")
    print("El CDP ha abierto NotebookLM en Chrome.")
    print("Cuando el video este descargado, presiona Enter para continuar...")
    input()

    # buscar el video descargado mas reciente
    videos_dir = os.path.expanduser("~/Desktop/videos")
    os.makedirs(videos_dir, exist_ok=True)
    import glob
    videos = glob.glob(os.path.join(videos_dir, "*"))
    if videos:
        latest = max(videos, key=os.path.getmtime)
        dest = os.path.join(STATE_DIR, "video.mp4")
        import shutil
        shutil.copy2(latest, dest)
        print(f"[NOTEBOOK-VIDEO] Video copiado: {dest}")

    # cerrar Chrome
    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True, timeout=5)
    except: pass

    print("[NOTEBOOK-VIDEO] Listo")

if __name__ == "__main__":
    main()

import os
import sys
import json
import time
import uuid
import subprocess
import urllib.request
import tempfile

PANAL_DIR = os.path.dirname(os.path.abspath(__file__))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
USER_DATA_DIR = os.path.join(PANAL_DIR, "chrome_data")
CDP_PORT = 9333
NOTEBOOK_PORT = 9444
GOOGLE_EMAIL = "1974juan0612@gmail.com"
GOOGLE_PASS = os.environ.get("GOOGLE_PASS", "")
NOTEBOOK_APP_ID = "gjcmcplpgihbecacndmmbaenpfgimlec"
CHROME_PROFILE = "Profile 2"

def get_page_ws(port):
    try:
        resp = urllib.request.urlopen(f"http://localhost:{port}/json/list", timeout=5)
        targets = json.loads(resp.read())
        for t in targets:
            if t.get("type") in ("page", "app"):
                return t.get("webSocketDebuggerUrl")
        if targets:
            return targets[0].get("webSocketDebuggerUrl")
    except:
        pass
    return None

def connect(port=CDP_PORT):
    import websocket
    ws_url = get_page_ws(port)
    if not ws_url:
        return None
    ws = websocket.create_connection(ws_url, timeout=30)
    return ws

def cmd(ws, method, params=None):
    if params is None:
        params = {}
    mid = int(uuid.uuid4().int % 1000000)
    ws.send(json.dumps({"id": mid, "method": method, "params": params}))
    while True:
        raw = ws.recv()
        resp = json.loads(raw)
        if resp.get("id") == mid:
            return resp

def launch(port=CDP_PORT, url=""):
    kill_chrome()
    time.sleep(1)
    chrome_exe = _find_chrome()
    if not chrome_exe:
        return None
    cmd_line = f'"{chrome_exe}" --remote-debugging-port={port} --remote-allow-origins=* --user-data-dir="{USER_DATA_DIR}" --no-first-run --no-default-browser-check'
    if url:
        cmd_line += f' "{url}"'
    subprocess.Popen(cmd_line, shell=True)
    time.sleep(5)
    return connect(port)

def launch_pwa(app_id=NOTEBOOK_APP_ID, profile=CHROME_PROFILE, port=NOTEBOOK_PORT):
    """Lanza NotebookLM con CDP usando chrome_data/ del panal como user-data-dir persistente.
    NO es el default de Chrome (que bloquea CDP), asi que CDP funciona.
    La sesion de Google se guarda ahi para siempre."""
    kill_chrome()
    time.sleep(1)
    chrome_exe = _find_chrome()
    if not chrome_exe:
        return None
    os.makedirs(USER_DATA_DIR, exist_ok=True)
    url = "https://notebooklm.google.com"
    cmd_line = f'"{chrome_exe}" --remote-debugging-port={port} --remote-allow-origins=* --user-data-dir="{USER_DATA_DIR}" --no-first-run --no-default-browser-check --window-size=1280,800 "{url}"'
    subprocess.Popen(cmd_line, shell=True)
    time.sleep(6)
    ws = connect(port)
    json.dump({"port": port}, open(os.path.join(PANAL_DIR, "chrome_cdp.json"), "w"))
    return ws

def _find_chrome():
    paths = [
        "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        os.path.expandvars("%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe")
    ]
    for p in paths:
        if os.path.isfile(p):
            return p
    return None

def navigate_and_reconnect(ws, port, url):
    """Navega a URL, reconecta WebSocket (la navegacion cambia el contexto)."""
    cmd(ws, "Page.enable")
    cmd(ws, "Runtime.enable")
    cmd(ws, "Page.navigate", {"url": url})
    time.sleep(4)
    ws.close()
    time.sleep(1)
    return connect(port)

def js_val(ws, code):
    """Evalua JS y retorna el value del resultado."""
    r = cmd(ws, "Runtime.evaluate", {"expression": code})
    return r.get("result", {}).get("result", {}).get("value", "")

def kill_chrome():
    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True, timeout=5)
        subprocess.run(["taskkill", "/F", "/IM", "chrome_proxy.exe"], capture_output=True, timeout=5)
    except:
        pass

def wait_input(msg):
    if sys.stdin.isatty():
        try:
            return input(msg)
        except:
            return ""
    return ""

GUIDE_DIR = os.path.join(PANAL_DIR, "guias")
_guide_cache = {}

def load_guide(app_name):
    """Carga el archivo de guia para una app (notebooklm, youtube, etc)."""
    if app_name in _guide_cache:
        return _guide_cache[app_name]
    path = os.path.join(GUIDE_DIR, f"{app_name}.json")
    if not os.path.isfile(path):
        return None
    with open(path, "r", encoding="utf-8") as f:
        guide = json.load(f)
    _guide_cache[app_name] = guide
    return guide

def click_action(ws, guide, action_name):
    """Usa los selectores de la guia para hacer click en una accion.
    Retorna el nombre del selector que funciono o 'fallo'."""
    action = guide.get("acciones", {}).get(action_name)
    if not action:
        return f"no-action:{action_name}"
    
    # Probar selectores CSS first
    selectors = action.get("selectores", [])
    for sel in selectors:
        js = f"document.querySelector('{sel.replace(chr(39), chr(92)+chr(39))}')?.click() || 'no'"
        r = cmd(ws, "Runtime.evaluate", {"expression": js})
        val = r.get("result", {}).get("result", {}).get("value", "")
        if val and val != "no":
            return f"selector: {sel[:50]}"
    
    # Fallback: js_fallback
    jsf = action.get("js_fallback")
    if jsf:
        r = cmd(ws, "Runtime.evaluate", {"expression": jsf})
        val = r.get("result", {}).get("result", {}).get("value", "")
        if val and "no-" not in val:
            return f"fallback: {val[:40]}"
    
    return "fallo"

def click_first(ws, selectors):
    """Prueba varios selectores JS para hacer click en un elemento."""
    for name, js in selectors:
        r = cmd(ws, "Runtime.evaluate", {"expression": js})
        val = r.get("result", {}).get("result", {}).get("value", "")
        if val and "no-" not in val:
            return f"{name}: {val}"
    return "no-click"

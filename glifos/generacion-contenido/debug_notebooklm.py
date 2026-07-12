"""Conecta a NotebookLM via CDP e inspecciona el DOM para encontrar selectores correctos.
Uso: corre CON NotebookLM abierto (lanzado desde el pipeline o manualmente con CDP).
Si no hay PWA activa, primero lanza una."""

import os, sys, json, time, subprocess, urllib.request

NOTEBOOK_PORT = 9444

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

def main():
    # Intentar conectar primero
    ws_url = get_page_ws(NOTEBOOK_PORT)
    if not ws_url:
        print(f"[DEBUG] No hay Chrome en puerto {NOTEBOOK_PORT}")
        print("[DEBUG] Lanzando NotebookLM PWA con CDP...")
        chrome = "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"
        if not os.path.isfile(chrome):
            chrome = os.path.expandvars("%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe")
        user_data = os.path.expandvars("%LOCALAPPDATA%\\Google\\Chrome\\User Data")
        cmd = f'"{chrome}" --remote-debugging-port={NOTEBOOK_PORT} --remote-allow-origins=* --user-data-dir="{user_data}" --profile-directory="Profile 2" --app-id=gjcmcplpgihbecacndmmbaenpfgimlec'
        subprocess.Popen(cmd, shell=True)
        print("[DEBUG] Esperando 8 segundos...")
        time.sleep(8)
        ws_url = get_page_ws(NOTEBOOK_PORT)
        if not ws_url:
            print("[DEBUG] No se pudo conectar")
            return

    import websocket
    ws = websocket.create_connection(ws_url, timeout=30)

    def c(method, params=None):
        if params is None: params = {}
        mid = int(time.time() * 1000) % 1000000
        ws.send(json.dumps({"id": mid, "method": method, "params": params}))
        while True:
            raw = ws.recv()
            resp = json.loads(raw)
            if resp.get("id") == mid:
                return resp

    c("Page.enable")
    c("Runtime.enable")
    time.sleep(2)

    # 1. Title + URL
    title = c("Runtime.evaluate", {"expression": "document.title"})
    url = c("Runtime.evaluate", {"expression": "window.location.href"})
    print(f"\n=== PAGINA ===")
    print(f"Title: {title.get('result',{}).get('result',{}).get('value','?')}")
    print(f"URL: {url.get('result',{}).get('result',{}).get('value','?')}")

    # 2. Todos los botones visibles
    print(f"\n=== BOTONES VISIBLES ===")
    btns = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('button, [role="button"], a, [role="menuitem"], [role="tab"]');
        for (let e of els) {
            if (e.offsetParent !== null) {
                let info = {
                    tag: e.tagName,
                    text: (e.textContent || '').trim().substring(0, 80),
                    aria: e.getAttribute('aria-label') || '',
                    testid: e.getAttribute('data-test-id') || '',
                    class: (e.className || '').substring(0, 60),
                    id: e.id || '',
                    href: e.getAttribute('href') || ''
                };
                out.push(info);
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = btns.get("result",{}).get("result",{}).get("value","[]")
    try:
        buttons = json.loads(raw)
        for b in buttons:
            print(f"  [{b['tag']}] text='{b['text']}' aria='{b['aria']}' testid='{b['testid']}' class='{b['class']}' id='{b['id']}'")
    except:
        print(f"  (parse error: {raw[:200]})")

    # 3. Buscar "+ Nuevo" especificamente
    print(f"\n=== BUSQUEDA: '+ Nuevo' ===")
    q = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('*');
        for (let e of els) {
            let t = (e.textContent || '').toLowerCase().trim();
            if ((t.includes('nuevo') || t.includes('new')) && e.offsetParent !== null) {
                out.push({
                    tag: e.tagName,
                    text: (e.textContent || '').trim().substring(0, 80),
                    rect: (r => r ? `${r.top}x${r.left}` : 'hidden')(e.getBoundingClientRect()),
                    class: (e.className || '').substring(0, 40)
                });
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    print(f"  Resultados: {raw[:500]}")

    # 4. Buscar "Crear video"
    print(f"\n=== BUSQUEDA: 'Crear video' ===")
    q = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('*');
        for (let e of els) {
            let t = (e.textContent || '').toLowerCase().trim();
            if ((t.includes('crear') || t.includes('create') || t.includes('video') || t.includes('audio')) && e.offsetParent !== null) {
                out.push({
                    tag: e.tagName,
                    text: (e.textContent || '').trim().substring(0, 80),
                    class: (e.className || '').substring(0, 40)
                });
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    print(f"  Resultados: {raw[:500]}")

    # 5. Buscar "Agregar fuente" / "Source"
    print(f"\n=== BUSQUEDA: 'fuente/source' ===")
    q = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('*');
        for (let e of els) {
            let t = (e.textContent || '').toLowerCase().trim();
            if ((t.includes('fuente') || t.includes('source') || t.includes('agregar') || t.includes('add')) && e.offsetParent !== null) {
                out.push({
                    tag: e.tagName,
                    text: (e.textContent || '').trim().substring(0, 80),
                    class: (e.className || '').substring(0, 40)
                });
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    print(f"  Resultados: {raw[:500]}")

    # 6. Buscar "Eliminar" / "Delete"
    print(f"\n=== BUSQUEDA: 'Eliminar/Delete' ===")
    q = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('*');
        for (let e of els) {
            let t = (e.textContent || '').toLowerCase().trim();
            if ((t.includes('eliminar') || t.includes('delete') || t.includes('borrar') || t.includes('remove')) && e.offsetParent !== null) {
                out.push({
                    tag: e.tagName,
                    text: (e.textContent || '').trim().substring(0, 80),
                    class: (e.className || '').substring(0, 40)
                });
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    print(f"  Resultados: {raw[:500]}")

    # 7. Campos de texto
    print(f"\n=== CAMPOS DE TEXTO ===")
    q = c("Runtime.evaluate", {"expression": """
    (() => {
        let out = [];
        let els = document.querySelectorAll('textarea, [contenteditable="true"], [role="textbox"], input[type="text"], input:not([type])');
        for (let e of els) {
            if (e.offsetParent !== null) {
                out.push({
                    tag: e.tagName,
                    placeholder: e.getAttribute('placeholder') || '',
                    class: (e.className || '').substring(0, 40),
                    role: e.getAttribute('role') || ''
                });
            }
        }
        return JSON.stringify(out);
    })()
    """})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    print(f"  Resultados: {raw[:500]}")

    ws.close()
    print("\n[DEBUG] Listo. Copia este output y dimelo para actualizar los selectores.")

if __name__ == "__main__":
    main()

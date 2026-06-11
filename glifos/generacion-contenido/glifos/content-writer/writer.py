#!/usr/bin/env python3
"""Glifo content-writer: abre IA en navegador via Chrome DevTools Protocol (WebSocket).
   CDP permite control preciso del navegador sin coordenadas de pantalla.
   Totalmente automatico. Sin API key. La IA recibe un prompt natural como usuario humano."""

import json, os, sys, subprocess, time, tempfile, socket, random
from datetime import datetime
import urllib.request

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
THEME_FILE = os.path.join(STATE_DIR, "tema_seleccionado.json")
OUTPUT_FILE = os.path.join(STATE_DIR, "respuesta_ia.txt")
AI_URL = os.environ.get("BDGB_AI_WEB_URL", "https://chatgpt.com")

ws = None


def ensure_ws():
    global ws
    if ws is None:
        import websocket
        ws = websocket


def find_free_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def find_browser():
    candidates = [
        (r"C:\Program Files\Google\Chrome\Application\chrome.exe", "chrome"),
        (r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe", "chrome"),
        (os.path.expanduser(r"~\AppData\Local\Google\Chrome\Application\chrome.exe"), "chrome"),
        (r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe", "edge"),
        (r"C:\Program Files\Microsoft\Edge\Application\msedge.exe", "edge"),
    ]
    for path, name in candidates:
        if os.path.exists(path):
            return path, name
    path2, name2 = "chrome", "chrome"
    return path2, name2


def launch_chrome(cdp_port):
    ensure_ws()
    path, name = find_browser()
    if not path or not os.path.exists(path):
        print(f"[content-writer] Chrome no encontrado en rutas conocidas")
        print(f"[content-writer] Usando 'chrome' como comando")
        path = "chrome"

    try:
        if os.name == "nt":
            subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(1)
    except:
        pass

    user_dir = os.path.join(tempfile.gettempdir(), f"bdgb_cdp_{cdp_port}")
    if os.path.exists(user_dir):
        import shutil
        try:
            shutil.rmtree(user_dir)
        except:
            pass

    print(f"[content-writer] Lanzando Chrome en puerto {cdp_port}...")
    try:
        proc = subprocess.Popen(
            [path, f"--remote-debugging-port={cdp_port}",
             "--remote-allow-origins=*",
             f"--user-data-dir={user_dir}", "--no-first-run",
             "--disable-sync", "--no-default-browser-check",
             f"--window-size=1280,800",
             AI_URL],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"[content-writer] Chrome PID: {proc.pid}")
        return True
    except Exception as e:
        print(f"[content-writer] Error lanzando Chrome: {e}")
        return False


def cdp_connect(cdp_port, retries=30):
    ensure_ws()
    print(f"[content-writer] Esperando pagina CDP en puerto {cdp_port}...")
    for attempt in range(retries):
        try:
            pages = json.loads(urllib.request.urlopen(
                f"http://127.0.0.1:{cdp_port}/json", timeout=2).read().decode())
            for p in pages:
                if p.get("type") == "page" and p.get("webSocketDebuggerUrl"):
                    ws_url = p["webSocketDebuggerUrl"]
                    print(f"[content-writer] Conectado a pagina: {ws_url[:60]}...")
                    conn = ws.create_connection(ws_url, timeout=30,
                        header={"Origin": f"http://127.0.0.1:{cdp_port}"})
                    return conn
            if pages and pages[0].get("webSocketDebuggerUrl"):
                ws_url = pages[0]["webSocketDebuggerUrl"]
                conn = ws.create_connection(ws_url, timeout=30,
                    header={"Origin": f"http://127.0.0.1:{cdp_port}"})
                return conn
        except Exception as e:
            if attempt < 3 or attempt % 5 == 4:
                print(f"[content-writer] Esperando pagina... ({attempt+1}/{retries})")
            time.sleep(1)
    return None


def cdp_send(conn, method, params=None):
    if params is None:
        params = {}
    msg = json.dumps({"id": random.randint(1, 999999), "method": method, "params": params})
    conn.send(msg)
    resp = conn.recv()
    return json.loads(resp)


def cdp_evaluate(conn, expression):
    msg_id = random.randint(1000, 999999)
    msg = json.dumps({"id": msg_id, "method": "Runtime.evaluate", "params": {
        "expression": expression,
        "returnByValue": True,
        "awaitPromise": True,
        "timeout": 30000
    }})
    conn.send(msg)
    start = time.time()
    while time.time() - start < 35:
        try:
            conn.settimeout(1)
            resp = json.loads(conn.recv())
            if resp.get("id") == msg_id:
                exc = resp.get("result", {}).get("exceptionDetails")
                if exc:
                    print(f"[content-writer] JS error: {exc.get('text','')}")
                    return ""
                return resp.get("result", {}).get("result", {}).get("value", "")
        except:
            continue
    return ""

def cdp_wait_event(conn, event_name, timeout=30):
    start = time.time()
    while time.time() - start < timeout:
        try:
            conn.settimeout(0.5)
            resp = json.loads(conn.recv())
            if resp.get("method") == event_name:
                return resp
        except:
            continue
    return None


def interact_with_ai(prompt_text, cdp_port):
    conn = cdp_connect(cdp_port)
    if not conn:
        return None

    cdp_send(conn, "Page.enable")
    cdp_send(conn, "Runtime.enable")

    print(f"[content-writer] Esperando carga de ChatGPT (15s)...")
    time.sleep(15)

    p_escaped = json.dumps(prompt_text)
    js_fill = f"""
    (() => {{
        const wait = (ms) => new Promise(r => setTimeout(r, ms));
        const selectors = [
            'textarea[placeholder*="Message"]',
            'textarea[placeholder*="Send"]',
            'textarea[placeholder*="message"]',
            'textarea[placeholder*="send"]',
            '#prompt-textarea',
            'textarea:not([hidden])',
            '[contenteditable="true"]',
            'div[contenteditable="true"]',
            'textarea'
        ];

        async function doFill() {{
            for (let retry = 0; retry < 15; retry++) {{
                for (const sel of selectors) {{
                    const el = document.querySelector(sel);
                    if (el) {{
                        el.focus();
                        await wait(200);
                        if (el.tagName === 'TEXTAREA' || el.tagName === 'INPUT') {{
                            el.value = {p_escaped};
                            el.dispatchEvent(new Event('input', {{bubbles:true}}));
                            el.dispatchEvent(new Event('change', {{bubbles:true}}));
                        }} else {{
                            el.textContent = {p_escaped};
                            el.dispatchEvent(new Event('input', {{bubbles:true}}));
                        }}
                        await wait(300);
                        el.dispatchEvent(new KeyboardEvent('keydown', {{
                            key:'Enter', code:'Enter', keyCode:13, which:13,
                            bubbles:true, cancelable:true
                        }}));
                        return 'OK';
                    }}
                }}
                await wait(1000);
            }}
            return 'NO_INPUT';
        }}
        return doFill();
    }})()
    """

    print(f"[content-writer] Enviando prompt a la IA...")
    result = cdp_evaluate(conn, js_fill)
    info = str(result)[:100]
    print(f"[content-writer] Resultado: {info}")

    if "NO_INPUT" in str(result):
        print(f"[content-writer] No se encontro campo de texto (login? pagina incorrecta?)")
        conn.close()
        return None

    print(f"[content-writer] Esperando respuesta de la IA (60s max)...")
    start = time.time()
    prev_text = ""

    while time.time() - start < 60:
        time.sleep(3)
        js_get = """
        (() => {
            const sel = 'article:last-of-type, [data-message-author-role="assistant"]:last-of-type, .markdown';
            const el = document.querySelector(sel);
            if (el) {
                const t = el.innerText || el.textContent || '';
                if (t.length > 200) return t;
            }
            return '';
        })()
        """
        text = cdp_evaluate(conn, js_get)
        if text and len(text) > 200:
            if text != prev_text:
                prev_text = text
                elapsed = int(time.time() - start)
                print(f"[content-writer] ...recibiendo respuesta ({elapsed}s, {len(text)} chars)")
            else:
                elapsed = int(time.time() - start)
                print(f"[content-writer] Respuesta completa ({elapsed}s, {len(text)} chars)")
                conn.close()
                return text

    conn.close()
    if prev_text and len(prev_text) > 200:
        return prev_text
    return None


def generate_mock(topic):
    return (
        f"# {topic}\n\n"
        f"## Introduccion\n\n"
        f"'{topic}' se ha convertido en un tema central. Este articulo explora "
        f"sus origenes, impacto y futuro.\n\n"
        f"## Contexto\n\n"
        f"El fenomeno de '{topic}' responde a cambios profundos en la sociedad digital. "
        f"Factores technologicos, culturales y economicos confluyen para darle forma.\n\n"
        f"## Analisis\n\n"
        f"Los datos mas recientes muestran que '{topic}' sigue ganando relevancia. "
        f"Expertos senalan que su impacto se extendera a nuevos sectores.\n\n"
        f"## Conclusion\n\n"
        f"'{topic}' no es una moda pasajera. Comprenderlo es clave.\n\n"
        f"---\n"
        f"*Articulo generado con BDGB Panal: generacion-contenido*\n"
        f"*{datetime.now().strftime('%Y-%m-%d %H:%M')}*\n"
    )


def build_prompt(topic):
    return (f"Eres un periodista experto en tecnologia y cultura digital. "
            f"Escribe un articulo profundo, bien estructurado y detallado en espanol "
            f"para una publicacion de divulgacion. "
            f"Extension: 800-1200 palabras. Incluye introduccion, 3-4 secciones "
            f"con subtitulos, y conclusion.\n\n"
            f"Tema del articulo: {topic}")


def main():
    print("[content-writer] Leyendo tema seleccionado...")
    if not os.path.exists(THEME_FILE):
        print(f"[content-writer] ERROR: {THEME_FILE} no encontrado")
        sys.exit(1)

    with open(THEME_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)

    selected = data.get("seleccionado", {})
    topic = selected.get("topic", "")
    if not topic:
        print("[content-writer] ERROR: no hay tema")
        sys.exit(1)

    prompt = build_prompt(topic)
    modo = os.environ.get("BDGB_AI_MODE", "browser")
    text = None

    if modo == "mock":
        print("[content-writer] Modo simulado")
        text = generate_mock(topic)
    elif modo == "browser":
        print(f"[content-writer] Prompt ({len(prompt)} chars)")
        print(f"[content-writer] IA: {AI_URL}")
        print(f"[content-writer] Modo: navegador automatico via CDP")

        cdp_port = find_free_port()
        if launch_chrome(cdp_port):
            text = interact_with_ai(prompt, cdp_port)
        if not text:
            print(f"[content-writer] No se obtuvo respuesta de la IA via CDP.")
            print(f"[content-writer] Posibles causas:")
            print(f"[content-writer] 1. No has iniciado sesion en {AI_URL}")
            print(f"[content-writer] 2. La pagina tardo mucho en cargar")
            print(f"[content-writer] 3. Chrome no se pudo iniciar con CDP")
            print(f"[content-writer] Generando contenido simulado en su lugar.")
            text = generate_mock(topic)

    if not text or len(text) < 50:
        print("[content-writer] Usando contenido simulado.")
        text = generate_mock(topic)

    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(f"Tema: {topic}\n")
        f.write(f"Capturado: {datetime.now().isoformat()}\n")
        f.write("=" * 60 + "\n\n")
        f.write(text)

    print(f"[content-writer] Respuesta guardada en: {OUTPUT_FILE}")
    print(f"[content-writer] Palabras: ~{len(text.split())}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

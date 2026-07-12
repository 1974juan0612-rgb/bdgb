import json, os, subprocess, time, urllib.request, sys

PORT = 9444
OUTFILE = os.path.join(os.environ.get("TEMP", "C:\\Temp"), "dump_result.txt")
CHROME = r"C:\Program Files\Google\Chrome\Application\chrome.exe"
if not os.path.isfile(CHROME):
    CHROME = os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe")

def log(m):
    with open(OUTFILE, "a", encoding="utf-8") as f:
        f.write(m + "\n")

log("=== INICIO ===")
user_data = os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\User Data")
log(f"User Data: {user_data}")

cmd = [
    CHROME,
    f"--remote-debugging-port={PORT}",
    "--remote-allow-origins=*",
    f"--user-data-dir={user_data}",
    "--profile-directory=Profile 2",
    "--app-id=gjcmcplpgihbecacndmmbaenpfgimlec",
    "--no-first-run",
    "--no-default-browser-check"
]

log("Lanzando Chrome...")
proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
log(f"PID: {proc.pid}")

log("Esperando CDP...")
for i in range(30):
    time.sleep(1)
    try:
        resp = urllib.request.urlopen(f"http://localhost:{PORT}/json/list", timeout=2)
        targets = json.loads(resp.read())
        if targets:
            log(f"CDP OK: {len(targets)} targets")
            break
    except Exception as e:
        if i % 5 == 0:
            log(f"  intento {i}: {str(e)[:60]}")
else:
    log("Timeout CDP")
    sys.exit(1)

ws_url = None
try:
    resp = urllib.request.urlopen(f"http://localhost:{PORT}/json/list", timeout=5)
    targets = json.loads(resp.read())
    for t in targets:
        log(f"  target: type={t.get('type')} url={t.get('url','')[:60]}")
        if t.get("type") in ("page", "app"):
            ws_url = t.get("webSocketDebuggerUrl")
            break
    if not ws_url and targets:
        ws_url = targets[0].get("webSocketDebuggerUrl")
except Exception as e:
    log(f"Error targets: {e}")

if not ws_url:
    log("No WS URL")
    sys.exit(1)

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
time.sleep(3)

title = c("Runtime.evaluate", {"expression": "document.title"})
url = c("Runtime.evaluate", {"expression": "window.location.href"})
log(f"TITLE: {title.get('result',{}).get('result',{}).get('value','?')}")
log(f"URL: {url.get('result',{}).get('result',{}).get('value','?')}")

dump = c("Runtime.evaluate", {"expression": """
(function() {
    var out = [];
    var all = document.querySelectorAll('button, [role=\"button\"], a, span, div, li, [role=\"tab\"], [role=\"menuitem\"], h1, h2, h3, p, label');
    for (var i = 0; i < all.length; i++) {
        var e = all[i];
        if (e.offsetParent !== null) {
            var t = (e.textContent || '').trim().substring(0, 120);
            if (t.length > 0) {
                out.push(JSON.stringify({tag: e.tagName, text: t, aria: e.getAttribute('aria-label') || '', role: e.getAttribute('role') || ''}));
            }
        }
    }
    return '[' + out.join(',') + ']';
})()
"""})
raw = dump.get("result",{}).get("result",{}).get("value","[]")
try:
    els = json.loads(raw)
    log(f"\n=== TODOS ({len(els)}) ===")
    for e in els:
        log(f"[{e['tag']}] t='{e['text']}' a='{e['aria']}' r='{e['role']}'")
except Exception as ex:
    log(f"ERROR dump: {ex}")

# Busquedas
for term in ["nuevo", "new", "crear", "create", "fuente", "source", "video", "eliminar", "delete", "agregar", "add", "more", "menu", "cuaderno"]:
    js = """
    (function() {
        var out = [];
        var all = document.querySelectorAll('*');
        var term = '%s';
        for (var i = 0; i < all.length; i++) {
            var e = all[i];
            var t = (e.textContent || '').toLowerCase().trim();
            var a = (e.getAttribute('aria-label') || '').toLowerCase();
            var tt = (e.getAttribute('title') || '').toLowerCase();
            if (e.offsetParent !== null && (t.indexOf(term) >= 0 || a.indexOf(term) >= 0 || tt.indexOf(term) >= 0)) {
                out.push(JSON.stringify({tag: e.tagName, text: (e.textContent || '').trim().substring(0, 80), aria: e.getAttribute('aria-label') || '', role: e.getAttribute('role') || '', title: e.getAttribute('title') || ''}));
            }
        }
        return '[' + out.join(',') + ']';
    })()
    """ % term
    q = c("Runtime.evaluate", {"expression": js})
    raw = q.get("result",{}).get("result",{}).get("value","[]")
    try:
        res = json.loads(raw)
        if res:
            log(f"\n=== '{term}' ({len(res)}) ===")
            for r in res:
                log(f"  [{r['tag']}] t='{r['text']}' a='{r['aria']}' r='{r['role']}' title='{r.get('title','')}'")
    except:
        pass

ws.close()
log("\nLISTO")

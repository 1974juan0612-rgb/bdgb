import json, os, subprocess, time, urllib.request, sys, tempfile

PORT = 9444

# Use temp user-data-dir (Chrome requires non-default for CDP)
tmpdir = os.path.join(tempfile.gettempdir(), "cdp_dump_" + str(int(time.time())))
os.makedirs(tmpdir, exist_ok=True)
CHROME = "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe"
if not os.path.isfile(CHROME):
    CHROME = os.path.expandvars("%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe")

# Kill Chrome first
subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], capture_output=True)
subprocess.run(["taskkill", "/F", "/IM", "chrome_proxy.exe"], capture_output=True)
time.sleep(2)

# Launch Chrome with temp user-data-dir (non-default = required for CDP)
cmd = f'"{CHROME}" --remote-debugging-port={PORT} --remote-allow-origins=* --user-data-dir="{tmpdir}" --no-first-run --no-default-browser-check "https://notebooklm.google.com"'
proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
sys.stderr.write(f"PID: {proc.pid}\n")

# Wait for CDP
sys.stderr.write("Esperando CDP...\n")
for i in range(30):
    time.sleep(1)
    try:
        resp = urllib.request.urlopen(f"http://localhost:{PORT}/json/list", timeout=2)
        targets = json.loads(resp.read())
        if targets:
            sys.stderr.write(f"CDP listo! targets={len(targets)}\n")
            break
    except:
        pass
else:
    sys.stderr.write("Timeout esperando CDP\n")
    sys.exit(1)

# Connect
ws_url = None
resp = urllib.request.urlopen(f"http://localhost:{PORT}/json/list", timeout=5)
targets = json.loads(resp.read())
for t in targets:
    if t.get("type") in ("page", "app"):
        ws_url = t.get("webSocketDebuggerUrl")
        break
if not ws_url and targets:
    ws_url = targets[0].get("webSocketDebuggerUrl")

if not ws_url:
    sys.stderr.write("No WS URL\n")
    # Debug: print targets
    for t in targets:
        sys.stderr.write(f"  type={t.get('type')} url={t.get('url','')[:60]}\n")
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
print(f"TITLE: {title.get('result',{}).get('result',{}).get('value','?')}")
print(f"URL: {url.get('result',{}).get('result',{}).get('value','?')}")

# Dump all visible elements
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
    print(f"\n=== TODOS ({len(els)}) ===")
    for e in els:
        print(f"[{e['tag']}] t='{e['text']}' a='{e['aria']}' r='{e['role']}'")
except Exception as ex:
    print(f"ERROR: {ex}")

# Busquedas clave
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
            print(f"\n=== '{term}' ({len(res)}) ===")
            for r in res:
                print(f"  [{r['tag']}] t='{r['text']}' a='{r['aria']}' r='{r['role']}' title='{r.get('title','')}'")
    except:
        pass

ws.close()
print("\nLISTO")

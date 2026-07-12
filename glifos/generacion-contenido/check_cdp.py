import json, urllib.request

port = 9444
try:
    resp = urllib.request.urlopen(f"http://localhost:{port}/json/list", timeout=5)
    targets = json.loads(resp.read())
    if not targets:
        print("No hay targets en CDP")
    for t in targets:
        print(f"Tipo: {t.get('type')}, URL: {t.get('url','')[:100]}")
except Exception as e:
    print(f"No hay PWA en puerto {port}: {e}")

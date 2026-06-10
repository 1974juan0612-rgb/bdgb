#!/usr/bin/env python3
"""Glifo cleanup: cierra Chrome/Edge abiertos por content-writer (CDP) y limpia archivos temporales."""

import os, sys, subprocess, signal, time, glob, tempfile, shutil

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def kill_chrome_cdp():
    """Mata procesos Chrome/Edge lanzados por content-writer (los de nuestro user-data-dir temporal)."""
    killed = 0
    temp_profiles = set()

    for d in glob.glob(os.path.join(tempfile.gettempdir(), "bdgb_cdp_*")):
        temp_profiles.add(d)

    for name in ["chrome", "msedge"]:
        try:
            if os.name == "nt":
                r = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {name}.exe", "/FO", "CSV"],
                                  capture_output=True, text=True, timeout=5)
                if name in r.stdout:
                    subprocess.run(["taskkill", "/F", "/IM", f"{name}.exe"],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    killed += 1
            else:
                r = subprocess.run(["pgrep", "-x", name], capture_output=True, text=True, timeout=5)
                if r.stdout.strip():
                    subprocess.run(["pkill", "-x", name], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    killed += 1
        except:
            pass

    time.sleep(1)

    for d in temp_profiles:
        try:
            shutil.rmtree(d, ignore_errors=True)
        except:
            pass

    return killed


def kill_port_processes(ports=None):
    """Mata procesos escuchando en los puertos CDP."""
    if ports is None:
        ports = [9222, 9223, 9224, 9225]
    killed = 0
    if os.name == "nt":
        for port in ports:
            try:
                r = subprocess.run(f'netstat -ano | findstr ":{port} "', shell=True,
                                  capture_output=True, text=True, timeout=5)
                for line in r.stdout.split("\n"):
                    parts = line.strip().split()
                    if len(parts) >= 5 and "LISTENING" in line:
                        pid = parts[4]
                        subprocess.run(["taskkill", "/F", "/PID", pid],
                                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                        killed += 1
            except:
                pass
    return killed


def main():
    print("[cleanup] Cerrando navegadores y limpiando recursos...")

    ch = kill_chrome_cdp()
    pk = kill_port_processes()

    print(f"[cleanup] Procesos terminados: {ch} Chrome/Edge, {pk} puertos liberados")

    temp_files = [
        os.path.join(PANAL_DIR, "tendencias.json"),
        os.path.join(PANAL_DIR, "tema_seleccionado.json"),
        os.path.join(PANAL_DIR, "respuesta_ia.txt"),
    ]
    for f in temp_files:
        if os.path.exists(f):
            os.remove(f)
            print(f"[cleanup] Eliminado: {os.path.basename(f)}")

    print("[cleanup] OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())

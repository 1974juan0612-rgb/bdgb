#!/usr/bin/env python3
"""Ejecutable del panal generacion-contenido.
   Ejecuta el pipeline completo: trend-fetcher -> authority-selector -> content-writer.
   Uso: python run.py"""

import os, sys, subprocess

PANAL_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS = [
    ("trend-fetcher",      "glifos/trend-fetcher/trend_fetcher.py"),
    ("authority-selector", "glifos/authority-selector/selector.py"),
    ("content-writer",     "glifos/content-writer/writer.py"),
]

def main():
    os.chdir(os.path.dirname(PANAL_DIR))  # BDGB_ROOT = glifos/
    print(f"===== Panal: Generacion de Contenido =====")
    for name, rel in SCRIPTS:
        script = os.path.join(PANAL_DIR, rel)
        print(f"\n--- {name} ---")
        r = subprocess.run([sys.executable, script], capture_output=False)
        if r.returncode != 0:
            print(f"ERROR: {name} fallo (codigo {r.returncode})")
            sys.exit(r.returncode)
    print(f"\n===== Pipeline completado exitosamente =====")
    return 0

if __name__ == "__main__":
    sys.exit(main())

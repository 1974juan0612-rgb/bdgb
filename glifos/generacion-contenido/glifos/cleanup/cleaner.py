import os, sys, glob, subprocess, time, shutil

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def main():
    print("[cleanup] Limpiando recursos...")

    # Solo limpia directorios temporales de CDP viejos (no mata Chrome)
    temp_dirs = glob.glob(os.path.join(os.environ.get("TMP", os.path.expanduser("~/AppData/Local/Temp")), "bdgb_cdp_*"))
    for d in temp_dirs:
        try:
            shutil.rmtree(d, ignore_errors=True)
            print(f"[cleanup] Eliminado temp: {d}")
        except:
            pass

    print("[cleanup] OK")
    return 0

if __name__ == "__main__":
    sys.exit(main())

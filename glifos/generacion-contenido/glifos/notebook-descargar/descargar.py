import os
import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import time
import shutil

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
VIDEOS_DIR = os.path.expanduser("~/Desktop/videos")

def find_latest_video():
    if not os.path.isdir(VIDEOS_DIR):
        return None
    import glob
    videos = glob.glob(os.path.join(VIDEOS_DIR, "*"))
    if not videos:
        return None
    return max(videos, key=os.path.getmtime)

def poll_for_video(timeout_min=5):
    known = find_latest_video()
    deadline = time.time() + timeout_min * 60
    while time.time() < deadline:
        latest = find_latest_video()
        if latest and latest != known:
            return latest
        time.sleep(3)
    return None

def main():
    os.makedirs(VIDEOS_DIR, exist_ok=True)
    os.makedirs(STATE_DIR, exist_ok=True)

    if os.environ.get("BDGB_AI_MODE", "browser") == "mock":
        print("[DESCARGAR] MOCK")
        with open(os.path.join(STATE_DIR, "video.mp4"), "w") as f:
            f.write("MOCK VIDEO")
        print("[DESCARGAR] Mock guardado")
        return

    print("[DESCARGAR] === INSTRUCCIONES ===")
    print("NotebookLM: espera que termine la generacion del video")
    print(f"Descarga el video a: {VIDEOS_DIR}")
    print("Despues presiona Enter aqui o espera 5 min")
    print("================================")

    input("Presiona Enter cuando hayas descargado el video...")

    video = poll_for_video(timeout_min=5)
    if video:
        shutil.copy2(video, os.path.join(STATE_DIR, "video.mp4"))
        print(f"[DESCARGAR] Video copiado: {video}")
    else:
        print("[DESCARGAR] No se detecto video, placeholder")
        with open(os.path.join(STATE_DIR, "video.mp4"), "w") as f:
            f.write("PLACEHOLDER VIDEO")

    print("[DESCARGAR] Siguiente: notebook-cerrar")

if __name__ == "__main__":
    main()

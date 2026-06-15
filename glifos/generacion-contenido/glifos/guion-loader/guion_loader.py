import os
import sys
import json
import glob

GUIONES_DIR = os.path.expanduser("~/Desktop/guiones")
STATE_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "pipeline_state")

def find_latest_pdf():
    if not os.path.isdir(GUIONES_DIR):
        print(f"[GUION-LOADER] No existe {GUIONES_DIR}")
        return None
    pdfs = glob.glob(os.path.join(GUIONES_DIR, "*.pdf"))
    if not pdfs:
        print(f"[GUION-LOADER] No hay PDFs en {GUIONES_DIR}")
        return None
    latest = max(pdfs, key=os.path.getmtime)
    print(f"[GUION-LOADER] PDF mas reciente: {latest}")
    return latest

def extract_text(pdf_path):
    try:
        import pdfplumber
        text = ""
        with pdfplumber.open(pdf_path) as pdf:
            for page in pdf.pages:
                t = page.extract_text()
                if t:
                    text += t + "\n"
        return text
    except ImportError:
        pass
    try:
        import PyPDF2
        text = ""
        with open(pdf_path, "rb") as f:
            r = PyPDF2.PdfReader(f)
            for page in r.pages:
                t = page.extract_text()
                if t:
                    text += t + "\n"
        return text
    except ImportError:
        pass
    try:
        import pdfminer
        from pdfminer.high_level import extract_text as pdfminer_extract
        return pdfminer_extract(pdf_path)
    except ImportError:
        pass
    print("[GUION-LOADER] No hay libreria PDF instalada. Usando fallback: copia manual.")
    return None

def main():
    pdf = find_latest_pdf()
    if not pdf:
        sys.exit(1)

    text = extract_text(pdf)
    if not text or not text.strip():
        fallback = os.path.splitext(pdf)[0] + ".txt"
        if os.path.isfile(fallback):
            with open(fallback, "r", encoding="utf-8") as f:
                text = f.read()
        else:
            print(f"[GUION-LOADER] No se pudo extraer texto ni hay .txt hermano")
            sys.exit(1)

    os.makedirs(STATE_DIR, exist_ok=True)
    out = os.path.join(STATE_DIR, "guion.txt")
    with open(out, "w", encoding="utf-8") as f:
        f.write(text)
    print(f"[GUION-LOADER] Guion guardado: {out} ({len(text)} chars)")

    meta = {
        "fuente": pdf,
        "titulo": os.path.splitext(os.path.basename(pdf))[0],
        "longitud": len(text)
    }
    meta_path = os.path.join(STATE_DIR, "guion_meta.json")
    with open(meta_path, "w", encoding="utf-8") as f:
        json.dump(meta, f, indent=2, ensure_ascii=False)
    print(f"[GUION-LOADER] Meta guardada: {meta_path}")

if __name__ == "__main__":
    main()

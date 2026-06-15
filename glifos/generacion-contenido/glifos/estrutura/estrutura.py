import os
import sys
import json

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
PROMPTS_DIR = os.path.join(PANAL_DIR, "prompts")

errors = []
warnings = []
created = []
ok = []

def log_ok(msg):
    ok.append(msg)
    print(f"  [OK] {msg}")

def log_warn(msg):
    warnings.append(msg)
    print(f"  [WARN] {msg}")

def log_error(msg):
    errors.append(msg)
    print(f"  [ERROR] {msg}")

def ensure_dir(path, desc=""):
    if os.path.isdir(path):
        log_ok(f"Directorio existe: {path}" + (f" ({desc})" if desc else ""))
        return True
    try:
        os.makedirs(path, exist_ok=True)
        created.append(path)
        log_ok(f"Directorio creado: {path}" + (f" ({desc})" if desc else ""))
        return True
    except Exception as e:
        log_error(f"No se pudo crear directorio {path}: {e}")
        return False

def ensure_file(path, desc="", content=""):
    if os.path.isfile(path):
        log_ok(f"Archivo existe: {path}" + (f" ({desc})" if desc else ""))
        return True
    try:
        with open(path, "w", encoding="utf-8") as f:
            f.write(content)
        created.append(path)
        log_ok(f"Archivo creado: {path}" + (f" ({desc})" if desc else ""))
        return True
    except Exception as e:
        log_error(f"No se pudo crear archivo {path}: {e}")
        return False

def main():
    semilla_path = os.path.join(PANAL_DIR, "semilla.json")
    if not os.path.isfile(semilla_path):
        log_error(f"semilla.json no encontrado en {semilla_path}")
        print_summary()
        sys.exit(1)

    with open(semilla_path, "r", encoding="utf-8") as f:
        semilla = json.load(f)

    panal_id = semilla.get("id", "desconocido")
    panal_nombre = semilla.get("nombre", panal_id)
    print(f"\n===== ESTRUTORA: {panal_nombre} ({panal_id}) =====\n")

    # 1. Directorios base del panal
    ensure_dir(STATE_DIR, "pipeline_state — comunicacion entre glifos")

    # 1b. Directorios externos que los glifos necesitan
    guiones_dir = os.path.expanduser("~/Desktop/guiones")
    videos_dir = os.path.expanduser("~/Desktop/videos")
    ensure_dir(guiones_dir, "PDFs generados por clipboard-capturer")
    ensure_dir(videos_dir, "videos descargados por notebook-video")

    # 2. Prompts si hay bibliotecario que los referencie
    biblio = semilla.get("bibliotecario", {})
    recursos = biblio.get("recursos", [])
    if recursos:
        ensure_dir(PROMPTS_DIR, "recursos del bibliotecario")
        for r in recursos:
            rpath = os.path.join(PANAL_DIR, r)
            dirp = os.path.dirname(rpath)
            if dirp and not os.path.isdir(dirp):
                ensure_dir(dirp)
            if not os.path.isfile(rpath):
                ensure_file(rpath, f"recurso bibliotecario: {r}",
                            content=f"# {os.path.basename(rpath)}\n# Recurso auto-generado por estrutura\n")

    # 3. Por cada glifo declarado en semilla.json
    glifos = semilla.get("glifos", [])
    pipeline = semilla.get("config", {}).get("pipeline", [])
    pipeline_ids = set(pipeline)

    glifo_ids = set()
    for g in glifos:
        gid = g.get("id", "")
        if not gid:
            continue
        glifo_ids.add(gid)

        # glifo.json
        gj_path = os.path.join(PANAL_DIR, "glifos", gid, "glifo.json")
        if not os.path.isfile(gj_path):
            # try relative from glifo_json field
            gj_rel = g.get("glifo_json", "")
            if gj_rel:
                gj_path2 = os.path.join(PANAL_DIR, gj_rel)
                if os.path.isfile(gj_path2):
                    gj_path = gj_path2

        if not os.path.isfile(gj_path):
            warn_or_error(f"glifo.json no encontrado para '{gid}' en {gj_path}")
        else:
            log_ok(f"glifo.json para '{gid}': {gj_path}")

        # entry file
        entry = g.get("entry", "")
        if entry:
            # entry puede ser "python3 path/to/script.py" o "bdgb --glifo-run id"
            parts = entry.split()
            if len(parts) >= 2 and not parts[0].startswith("bdgb"):
                script_rel = parts[-1]
                script_path = os.path.join(os.environ.get("BDGB_ROOT", "."), script_rel)
                if not os.path.isfile(script_path):
                    # try relative to panal
                    script_path2 = os.path.join(PANAL_DIR, script_rel)
                    if os.path.isfile(script_path2):
                        script_path = script_path2
                if os.path.isfile(script_path):
                    log_ok(f"entry existe: {script_path}")
                else:
                    warn_or_error(f"entry no encontrado para '{gid}': {script_path}")
            else:
                log_ok(f"entry nativo: {entry}")

        # directorio del glifo
        gdir = os.path.join(PANAL_DIR, "glifos", gid)
        if not os.path.isdir(gdir):
            ensure_dir(gdir, f"directorio del glifo '{gid}'")

        # No se verifican directorios de outputs — los glifos los crean al ejecutarse

    # 4. Validar pipeline vs glifos declarados
    for pid in pipeline:
        if pid not in glifo_ids:
            warn_or_error(f"config.pipeline referencia glifo '{pid}' que no esta declarado en glifos[]")

    # 5. Glifos sin dependencias salvo estructora itself
    if "estrutura" not in glifo_ids:
        log_warn("estrutura no declarado en glifos[] — deberia agregarse")

    # Summary
    print_summary()

def warn_or_error(msg):
    log_error(msg)

def print_summary():
    print(f"\n----- ESTRUTORA: resumen -----")
    print(f"  {len(ok)} verificaciones OK")
    print(f"  {len(warnings)} advertencias")
    print(f"  {len(errors)} errores")
    print(f"  {len(created)} elementos creados")
    if created:
        print("  Creado:")
        for c in created:
            print(f"    - {c}")
    if warnings:
        print("  Advertencias (continuar):")
        for w in warnings:
            print(f"    - {w}")
    if errors:
        print("  Errores (requieren accion):")
        for e in errors:
            print(f"    - {e}")
    print(f"=====================================\n")

if __name__ == "__main__":
    main()

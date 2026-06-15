#!/usr/bin/env python3
"""Orquestador generico del panal.
   Lee semilla.json, ejecuta pipeline declarado en config.pipeline.
   Equivalente Python al orquestador C (glifo_pipeline_run).
   Uso: python run.py"""

import os, sys, json, subprocess, time, glob

PANAL_DIR = os.path.dirname(os.path.abspath(__file__))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")

def log(msg):
    print(f"[PIPELINE] {msg}")

def run_command(cmd):
    log(cmd)
    r = subprocess.run(cmd, shell=True, capture_output=False)
    return r.returncode

def find_entry(semilla, glifo_id):
    for g in semilla.get("glifos", []):
        if g.get("id") == glifo_id:
            return g.get("entry", "")
    return ""

def find_cleanup_step(step_ids):
    for i, sid in enumerate(step_ids):
        if "cleanup" in sid:
            return i
    return -1

def main():
    semilla_path = os.path.join(PANAL_DIR, "semilla.json")
    if not os.path.isfile(semilla_path):
        log(f"ERROR: no se encuentra {semilla_path}")
        sys.exit(1)

    with open(semilla_path, "r", encoding="utf-8") as f:
        semilla = json.load(f)

    panal_id = semilla.get("id", "desconocido")
    log(f"===== Panal: {panal_id} =====")

    # Determinar orden del pipeline
    step_ids = []
    step_args = []

    # Priority 1: config.pipeline
    cfg = semilla.get("config", {})
    pipeline = cfg.get("pipeline", [])
    if pipeline:
        for pid in pipeline:
            step_ids.append(pid)
            step_args.append("")

    # Priority 2: pipeline.orden
    if not step_ids:
        orden = semilla.get("pipeline", {}).get("orden", [])
        for step in orden:
            gid = step.get("glifo", "")
            if not gid:
                continue
            step_ids.append(gid)
            args_list = step.get("args", [])
            step_args.append(" ".join(args_list) if args_list else "")

    # Priority 3: glifos en orden
    if not step_ids:
        for g in semilla.get("glifos", []):
            gid = g.get("id", "")
            if gid:
                step_ids.append(gid)
                step_args.append("")

    if not step_ids:
        log("ERROR: no se encontraron pasos en semilla.json")
        sys.exit(1)

    # Identificar cleanup para finally
    cleanup_idx = find_cleanup_step(step_ids)
    has_cleanup = cleanup_idx >= 0

    # Ejecutar pipeline con finally
    first_error = 0
    total_ok = 0
    total_skip = 0

    for i, gid in enumerate(step_ids):
        if has_cleanup and i == cleanup_idx:
            continue

        entry = find_entry(semilla, gid)
        if not entry:
            log(f"ERROR: '{gid}' no tiene entry")
            first_error = 1
            continue

        cmd = f"{entry} {step_args[i]}" if step_args[i] else entry
        log(f"Paso {i+1}/{len(step_ids)}: {gid}")
        r = run_command(cmd)

        if r == 0:
            total_ok += 1
        else:
            log(f"ERROR: '{gid}' fallo (codigo {r}), continuando al finally")
            if not first_error:
                first_error = r

    # FINALLY: cleanup siempre se ejecuta
    if has_cleanup:
        log("FINALLY: ejecutando cleanup")
        entry = find_entry(semilla, step_ids[cleanup_idx])
        if entry:
            run_command(entry)

    if first_error:
        log(f"===== Pipeline fallido: {total_ok} OK, {total_skip} SKIP =====")
        sys.exit(first_error)

    log(f"===== Pipeline completado: {total_ok} OK =====")

if __name__ == "__main__":
    main()

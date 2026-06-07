#!/usr/bin/env python3
"""
trend_tracker.py — Agente Trend Tracker para BDGB.
Carpeta separada de la masa madre: agents/trend-tracker/

Tareas:
  --daily    : Obtiene Google Trends, guarda reporte diario en daily/
  --weekly   : Agrega los ultimos 7 reportes en weekly/YYYY-WW.json
  --status   : Muestra estado actual del agente
  --inject   : Inyecta tendencias en BDGB via bdgb --add-concept

Schedule recomendado (cron / supervisor tick):
  DIARIO  a las 08:00: python trend_tracker.py --daily
  LUNES   a las 09:00: python trend_tracker.py --weekly
"""

import json
import os
import sys
import glob
from datetime import datetime, timedelta

AGENT_DIR = os.path.dirname(os.path.abspath(__file__))
DAILY_DIR = os.path.join(AGENT_DIR, "daily")
WEEKLY_DIR = os.path.join(AGENT_DIR, "weekly")

BDGB_ROOT = os.environ.get(
    "BDGB_ROOT",
    os.path.normpath(os.path.join(AGENT_DIR, "..", ".."))
)

# ---- MOCK TRENDS (fallback when pytrends not available) ----
MOCK_TRENDS = [
    {"topic": "inteligencia artificial pixel art", "score": 88, "category": "tecnologia"},
    {"topic": "video juegos retro 2026", "score": 79, "category": "entretenimiento"},
    {"topic": "musica lo fi para estudiar", "score": 73, "category": "musica"},
    {"topic": "tutorial blender 3d", "score": 67, "category": "educacion"},
    {"topic": "cocina vegana rapida", "score": 61, "category": "cocina"},
    {"topic": "diseno de personajes pixel", "score": 82, "category": "arte"},
    {"topic": "animacion 2d principiantes", "score": 71, "category": "educacion"},
    {"topic": "chip tunes musica", "score": 64, "category": "musica"},
    {"topic": "game development sin codigo", "score": 76, "category": "tecnologia"},
    {"topic": "ia generativa arte", "score": 91, "category": "tecnologia"},
]

TREND_CACHE = {}


def fetch_trends():
    """Intenta pytrends real; fallback a MOCK_TRENDS."""
    try:
        from pytrends.request import TrendReq
        pytrends = TrendReq(hl="es", tz=360)
        pytrends.build_payload(kw_list=["inteligencia artificial", "pixel art",
                                        "video juegos", "musica", "ia"])
        data = pytrends.interest_over_time()
        if data is not None and not data.empty:
            trends = []
            for col in data.columns:
                if col != "isPartial":
                    trends.append({
                        "topic": col,
                        "score": int(data[col].iloc[-1]),
                        "category": "general"
                    })
            return trends
    except Exception:
        pass
    return MOCK_TRENDS


def save_daily_report(trends):
    """Guarda el reporte diario como JSON."""
    today = datetime.now().strftime("%Y-%m-%d")
    report = {
        "date": today,
        "timestamp": datetime.now().isoformat(),
        "source": "pytrends" if len(trends) > 5 and trends[0]["topic"] != MOCK_TRENDS[0]["topic"] else "mock",
        "trend_count": len(trends),
        "trends": trends
    }
    path = os.path.join(DAILY_DIR, f"{today}.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
    return path


def load_daily_reports(days=7):
    """Carga los ultimos N reportes diarios."""
    files = sorted(glob.glob(os.path.join(DAILY_DIR, "*.json")))
    reports = []
    for f in files[-days:]:
        with open(f, "r", encoding="utf-8") as fh:
            try:
                reports.append(json.load(fh))
            except json.JSONDecodeError:
                continue
    return reports


def generate_weekly_summary():
    """Agrega los ultimos 7 reportes en un resumen semanal."""
    reports = load_daily_reports(7)
    if not reports:
        print('[TRENDTRACKER] No hay suficientes reportes diarios')

    all_trends = {}
    for r in reports:
        for t in r.get("trends", []):
            name = t["topic"]
            if name not in all_trends:
                all_trends[name] = {"score_total": 0, "count": 0, "category": t.get("category", "general")}
            all_trends[name]["score_total"] += t["score"]
            all_trends[name]["count"] += 1

    ranked = sorted(
        [{"topic": k, "avg_score": v["score_total"] // v["count"],
          "appearances": v["count"], "category": v["category"]}
         for k, v in all_trends.items()],
        key=lambda x: (-x["avg_score"], -x["appearances"])
    )

    today = datetime.now()
    week_num = today.isocalendar()[1]
    summary = {
        "week": week_num,
        "year": today.year,
        "generated": today.isoformat(),
        "days_covered": len(reports),
        "date_range": {
            "from": reports[0]["date"] if reports else None,
            "to": reports[-1]["date"] if reports else None
        },
        "top_trends": ranked[:15],
        "total_unique_topics": len(ranked)
    }

    path = os.path.join(WEEKLY_DIR, f"{today.year}-W{week_num:02d}.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2, ensure_ascii=False)

    return path, summary


def inject_into_bdgb(trends):
    """Inyecta tendencias en BDGB via CLI."""
    import subprocess
    bdgb_exe = os.path.join(BDGB_ROOT, "build",
                            "bdgb.exe" if sys.platform == "win32" else "bdgb")
    if not os.path.exists(bdgb_exe):
        return 0
    data_path = os.path.join(BDGB_ROOT, "data")
    injected = 0
    for i, t in enumerate(trends):
        node_id = (i + 10) % 256
        concept_id = 6000 + i
        cmd = [bdgb_exe, "--data-path", data_path,
               "--add-concept", str(node_id), str(concept_id),
               str(t.get("score", 100)), "0"]
        try:
            r = subprocess.run(cmd, capture_output=True, timeout=10,
                               env={**os.environ, "BDGB_ROOT": BDGB_ROOT})
            if r.returncode == 0:
                injected += 1
        except Exception:
            pass
    return injected


def cmd_daily():
    print("[TRENDTRACKER] Obteniendo tendencias...")
    trends = fetch_trends()
    path = save_daily_report(trends)
    print(f"[TRENDTRACKER] Reporte diario guardado: {path}")
    n = inject_into_bdgb(trends[:5])
    if n > 0:
        print(f"[TRENDTRACKER] {n} tendencias inyectadas en BDGB")


def cmd_weekly():
    print("[TRENDTRACKER] Generando resumen semanal...")
    path, summary = generate_weekly_summary()
    print(f"[TRENDTRACKER] Resumen semanal guardado: {path}")
    print(json.dumps(summary["top_trends"][:5], indent=2, ensure_ascii=False))

    # Aprender los top trends como terminos NLP
    import subprocess
    texts = [t["topic"] for t in summary["top_trends"][:10]]
    combined = ". ".join(texts)
    bdgb_exe = os.path.join(BDGB_ROOT, "build",
                            "bdgb.exe" if sys.platform == "win32" else "bdgb")
    if os.path.exists(bdgb_exe):
        cmd = [bdgb_exe, "--data-path", os.path.join(BDGB_ROOT, "data"),
               "--learn", combined]
        subprocess.run(cmd, capture_output=True, timeout=10,
                       env={**os.environ, "BDGB_ROOT": BDGB_ROOT})


def cmd_status():
    reports = load_daily_reports(365 * 10)
    weeklies = sorted(glob.glob(os.path.join(WEEKLY_DIR, "*.json")))
    status = {
        "agente": "trend-tracker",
        "estado": "activo",
        "total_reportes_diarios": len(reports),
        "total_resumenes_semanales": len(weeklies),
        "ultimo_reporte": reports[-1]["date"] if reports else None,
        "ultimo_resumen": weeklies[-1] if weeklies else None,
    }
    print(json.dumps(status, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    if "--daily" in sys.argv:
        cmd_daily()
    elif "--weekly" in sys.argv:
        cmd_weekly()
    elif "--status" in sys.argv:
        cmd_status()
    elif "--inject" in sys.argv:
        trends = fetch_trends()
        n = inject_into_bdgb(trends)
        print(f"[TRENDTRACKER] {n} tendencias inyectadas")
    else:
        print(__doc__.strip())

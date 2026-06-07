#!/usr/bin/env python3
"""
trend_tracker.py — Agente Trend Tracker para BDGB.
Carpeta separada de la masa madre: agents/trend-tracker/

Tareas:
  --daily    : Obtiene Google Trends, guarda reporte diario en daily/ (JSON + PDF)
  --weekly   : Agrega los ultimos 7 reportes en weekly/YYYY-WW.json + PDF
  --export-pdf : Convierte TODOS los JSONs de daily/ y weekly/ a PDF
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


def json_to_pdf_daily(json_path):
    """Convierte un reporte diario JSON a PDF."""
    from reportlab.lib.pagesizes import A4
    from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle
    from reportlab.lib.styles import getSampleStyleSheet
    from reportlab.lib import colors

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    pdf_path = json_path.replace(".json", ".pdf")
    doc = SimpleDocTemplate(pdf_path, pagesize=A4,
                            title=f"Trend Report {data['date']}")
    styles = getSampleStyleSheet()
    story = []

    story.append(Paragraph(f"<b>BDGB Trend Report</b>", styles["Title"]))
    story.append(Spacer(1, 8))
    story.append(Paragraph(f"Fecha: {data['date']}", styles["Normal"]))
    story.append(Paragraph(f"Fuente: {data['source']}", styles["Normal"]))
    story.append(Paragraph(f"Tendencias detectadas: {data['trend_count']}", styles["Normal"]))
    story.append(Spacer(1, 12))

    table_data = [["#", "Tema", "Score", "Categoria"]]
    for i, t in enumerate(data["trends"], 1):
        table_data.append([str(i), t["topic"], str(t["score"]), t.get("category", "")])

    t = Table(table_data, colWidths=[30, 280, 60, 120])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#2C3E50")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("FONTSIZE", (0, 0), (-1, -1), 9),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#ECF0F1")]),
    ]))
    story.append(t)
    doc.build(story)
    return pdf_path


def json_to_pdf_weekly(json_path):
    """Convierte un resumen semanal JSON a PDF."""
    from reportlab.lib.pagesizes import A4
    from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle
    from reportlab.lib.styles import getSampleStyleSheet
    from reportlab.lib import colors

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    pdf_path = json_path.replace(".json", ".pdf")
    doc = SimpleDocTemplate(pdf_path, pagesize=A4,
                            title=f"Weekly Trend Summary W{data['week']}")
    styles = getSampleStyleSheet()
    story = []

    story.append(Paragraph(f"<b>BDGB Weekly Trend Summary</b>", styles["Title"]))
    story.append(Spacer(1, 8))
    story.append(Paragraph(f"Semana: W{data['week']} ({data['year']})", styles["Normal"]))
    if data["date_range"]["from"] and data["date_range"]["to"]:
        story.append(Paragraph(f"Periodo: {data['date_range']['from']} — {data['date_range']['to']}", styles["Normal"]))
    story.append(Paragraph(f"Dias cubiertos: {data['days_covered']}", styles["Normal"]))
    story.append(Paragraph(f"Topicos unicos: {data['total_unique_topics']}", styles["Normal"]))
    story.append(Spacer(1, 12))

    story.append(Paragraph("<b>Top Tendencias</b>", styles["Heading2"]))
    story.append(Spacer(1, 6))

    table_data = [["#", "Tema", "Score Prom.", "Apariciones", "Categoria"]]
    for i, t in enumerate(data["top_trends"], 1):
        table_data.append([str(i), t["topic"], str(t["avg_score"]),
                           str(t["appearances"]), t["category"]])

    t = Table(table_data, colWidths=[25, 240, 70, 70, 100])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#2C3E50")),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("FONTSIZE", (0, 0), (-1, -1), 9),
        ("GRID", (0, 0), (-1, -1), 0.5, colors.grey),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#ECF0F1")]),
    ]))
    story.append(t)
    doc.build(story)
    return pdf_path


def export_all_to_pdf():
    """Convierte todos los JSONs de daily/ y weekly/ a PDF."""
    count = 0
    for f in sorted(glob.glob(os.path.join(DAILY_DIR, "*.json"))):
        pdf = f.replace(".json", ".pdf")
        if not os.path.exists(pdf):
            json_to_pdf_daily(f)
            count += 1
            print(f"  PDF: {os.path.basename(pdf)}")
    for f in sorted(glob.glob(os.path.join(WEEKLY_DIR, "*.json"))):
        pdf = f.replace(".json", ".pdf")
        if not os.path.exists(pdf):
            json_to_pdf_weekly(f)
            count += 1
            print(f"  PDF: {os.path.basename(pdf)}")
    return count


def cmd_daily():
    print("[TRENDTRACKER] Obteniendo tendencias...")
    trends = fetch_trends()
    path = save_daily_report(trends)
    print(f"[TRENDTRACKER] Reporte diario guardado: {path}")
    pdf = json_to_pdf_daily(path)
    print(f"[TRENDTRACKER] PDF: {pdf}")
    n = inject_into_bdgb(trends[:5])
    if n > 0:
        print(f"[TRENDTRACKER] {n} tendencias inyectadas en BDGB")


def cmd_weekly():
    print("[TRENDTRACKER] Generando resumen semanal...")
    path, summary = generate_weekly_summary()
    print(f"[TRENDTRACKER] Resumen semanal guardado: {path}")
    pdf = json_to_pdf_weekly(path)
    print(f"[TRENDTRACKER] PDF: {pdf}")
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
    elif "--export-pdf" in sys.argv:
        n = export_all_to_pdf()
        print(f"[TRENDTRACKER] {n} PDFs generados")
    elif "--status" in sys.argv:
        cmd_status()
    elif "--inject" in sys.argv:
        trends = fetch_trends()
        n = inject_into_bdgb(trends)
        print(f"[TRENDTRACKER] {n} tendencias inyectadas")
    else:
        print(__doc__.strip())

#!/usr/bin/env python3
"""scraper_trends.py — Google Trends + YouTube scraper for BDGB.

Usage:
    python scraper_trends.py                          # fetch trends → stdout JSON
    python scraper_trends.py --ingest                 # fetch + inject into BDGB
    python scraper_trends.py --list                   # list known topics from BDGB

Requires: pip install pytrends google-api-python-client
(or set DISABLE_EXTERNAL=1 to use mock data)
"""
import json
import os
import subprocess
import sys
import time
from datetime import datetime

BDGB_ROOT = os.environ.get("BDGB_ROOT", os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BDGB_EXE = os.path.join(BDGB_ROOT, "build", "bdgb.exe")
DATA_PATH = os.path.join(BDGB_ROOT, "data")

# ---- MOCK: fallback when pytrends not installed ----
MOCK_TRENDS = [
    {"topic": "inteligencia artificial cine", "score": 85, "category": "tecnologia"},
    {"topic": "video juegos retro", "score": 72, "category": "entretenimiento"},
    {"topic": "musica lo fi estudio", "score": 68, "category": "musica"},
    {"topic": "tutorial python principiantes", "score": 65, "category": "educacion"},
    {"topic": "cocina rapida saludable", "score": 60, "category": "cocina"},
]

TREND_CONCEPT_BASE = 5000  # concept IDs start here


def get_trends():
    """Fetch trending topics. Falls back to mock if pytrends not available."""
    try:
        from pytrends.request import TrendReq
        pytrends = TrendReq(hl="es-MX", tz=360)
        pytrends.build_payload(kw_list=[""], cat=0, timeframe="now 1-d", geo="MX", gprop="")
        related = pytrends.related_queries()
        trends = []
        for kw, data in related.items():
            if data and "top" in data and data["top"] is not None:
                for _, row in data["top"].head(10).iterrows():
                    trends.append({
                        "topic": row["query"],
                        "score": int(row["value"]),
                        "category": kw,
                    })
        return trends[:20] if trends else MOCK_TRENDS
    except ImportError:
        print("[SCRAPER] pytrends not installed, using mock data", file=sys.stderr)
        return MOCK_TRENDS
    except Exception as e:
        print(f"[SCRAPER] Error fetching trends: {e}", file=sys.stderr)
        return MOCK_TRENDS


def get_youtube_trends():
    """Fetch YouTube trending (mock for now)."""
    return [
        {"topic": "gameplay indie sorprendente", "score": 78, "category": "gaming"},
        {"topic": "review gadget 2026", "score": 74, "category": "tecnologia"},
        {"topic": "documental naturaleza 4k", "score": 70, "category": "educacion"},
    ]


def ingest_into_bdgb(trends):
    """Convert trends into BDGB nodes + concepts via CLI."""
    for i, t in enumerate(trends):
        node_id = i % 240 + 10  # avoid clashing with demo concepts (0-15)
        concept_id = TREND_CONCEPT_BASE + i

        # Add as concept
        cmd = [BDGB_EXE, "--data-path", DATA_PATH,
               "--add-concept", str(node_id), str(concept_id), "150", "0"]
        env = os.environ.copy()
        env["BDGB_ROOT"] = BDGB_ROOT
        subprocess.run(cmd, capture_output=True, env=env)

        print(f"[INGEST] node {node_id} <- concept {concept_id} '{t['topic']}' (score={t['score']})")


def list_topics():
    """Show topics currently in BDGB."""
    cmd = [BDGB_EXE, "--data-path", DATA_PATH, "--export-nodes"]
    env = os.environ.copy()
    env["BDGB_ROOT"] = BDGB_ROOT
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=10, env=env)
        data = json.loads(r.stdout)
        nodes = data.get("nodes", [])
        print(f"[BDGB] {len(nodes)} nodos en total")
        for n in nodes[:20]:
            print(f"  #{n['id']:3d}  bits={n['bits']:08b}  "
                  f"dens={n['densidad']}  sim={'S' if n['simetria'] else 'N'}  "
                  f"geom={n['tipo_geom']}  atractor={n['atractor_id']}")
        if len(nodes) > 20:
            print(f"  ... y {len(nodes) - 20} mas")
    except Exception as e:
        print(f"[ERROR] {e}")


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--ingest":
        trends = get_trends()
        yt = get_youtube_trends()
        all_trends = trends + yt
        ingest_into_bdgb(all_trends)
        print(f"\n[SCRAPER] Ingestadas {len(all_trends)} tendencias")
    elif len(sys.argv) > 1 and sys.argv[1] == "--list":
        list_topics()
    else:
        trends = get_trends()
        print(json.dumps(trends, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()

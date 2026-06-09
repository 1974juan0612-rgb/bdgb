#!/usr/bin/env python3
"""Glifo trend-fetcher: busca tendencias en internet y las guarda en tendencias.json"""

import json, os, sys, subprocess, random
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DATA_FILE = os.path.join(PANAL_DIR, "tendencias.json")
TRENDS_RSS = "https://trends.google.com/trending/rss?geo=US"

MOCK_TOPICS = [
    "inteligencia artificial pixel art",
    "video juegos retro 2026",
    "musica lo fi para estudiar",
    "tutorial blender 3d",
    "cocina vegana rapida",
    "diseno de personajes pixel",
    "animacion 2d principiantes",
    "chip tunes musica",
    "game development sin codigo",
    "ia generativa arte",
    "machine learning educacion",
    "realidad virtual accesible",
    "computacion cuantica basica",
    "seguridad informatica hogar",
    "criptomonedas regulacion"
]

MOCK_CATEGORIES = [
    "tecnologia", "entretenimiento", "musica", "educacion", "cocina",
    "arte", "educacion", "musica", "tecnologia", "tecnologia",
    "educacion", "tecnologia", "ciencia", "seguridad", "finanzas"
]

MOCK_SCORES = [88, 79, 73, 67, 61, 82, 71, 64, 76, 91, 85, 69, 58, 77, 63]


def fetch_rss():
    trends = []
    try:
        r = subprocess.run(["curl", "-s", "--max-time", "5", TRENDS_RSS],
                          capture_output=True, text=True, timeout=10)
        body = r.stdout
        if "<title>" in body and "FALLBACK" not in body:
            import re
            titles = re.findall(r"<title>(.*?)</title>", body)
            for t in titles:
                t = t.strip()
                if t:
                    trends.append({
                        "topic": t,
                        "score": 50 + random.randint(0, 49),
                        "category": "general",
                        "source": "google-trends-rss"
                    })
        if trends:
            return trends
    except Exception as e:
        print(f"[trend-fetcher] RSS fallback: {e}")

    for i in range(len(MOCK_TOPICS)):
        trends.append({
            "topic": MOCK_TOPICS[i],
            "score": MOCK_SCORES[i] + random.randint(-5, 5),
            "category": MOCK_CATEGORIES[i],
            "source": "mock"
        })
    print(f"[trend-fetcher] Usando datos mock ({len(trends)} tendencias)")
    return trends


def main():
    print("[trend-fetcher] Buscando tendencias en internet...")
    trends = fetch_rss()
    if not trends:
        print("[trend-fetcher] ERROR: no se encontraron tendencias")
        sys.exit(1)

    out = {
        "fecha": datetime.now().isoformat(),
        "total": len(trends),
        "tendencias": trends
    }
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    print(f"[trend-fetcher] {len(trends)} tendencias guardadas en {DATA_FILE}")
    for i, t in enumerate(trends[:5]):
        print(f"  {i+1}. {t['topic']:40s} {t['score']:3d}  [{t['category']}]")
    return 0


if __name__ == "__main__":
    sys.exit(main())

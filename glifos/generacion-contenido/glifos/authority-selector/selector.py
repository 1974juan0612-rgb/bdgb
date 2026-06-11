#!/usr/bin/env python3
"""Glifo authority-selector: elige la tendencia de mayor autoridad"""

import json, os, sys
from datetime import datetime

PANAL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
STATE_DIR = os.path.join(PANAL_DIR, "pipeline_state")
TRENDS_FILE = os.path.join(STATE_DIR, "tendencias.json")
OUTPUT_FILE = os.path.join(STATE_DIR, "tema_seleccionado.json")


def calculate_authority(trend):
    score = trend.get("score", 0)
    category_bonus = {"tecnologia": 5, "ciencia": 5, "educacion": 3}
    bonus = category_bonus.get(trend.get("category", ""), 0)
    topic_len_bonus = min(len(trend.get("topic", "")) // 10, 5)
    return score + bonus + topic_len_bonus


def main():
    print("[authority-selector] Leyendo tendencias...")
    if not os.path.exists(TRENDS_FILE):
        print(f"[authority-selector] ERROR: {TRENDS_FILE} no encontrado. Ejecutar trend-fetcher primero")
        sys.exit(1)

    with open(TRENDS_FILE, "r", encoding="utf-8") as f:
        data = json.load(f)

    trends = data.get("tendencias", [])
    if not trends:
        print("[authority-selector] ERROR: no hay tendencias que evaluar")
        sys.exit(1)

    ranked = [(calculate_authority(t), t) for t in trends]
    ranked.sort(key=lambda x: x[0], reverse=True)

    winner = ranked[0][1]
    winner["autoridad"] = ranked[0][0]
    winner["fecha_seleccion"] = datetime.now().isoformat()
    winner["rango"] = [{"topic": t["topic"], "score": s, "authority": s}
                       for s, t in ranked[:5]]

    out = {
        "fecha": datetime.now().isoformat(),
        "metodo_seleccion": "autoridad (score + categoria + longitud_titulo)",
        "total_evaluadas": len(trends),
        "seleccionado": winner,
        "top_5": winner["rango"]
    }

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    print(f"[authority-selector] Tema seleccionado por autoridad:")
    print(f"  {winner['topic']} (autoridad: {winner['autoridad']})")
    print(f"  Categoria: {winner.get('category', 'N/A')}")
    print(f"  Score base: {winner.get('score', 0)}")
    print(f"  Guardado en: {OUTPUT_FILE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

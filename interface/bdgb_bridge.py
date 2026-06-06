"""bdgb_bridge.py — Python-C bridge: calls bdgb.exe CLI for all engine operations.

Replaces the duplicated search/semantic logic that was in bdgb_gui.py.
Single source of truth: the C binary.
"""
import json
import os
import subprocess
import struct

import platform

BDGB_ROOT = os.environ.get(
    "BDGB_ROOT",
    os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
)
_BDGB_BIN = "bdgb.exe" if platform.system() == "Windows" else "bdgb"
BDGB_EXE = os.path.join(BDGB_ROOT, "build", _BDGB_BIN)
DATA_PATH = os.path.join(BDGB_ROOT, "data")
NODE_FILE = os.path.join(DATA_PATH, "nodes.dat")


def _run_cmd(*args):
    env = os.environ.copy()
    env["BDGB_ROOT"] = BDGB_ROOT
    cmd = [BDGB_EXE, "--data-path", DATA_PATH] + list(args)
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30, env=env)
        if r.returncode != 0:
            return {"error": r.stderr.strip() or f"exit code {r.returncode}"}
        if not r.stdout.strip():
            return {}
        return json.loads(r.stdout)
    except FileNotFoundError:
        return {"error": f"engine not found at {BDGB_EXE}"}
    except json.JSONDecodeError:
        return {"error": "invalid JSON from engine", "raw": r.stdout}
    except subprocess.TimeoutExpired:
        return {"error": "engine timeout"}


def search(query: str) -> dict:
    return _run_cmd("--search", query)


def export_nodes() -> dict:
    return _run_cmd("--export-nodes")


def add_concept(node_id: int, concept_id: int, weight: int = 200, rel_type: int = 0) -> dict:
    return _run_cmd("--add-concept", str(node_id), str(concept_id), str(weight), str(rel_type))


def init_data() -> dict:
    return _run_cmd("--init")


def agent_run(agent_id: str) -> dict:
    return _run_cmd("--agent-run", agent_id)


def load_nodes_from_disk() -> dict:
    """Read nodes.dat directly (fast, no engine needed)."""
    nodes = {}
    if not os.path.exists(NODE_FILE):
        return nodes
    with open(NODE_FILE, "rb") as f:
        raw = f.read()
    for i in range(0, len(raw), 4):
        if i + 4 <= len(raw):
            node_id = raw[i]
            x = raw[i + 1]
            y = raw[i + 2]
            flags = raw[i + 3]
            bits = f"{node_id:08b}"
            nodes[node_id] = {
                "id": node_id, "x": x, "y": y, "bits": bits, "flags": flags,
            }
    return nodes


def decode_props(node_id: int, props: dict) -> dict:
    """Add computed properties to a node dict (from engine response)."""
    return {
        "id": node_id,
        "densidad": props.get("densidad", 0),
        "simetria": bool(props.get("simetria")),
        "tipo_geom": props.get("tipo_geom", 0),
        "radio": props.get("radio", 0),
        "clase_dinamica": props.get("clase_dinamica", 0),
        "pasos_atractor": props.get("pasos_atractor", -1),
        "atractor_id": props.get("atractor_id", 0),
    }


_GEOM_NAMES = {0: "CORNER", 1: "EDGE", 2: "INTERIOR"}
_DYN_NAMES = {0: "ATTRACTOR", 1: "PRE_ATTRACTOR", 2: "TRANSIENT"}


def props_str(props: dict) -> str:
    g = _GEOM_NAMES.get(props.get("tipo_geom", 0), "?")
    d = _DYN_NAMES.get(props.get("clase_dinamica", 0), "?")
    return (
        f"D:{props.get('densidad',0)} "
        f"S:{'Y' if props.get('simetria') else 'N'} "
        f"G:{g} "
        f"R:{props.get('radio',0)} "
        f"{d} "
        f"A:{props.get('atractor_id',0)}"
    )

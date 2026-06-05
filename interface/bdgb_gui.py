import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import json
import struct
import os
import subprocess
from datetime import datetime

DATA_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "data")
AGENTS_PATH = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "agents")
REGISTRY = os.path.join(AGENTS_PATH, "registry.json")

NODE_FILE = os.path.join(DATA_PATH, "nodes.dat")

COLORS = {
    "bg": "#1a1a2e",
    "surface": "#16213e",
    "primary": "#0f3460",
    "accent": "#e94560",
    "text": "#eaeaea",
    "text_dim": "#888",
    "green": "#2ecc71",
    "yellow": "#f1c40f",
    "blue": "#3498db",
}

GRID_SIZE = 4

class BDGBGui:
    def __init__(self, root):
        self.root = root
        self.root.title("BDGB - Base de Datos Geométrica Binaria")
        self.root.geometry("1200x750")
        self.root.configure(bg=COLORS["bg"])

        self.node_data = {}
        self.agents = []
        self.selected_node = None

        self.load_data()
        self.build_ui()

    def load_data(self):
        self.node_data = {}
        if os.path.exists(NODE_FILE):
            with open(NODE_FILE, "rb") as f:
                raw = f.read()
                for i in range(0, len(raw), 4):
                    if i + 4 <= len(raw):
                        node_id = raw[i]
                        x = raw[i + 1]
                        y = raw[i + 2]
                        flags = raw[i + 3]
                        bits = f"{node_id:04b}"
                        ones = bin(node_id).count("1")
                        symmetric = bits == bits[::-1]
                        geom = "CORNER" if (x in (0, 3) and y in (0, 3)) else ("EDGE" if (x in (0, 3) or y in (0, 3)) else "INTERIOR")
                        self.node_data[node_id] = {
                            "id": node_id, "x": x, "y": y, "bits": bits,
                            "densidad": ones, "simetria": symmetric, "tipo_geom": geom,
                            "radio": self._calc_radio(x, y)
                        }

        self.agents = []
        if os.path.exists(REGISTRY):
            with open(REGISTRY, "r") as f:
                data = json.load(f)
                self.agents = data.get("agentes", [])

    def _calc_radio(self, x, y):
        dx = 2 * x - 3
        dy = 2 * y - 3
        dist2 = dx * dx + dy * dy
        if dist2 == 0: return 0
        if dist2 <= 8: return 1
        return 2

    def build_ui(self):
        main_frame = tk.Frame(self.root, bg=COLORS["bg"])
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        left = tk.Frame(main_frame, bg=COLORS["surface"], bd=1, relief=tk.RAISED)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        right = tk.Frame(main_frame, bg=COLORS["surface"], bd=1, relief=tk.RAISED)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(5, 0))

        self.build_grid(left)
        self.build_info(right)
        self.build_agents(right)
        self.build_search(right)

    def build_grid(self, parent):
        header = tk.Label(parent, text="CUADRÍCULA 4x4", font=("Consolas", 14, "bold"),
                          bg=COLORS["surface"], fg=COLORS["accent"])
        header.pack(pady=(10, 5))

        grid_frame = tk.Frame(parent, bg=COLORS["surface"])
        grid_frame.pack(pady=10)

        for y in range(GRID_SIZE):
            row_frame = tk.Frame(grid_frame, bg=COLORS["surface"])
            row_frame.pack()
            for x in range(GRID_SIZE):
                node_id = (y << 2) | x
                data = self.node_data.get(node_id, {})
                self._draw_cell(row_frame, node_id, data, x, y)

        legend = tk.Frame(parent, bg=COLORS["surface"])
        legend.pack(pady=10)
        items = [
            ("#2ecc71", "ATRACTOR"), ("#f39c12", "PRE-ATRACTOR"), ("#e94560", "SIMÉTRICO")
        ]
        for color, label in items:
            f = tk.Frame(legend, bg=COLORS["surface"])
            f.pack(side=tk.LEFT, padx=10)
            tk.Canvas(f, width=12, height=12, bg=color, highlightthickness=0,
                      bd=0).pack(side=tk.LEFT)
            tk.Label(f, text=label, font=("Consolas", 9), bg=COLORS["surface"],
                     fg=COLORS["text"]).pack(side=tk.LEFT, padx=3)

    def _draw_cell(self, parent, node_id, data, x, y):
        is_sym = data.get("simetria", False)
        densidad = data.get("densidad", 0)

        if node_id in (0, 7, 9):
            bg = "#2ecc71"
        elif is_sym:
            bg = "#e94560"
        else:
            bg = COLORS["primary"]

        cell = tk.Frame(parent, width=70, height=70, bg=bg, bd=1, relief=tk.RAISED,
                        cursor="hand2")
        cell.pack(side=tk.LEFT, padx=2, pady=2)
        cell.pack_propagate(False)

        inner = tk.Frame(cell, bg=bg)
        inner.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

        tk.Label(inner, text=f"#{node_id}", font=("Consolas", 10, "bold"),
                bg=bg, fg="white").pack()
        tk.Label(inner, text=f"({x},{y})", font=("Consolas", 8),
                bg=bg, fg="#ddd").pack()
        tk.Label(inner, text=f"D:{densidad}", font=("Consolas", 7),
                bg=bg, fg="#ccc").pack()

        cell.bind("<Button-1>", lambda e, nid=node_id: self.on_node_click(nid))

    def build_info(self, parent):
        self.info_frame = tk.Frame(parent, bg=COLORS["surface"])
        self.info_frame.pack(fill=tk.BOTH, padx=10, pady=(10, 5))

        tk.Label(self.info_frame, text="NODO SELECCIONADO",
                font=("Consolas", 12, "bold"), bg=COLORS["surface"],
                fg=COLORS["accent"]).pack(anchor="w")

        self.info_text = scrolledtext.ScrolledText(
            self.info_frame, height=6, font=("Consolas", 10),
            bg=COLORS["bg"], fg=COLORS["text"], insertbackground=COLORS["text"])
        self.info_text.pack(fill=tk.BOTH, pady=(5, 0))
        self.info_text.insert(tk.END, "Haz clic en un nodo de la cuadrícula")
        self.info_text.config(state=tk.DISABLED)

    def build_agents(self, parent):
        frame = tk.Frame(parent, bg=COLORS["surface"])
        frame.pack(fill=tk.BOTH, padx=10, pady=(10, 5))

        tk.Label(frame, text="AGENTES ACTIVOS", font=("Consolas", 12, "bold"),
                bg=COLORS["surface"], fg=COLORS["accent"]).pack(anchor="w")

        self.agent_tree = ttk.Treeview(frame, columns=("nombre", "estado", "ejec"),
                                       show="headings", height=4)
        self.agent_tree.heading("nombre", text="Agente")
        self.agent_tree.heading("estado", text="Estado")
        self.agent_tree.heading("ejec", text="Ejec.")

        style = ttk.Style()
        style.theme_use("default")
        style.configure("Treeview", background=COLORS["bg"], foreground=COLORS["text"],
                        fieldbackground=COLORS["bg"], font=("Consolas", 9))
        style.configure("Treeview.Heading", font=("Consolas", 9, "bold"))

        self.agent_tree.pack(fill=tk.BOTH, pady=(5, 5))

        for a in self.agents:
            self.agent_tree.insert("", tk.END,
                values=(a.get("nombre", a["id"]), a.get("estado", "?"), a.get("metricas", {}).get("ejecuciones", 0)))

        btn_frame = tk.Frame(frame, bg=COLORS["surface"])
        btn_frame.pack(fill=tk.X)

        tk.Button(btn_frame, text="▶ Ejecutar", font=("Consolas", 9),
                  bg=COLORS["green"], fg="white", relief=tk.FLAT,
                  command=self.run_selected_agent).pack(side=tk.LEFT, padx=(0, 5))
        tk.Button(btn_frame, text="↻ Recargar", font=("Consolas", 9),
                  bg=COLORS["blue"], fg="white", relief=tk.FLAT,
                  command=self.reload_agents).pack(side=tk.LEFT)

    def build_search(self, parent):
        frame = tk.Frame(parent, bg=COLORS["surface"])
        frame.pack(fill=tk.BOTH, padx=10, pady=(10, 5))

        tk.Label(frame, text="BÚSQUEDA EN LENGUAJE NATURAL",
                font=("Consolas", 12, "bold"), bg=COLORS["surface"],
                fg=COLORS["accent"]).pack(anchor="w")

        self.search_var = tk.StringVar(value="belleza simetrico")
        entry = tk.Entry(frame, textvariable=self.search_var,
                        font=("Consolas", 12), bg=COLORS["bg"], fg=COLORS["text"],
                        insertbackground=COLORS["text"], bd=2)
        entry.pack(fill=tk.X, pady=(5, 5))
        entry.bind("<Return>", lambda e: self.do_search())

        tk.Button(frame, text="🔍 Buscar", font=("Consolas", 10, "bold"),
                  bg=COLORS["accent"], fg="white", relief=tk.FLAT,
                  command=self.do_search).pack()

        self.result_text = scrolledtext.ScrolledText(
            frame, height=6, font=("Consolas", 10),
            bg=COLORS["bg"], fg=COLORS["text"], insertbackground=COLORS["text"])
        self.result_text.pack(fill=tk.BOTH, pady=(5, 0))

    def on_node_click(self, node_id):
        self.selected_node = node_id
        data = self.node_data.get(node_id, {})

        bits_str = "".join(str((node_id >> i) & 1) for i in range(3, -1, -1))

        semantics_map = {}
        sem_file = os.path.join(DATA_PATH, "semantics.dat")
        if os.path.exists(sem_file):
            with open(sem_file, "rb") as f:
                raw = f.read()
                for i in range(0, len(raw), 5):
                    if i + 5 <= len(raw):
                        nid = raw[i]
                        cid = struct.unpack("<H", raw[i+1:i+3])[0]
                        if nid == node_id:
                            concepts = {1001: "belleza", 2002: "resistencia", 3003: "volumen", 4004: "simetria"}
                            semantics_map[cid] = concepts.get(cid, f"concepto_{cid}")

        text = f"▸ NODO #{node_id}\n"
        text += f"  Bits: {bits_str}  |  Coord: ({data.get('x','?')},{data.get('y','?')})\n"
        text += f"  Densidad: {data.get('densidad', '?')}  |  Radio: {data.get('radio', '?')}\n"
        text += f"  Simetría: {'SI' if data.get('simetria') else 'NO'}\n"
        text += f"  Tipo geom: {data.get('tipo_geom', '?')}\n"
        text += f"  Atractor: {'0' if node_id == 0 else ('7' if node_id == 7 else ('9' if node_id == 9 else '?'))}\n"
        if semantics_map:
            text += f"  Conceptos: {', '.join(f'{k}({v})' for k, v in semantics_map.items())}\n"

        self.info_text.config(state=tk.NORMAL)
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(tk.END, text)
        self.info_text.config(state=tk.DISABLED)

    def do_search(self):
        query = self.search_var.get().strip()
        if not query:
            return

        concept_map = {"belleza": 1001, "resistencia": 2002, "volumen": 3003, "simetria": 4004}
        dyn_map = {"estable": [0, 7, 9], "atractor0": [0], "atractor7": [7], "atractor9": [9],
                   "cerca0": [0], "cerca7": [7], "cerca9": [9]}
        prop_map = {"simetrico": "simetria", "simetrica": "simetria", "denso": "densidad",
                    "densa": "densidad", "interior": "interior", "esquina": "esquina",
                    "borde": "borde", "estable": "estable"}

        tokens = query.lower().split()
        concept = 0
        dyn_filter = -1
        prop_filter = None

        for t in tokens:
            if t in concept_map: concept = concept_map[t]
            if t in dyn_map: dyn_filter = dyn_map[t][0]
            if t in prop_map: prop_filter = prop_map[t]

        results = []
        for nid, data in self.node_data.items():
            score = 100
            hit = True

            if concept:
                sem_file = os.path.join(DATA_PATH, "semantics.dat")
                found = False
                if os.path.exists(sem_file):
                    with open(sem_file, "rb") as f:
                        raw = f.read()
                        for i in range(0, len(raw), 5):
                            if i + 5 <= len(raw) and raw[i] == nid:
                                cid = struct.unpack("<H", raw[i+1:i+3])[0]
                                if cid == concept: found = True
                if not found: hit = False

            if dyn_filter >= 0 and nid not in [0, 7, 9]:
                hit = False
            if dyn_filter >= 0 and nid == dyn_filter:
                score += 150
            elif nid in (0, 7, 9):
                score += 150

            if prop_filter == "simetria" and not data.get("simetria"): hit = False
            if prop_filter == "simetria" and data.get("simetria"): score += 100
            if prop_filter == "densidad" and data.get("densidad", 0) < 3: hit = False
            if prop_filter == "densidad" and data.get("densidad", 0) >= 3: score += 80
            if prop_filter == "interior" and data.get("tipo_geom") != "INTERIOR": hit = False
            if prop_filter == "esquina" and data.get("tipo_geom") != "CORNER": hit = False
            if prop_filter == "borde" and data.get("tipo_geom") != "EDGE": hit = False

            if hit:
                results.append((nid, score))

        results.sort(key=lambda r: -r[1])

        self.result_text.config(state=tk.NORMAL)
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, f"Consulta: '{query}'  →  {len(results)} resultados\n\n")
        for nid, score in results:
            data = self.node_data.get(nid, {})
            bits = "".join(str((nid >> i) & 1) for i in range(3, -1, -1))
            self.result_text.insert(tk.END,
                f"  #{nid}  {bits}  score={score:3d}  "
                f"sim={'S' if data.get('simetria') else 'N'}  "
                f"dens={data.get('densidad',0)}  {data.get('tipo_geom','?')}\n")
        self.result_text.config(state=tk.DISABLED)

    def run_selected_agent(self):
        sel = self.agent_tree.selection()
        if not sel:
            messagebox.showwarning("BDGB", "Selecciona un agente primero")
            return
        item = self.agent_tree.item(sel[0])
        agent_name = item["values"][0]

        agent_id = None
        for a in self.agents:
            if a.get("nombre", a["id"]) == agent_name:
                agent_id = a["id"]
                break

        if agent_id:
            config_path = os.path.join(AGENTS_PATH, agent_id, "config.json")
            if os.path.exists(config_path):
                with open(config_path, "r") as f:
                    cfg = json.load(f)
                pipeline = cfg.get("pipeline", {})
                steps = list(pipeline.keys())
                text = f"▶ Ejecutando: {agent_name}\n"
                for i, step in enumerate(steps):
                    text += f"   {i+1}. {step}\n"
                text += "\n[Simulado] Pipeline completado."
                self.result_text.config(state=tk.NORMAL)
                self.result_text.delete(1.0, tk.END)
                self.result_text.insert(tk.END, text)
                self.result_text.config(state=tk.DISABLED)
            else:
                messagebox.showerror("BDGB", f"Config no encontrado para {agent_id}")

    def reload_agents(self):
        self.load_data()
        for item in self.agent_tree.get_children():
            self.agent_tree.delete(item)
        for a in self.agents:
            self.agent_tree.insert("", tk.END,
                values=(a.get("nombre", a["id"]), a.get("estado", "?"),
                        a.get("metricas", {}).get("ejecuciones", 0)))

if __name__ == "__main__":
    root = tk.Tk()
    app = BDGBGui(root)
    root.mainloop()

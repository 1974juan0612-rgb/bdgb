import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bdgb_bridge import (search, export_nodes, agent_run,
                         load_nodes_from_disk, decode_props, props_str,
                         BDGB_ROOT, DATA_PATH)

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


def _load_agents():
    registry_path = os.path.join(BDGB_ROOT, "glifos", "registry.json")
    try:
        with open(registry_path, "r") as f:
            data = json.load(f)
        return data.get("glifos", [])
    except Exception:
        return []


class BDGBGui:
    def __init__(self, root):
        self.root = root
        self.root.title("BDGB - Base de Datos Geométrica Binaria")
        self.root.geometry("1200x750")
        self.root.configure(bg=COLORS["bg"])

        self.node_data = {}
        self.engine_nodes = []
        self.agents = _load_agents()
        self.selected_node = None

        self.load_data()
        self.build_ui()

    def load_data(self):
        self.node_data = load_nodes_from_disk()
        r = export_nodes()
        self.engine_nodes = r.get("nodes", []) if isinstance(r, dict) else []

    def merge_props(self, node_id):
        for n in self.engine_nodes:
            if n["id"] == node_id:
                return decode_props(node_id, n)
        return None

    def build_ui(self):
        main_frame = tk.Frame(self.root, bg=COLORS["bg"])
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        left = tk.Frame(main_frame, bg=COLORS["surface"], bd=1, relief=tk.RAISED)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        right = tk.Frame(main_frame, bg=COLORS["surface"], bd=1, relief=tk.RAISED)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(5, 0))

        self.header_var = tk.StringVar(value="CUADRÍCULA 16×16 (256 nodos)")
        header = tk.Label(left, textvariable=self.header_var,
                          font=("Consolas", 14, "bold"),
                          bg=COLORS["surface"], fg=COLORS["accent"])
        header.pack(pady=(10, 5))

        self.build_grid(left)
        self.build_info(right)
        self.build_agents(right)
        self.build_search(right)

    def build_grid(self, parent):
        canvas_frame = tk.Frame(parent, bg=COLORS["surface"])
        canvas_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.grid_canvas = tk.Canvas(canvas_frame, bg=COLORS["bg"],
                                     highlightthickness=0)
        vbar = tk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.grid_canvas.yview)
        hbar = tk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.grid_canvas.xview)
        self.grid_canvas.configure(yscrollcommand=vbar.set, xscrollcommand=hbar.set)

        vbar.pack(side=tk.RIGHT, fill=tk.Y)
        hbar.pack(side=tk.BOTTOM, fill=tk.X)
        self.grid_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.grid_inner = tk.Frame(self.grid_canvas, bg=COLORS["bg"])
        self.grid_canvas.create_window((0, 0), window=self.grid_inner, anchor="nw")

        cells = []
        for y in range(16):
            row_frame = tk.Frame(self.grid_inner, bg=COLORS["bg"])
            row_frame.pack()
            for x in range(16):
                node_id = (y << 4) | x
                props = self.merge_props(node_id)
                cell = self._make_cell(row_frame, node_id, props, x, y)
                cells.append((node_id, cell))

        self.grid_canvas.update_idletasks()
        self.grid_canvas.config(scrollregion=self.grid_canvas.bbox("all"))

        legend = tk.Frame(parent, bg=COLORS["surface"])
        legend.pack(pady=5)
        items = [
            ("#2ecc71", "ATRACTOR"), ("#f39c12", "PRE-ATRACT"),
            ("#e94560", "SIMÉTRICO"), ("#0f3460", "OTRO"),
        ]
        for color, label in items:
            f = tk.Frame(legend, bg=COLORS["surface"])
            f.pack(side=tk.LEFT, padx=5)
            tk.Canvas(f, width=12, height=12, bg=color, highlightthickness=0,
                      bd=0).pack(side=tk.LEFT)
            tk.Label(f, text=label, font=("Consolas", 9), bg=COLORS["surface"],
                     fg=COLORS["text"]).pack(side=tk.LEFT, padx=3)

        self.legend = legend

    def _cell_color(self, props):
        if not props:
            return COLORS["primary"]
        if props.get("clase_dinamica") == 0:
            return "#2ecc71"
        if props.get("simetria"):
            return "#e94560"
        if props.get("clase_dinamica") == 1:
            return "#f39c12"
        return COLORS["primary"]

    def _make_cell(self, parent, node_id, props, x, y):
        bg = self._cell_color(props)
        cell = tk.Frame(parent, width=48, height=48, bg=bg, bd=1,
                        relief=tk.RAISED, cursor="hand2")
        cell.pack(side=tk.LEFT, padx=1, pady=1)
        cell.pack_propagate(False)

        inner = tk.Frame(cell, bg=bg)
        inner.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

        tk.Label(inner, text=f"#{node_id}", font=("Consolas", 7, "bold"),
                bg=bg, fg="white").pack()
        if props:
            d = props.get("densidad", 0)
            s = "S" if props.get("simetria") else ""
            tk.Label(inner, text=f"D{d}{s}", font=("Consolas", 6),
                    bg=bg, fg="#ddd").pack()

        cell.bind("<Button-1>", lambda e, nid=node_id: self.on_node_click(nid))
        return cell

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
                values=(a.get("nombre", a["id"]), a.get("estado", "?"),
                        a.get("metricas", {}).get("ejecuciones", 0)))

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
            frame, height=8, font=("Consolas", 10),
            bg=COLORS["bg"], fg=COLORS["text"], insertbackground=COLORS["text"])
        self.result_text.pack(fill=tk.BOTH, pady=(5, 0))

        tk.Label(frame, text="(búsqueda vía motor C nativo)",
                font=("Consolas", 7), bg=COLORS["surface"],
                fg=COLORS["text_dim"]).pack()

    def on_node_click(self, node_id):
        self.selected_node = node_id
        raw = self.node_data.get(node_id, {})
        props = self.merge_props(node_id)

        bits = raw.get("bits", format(node_id, "08b"))
        text = f"▸ NODO #{node_id}\n"
        text += f"  Bits: {bits}  |  Coord: ({raw.get('x','?')},{raw.get('y','?')})\n"

        if props:
            text += f"  Densidad: {props.get('densidad',0)}  |  Radio: {props.get('radio',0)}\n"
            text += f"  Simetría: {'SI' if props.get('simetria') else 'NO'}\n"
            geom_names = {0: "CORNER", 1: "EDGE", 2: "INTERIOR"}
            dyn_names = {0: "ATRACTOR", 1: "PRE-ATRACTOR", 2: "TRANSIENTE"}
            text += f"  Tipo geom: {geom_names.get(props.get('tipo_geom',0),'?')}\n"
            text += f"  Clase din: {dyn_names.get(props.get('clase_dinamica',0),'?')}\n"
            text += f"  Atractor: #{props.get('atractor_id','?')} ({props.get('pasos_atractor','?')} steps)\n"

        self.info_text.config(state=tk.NORMAL)
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(tk.END, text)
        self.info_text.config(state=tk.DISABLED)

    def do_search(self):
        query = self.search_var.get().strip()
        if not query:
            return

        r = search(query)
        if isinstance(r, dict) and "results" in r:
            results = r["results"]
        else:
            err = r.get("error", "unknown") if isinstance(r, dict) else "invalid response"
            self.result_text.config(state=tk.NORMAL)
            self.result_text.delete(1.0, tk.END)
            self.result_text.insert(tk.END, f"Error del motor C: {err}\n")
            self.result_text.config(state=tk.DISABLED)
            return

        out = f"Consulta: '{query}'  →  {len(results)} resultados (motor C)\n\n"
        for res in results:
            nid = res.get("node_id", 0)
            score = res.get("score", 0)
            d = res.get("densidad", 0)
            s = "S" if res.get("simetria") else "N"
            geom_names = {0: "CRN", 1: "EDG", 2: "INT"}
            g = geom_names.get(res.get("tipo_geom"), "?")
            a = res.get("atractor_id", 0)
            bits = format(nid, "08b")
            out += f"  #{nid:3d} {bits}  score={score:3d}  "
            out += f"D{d} {s} {g} A{a}\n"

        self.result_text.config(state=tk.NORMAL)
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, out)
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
        if not agent_id:
            return

        self.result_text.config(state=tk.NORMAL)
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, f"▶ Ejecutando agente '{agent_id}' vía motor C...\n\n")
        self.result_text.config(state=tk.DISABLED)
        self.root.update()

        r = agent_run(agent_id)
        self.result_text.config(state=tk.NORMAL)
        if isinstance(r, dict) and "error" in r:
            self.result_text.insert(tk.END, f"  Error: {r['error']}\n")
        else:
            self.result_text.insert(tk.END, "  Agente ejecutado (ver consola para detalles)\n")
        self.result_text.config(state=tk.DISABLED)

    def reload_agents(self):
        self.agents = _load_agents()
        self.load_data()
        for item in self.agent_tree.get_children():
            self.agent_tree.delete(item)
        for a in self.agents:
            self.agent_tree.insert("", tk.END,
                values=(a.get("nombre", a["id"]), a.get("estado", "?"),
                        a.get("metricas", {}).get("ejecuciones", 0)))
        self.header_var.set(f"CUADRÍCULA 16×16 ({len(self.node_data)} nodos)")


if __name__ == "__main__":
    root = tk.Tk()
    app = BDGBGui(root)
    root.mainloop()

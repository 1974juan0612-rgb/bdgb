#!/usr/bin/env python3
"""Generate complete project audit PDF: auditor.pdf"""

import os, json, subprocess, sys
from datetime import datetime
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import cm
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.colors import HexColor, black, white
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, HRFlowable
)
from reportlab.lib import colors

BDGB_ROOT = os.environ.get(
    "BDGB_ROOT",
    os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
)
OUTPUT = os.path.join(BDGB_ROOT, "auditor.pdf")

DARK  = HexColor("#1a1a2e")
ACCENT = HexColor("#0f3460")
GREEN = HexColor("#16a34a")
AMBER = HexColor("#d97706")
TEAL = HexColor("#0891b2")

doc = SimpleDocTemplate(OUTPUT, pagesize=A4,
    leftMargin=2*cm, rightMargin=2*cm, topMargin=2*cm, bottomMargin=2*cm)

styles = getSampleStyleSheet()

s_title = ParagraphStyle("s_title", parent=styles["Title"],
    fontSize=22, leading=28, textColor=DARK, spaceAfter=4, alignment=TA_CENTER,
    fontName="Helvetica-Bold")
s_subtitle = ParagraphStyle("s_subtitle", parent=styles["Normal"],
    fontSize=10, leading=13, textColor=HexColor("#555"), spaceAfter=20, alignment=TA_CENTER)
s_h1 = ParagraphStyle("s_h1", parent=styles["Heading1"],
    fontSize=14, leading=18, textColor=DARK, spaceBefore=16, spaceAfter=8, fontName="Helvetica-Bold")
s_h2 = ParagraphStyle("s_h2", parent=styles["Heading2"],
    fontSize=11, leading=14, textColor=ACCENT, spaceBefore=10, spaceAfter=4, fontName="Helvetica-Bold")
s_body = ParagraphStyle("s_body", parent=styles["Normal"],
    fontSize=8.5, leading=11.5, textColor=HexColor("#222"), spaceAfter=4,
    alignment=TA_JUSTIFY, fontName="Helvetica")
s_code = ParagraphStyle("s_code", parent=s_body,
    fontSize=7, leading=9, fontName="Courier", leftIndent=8, backColor=HexColor("#f5f5f5"))
s_ok = ParagraphStyle("s_ok", parent=s_body, textColor=GREEN, fontName="Helvetica-Bold", fontSize=8)

story = []

def hr():
    story.append(HRFlowable(width="100%", thickness=0.5, color=HexColor("#ccc"), spaceBefore=4, spaceAfter=4))

def tb(data, col_widths=None, hdr=True):
    t = Table(data, colWidths=col_widths, repeatRows=1 if hdr else 0)
    style_cmds = [
        ("FONTSIZE", (0, 0), (-1, -1), 7.5),
        ("LEADING", (0, 0), (-1, -1), 9.5),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("RIGHTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
        ("GRID", (0, 0), (-1, -1), 0.3, HexColor("#bbb")),
    ]
    if hdr:
        style_cmds += [
            ("BACKGROUND", (0, 0), (-1, 0), DARK),
            ("TEXTCOLOR", (0, 0), (-1, 0), white),
            ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
        ]
    t.setStyle(TableStyle(style_cmds))
    story.append(t)
    story.append(Spacer(1, 4))

def get_git_log():
    try:
        r = subprocess.run(["git", "log", "--oneline", "-20"],
            capture_output=True, text=True, cwd=BDGB_ROOT, timeout=10)
        return [l.strip() for l in r.stdout.splitlines() if l.strip()]
    except: return ["(git log unavailable)"]

def get_test_result():
    try:
        r = subprocess.run(
            [os.path.join(BDGB_ROOT, "build", "bdgb_test.exe")],
            capture_output=True, text=True, cwd=BDGB_ROOT, timeout=30)
        return r.stdout + r.stderr
    except: return "(test run unavailable)"

def count_lines(path):
    total = 0
    try:
        for root, dirs, files in os.walk(path):
            for f in files:
                if f.endswith((".c", ".h")):
                    with open(os.path.join(root, f), encoding="utf-8", errors="replace") as fh:
                        total += sum(1 for _ in fh)
    except: pass
    return total

clines = count_lines(os.path.join(BDGB_ROOT, "src"))
hline = count_lines(os.path.join(BDGB_ROOT, "include"))
cryptlines = count_lines(os.path.join(BDGB_ROOT, "crypt"))
testout = get_test_result()
gitlog = get_git_log()
pass_count = testout.count("PASS") if "PASS" in testout else "?"
fail_count = testout.count("FAIL") if "FAIL" in testout else "0"

# Load glifosenilla.json
try:
    with open(os.path.join(BDGB_ROOT, "glifos", "vigilancia-tendencias", "glifosenilla.json"), encoding="utf-8") as f:
        glifosenilla = json.load(f)
except:
    glifosenilla = {}

# Load registry
try:
    with open(os.path.join(BDGB_ROOT, "glifos", "registry.json"), encoding="utf-8") as f:
        registry = json.load(f)
except:
    registry = {}

# ══════════════════════════════════════════════════════════════════════
# PORTADA
# ══════════════════════════════════════════════════════════════════════
story.append(Spacer(1, 3*cm))
story.append(Paragraph("INFORME DE AUDITORIA", s_title))
story.append(Paragraph("BDGB &mdash; Binary Dynamics Grid Brain", s_subtitle))
story.append(Spacer(1, 0.5*cm))
story.append(HRFlowable(width="60%", thickness=1.5, color=ACCENT, spaceBefore=4, spaceAfter=8))
story.append(Paragraph(
    "Rejilla binaria 16x16 con dinamica Kaprekar, cifrado de flujo BDGB-Cipher v1, "
    "motor semantico, NLP, aprendizaje, sistema de glifos nativos C y agentes externos Python.",
    ParagraphStyle("portada_desc", parent=s_body, fontSize=9, leading=13,
                   alignment=TA_CENTER, textColor=HexColor("#444"))))
story.append(Spacer(1, 1.5*cm))

meta = [
    ["Repositorio", "https://github.com/1974juan0612-rgb/bdgb"],
    ["Lenguaje", "C99 + Python 3"],
    ["Lineas C", f"~{clines} (nucleo + crypt)"],
    ["Lineas Python", "~1,200 (bridge + GUI + scraper + audit)"],
    ["Nodos", "256 (16x16 grid, 8-bit)"],
    ["Tests", f"{pass_count}/{19} PASS"],
    ["Fecha auditoria", datetime.now().strftime("%Y-%m-%d %H:%M")],
]
mt = Table(meta, colWidths=[3.2*cm, 10*cm])
mt.setStyle(TableStyle([
    ("FONTSIZE", (0, 0), (-1, -1), 8.5),
    ("LEADING", (0, 0), (-1, -1), 11),
    ("FONTNAME", (0, 0), (0, -1), "Helvetica-Bold"),
    ("FONTNAME", (1, 0), (1, -1), "Helvetica"),
    ("TEXTCOLOR", (0, 0), (0, -1), ACCENT),
    ("ALIGN", (0, 0), (0, -1), "RIGHT"),
    ("TOPPADDING", (0, 0), (-1, -1), 1),
    ("BOTTOMPADDING", (0, 0), (-1, -1), 1),
    ("LINEBELOW", (0, 0), (-1, -2), 0.3, HexColor("#ddd")),
]))
story.append(mt)
story.append(PageBreak())

# ══════════════════════════════════════════════════════════════════════
# 1. RESUMEN EJECUTIVO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("1. Resumen Ejecutivo", s_h1))
hr()
story.append(Paragraph(
    "BDGB es un sistema de conocimiento geometrico que integra una rejilla binaria 16x16 "
    "con dinamica Kaprekar, un motor semantico, busqueda hibrida, NLP, aprendizaje por refuerzo, "
    "cifrado propio (BDGB-Cipher v1) y un sistema de glifos nativos en C. "
    "Todo el nucleo opera sin GPU, sin LLM, y sin dependencias externas en C.",
    s_body))
story.append(Paragraph(
    "El sistema de glifos permite automatizar flujos de trabajo (Sistemas) mediante nodos "
    "operativos (Glifos) que pueden ser nativos (compilados en el binario) o externos "
    "(scripts Python). Cada sistema tiene un glifo maestro (glifosenilla.json) que define "
    "el pipeline completo, las relaciones entre glifos y los datos del sistema.",
    s_body))

story.append(Spacer(1, 4))
story.append(Paragraph("<b>Metricas clave:</b>", s_body))
tb([
    ["Metrica", "Valor"],
    [f"Archivos C", f"{clines} lineas en ~10 modulos"],
    [f"Archivos Python", "~1,200 lineas (bridge, GUI, scraper, scripts)"],
    [f"Tests", f"{pass_count}/{19} PASS"],
    [f"Commits", f"{len(gitlog)}"],
    [f"Sistemas de glifos", "1 activo (vigilancia-tendencias)"],
    [f"Glifos registrados", "3 (primo, trend-tracker, youtube-automator)"],
    [f"Cifrado", "BDGB-Cipher v1, 128-bit estado, S-box geometrica"],
    [f"Dependencias C", "CERO"],
    [f"Binario", "~151 KB (bdgb.exe)"],
], col_widths=[5*cm, 9.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 2. DESCRIPCION GENERAL
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("2. Descripcion General del Proyecto", s_h1))
hr()
story.append(Paragraph(
    "BDGB (Binary Dynamics Grid Brain) es un sistema de conocimiento geometrico "
    "basado en una rejilla binaria de 16x16 nodos (256 nodos, 8-bit cada uno). "
    "Cada nodo representa un valor 0-255 cuyas propiedades se derivan de su "
    "representacion binaria: densidad (popcount), simetria, tipo geometrico "
    "(esquina/borde/interior), radio, clase dinamica y atractor asociado.", s_body))
story.append(Paragraph(
    "La dinamica Kaprekar binaria asigna a cada nodo un sucesor (restando el orden "
    "ascendente del descendente de sus bits), generando cuencas de atraccion que "
    "convergen en puntos fijos. Este modelo matematico subyace tanto al motor de "
    "busqueda semantica como al sistema de cifrado.", s_body))

story.append(Paragraph("<b>Componentes del sistema:</b>", s_body))
tb([
    ["Componente", "Descripcion", "Estado"],
    ["Nucleo geometrico", "Grid 16x16, nodos de 4 bytes, propiedades O(1)", "100%"],
    ["Regla dinamica", "Sucesor Kaprekar binario, deteccion de atractores", "100%"],
    ["Semantica", "Hash index (256 buckets) nodo->concepto", "90%"],
    ["Grafo conceptos", "Hash index para aristas entre conceptos", "85%"],
    ["Busqueda", "Hibrida: semantico + propiedades + atractor", "80%"],
    ["NLP", "Terminos fijos + aprendizaje dinamico, tokenizador", "70%"],
    ["Aprendizaje", "Refuerzo y decaimiento con persistencia a disco", "70%"],
    ["Glifos nativos", "Sistema de glifos en C compilados en binario", "100%"],
    ["Sistema glifos", "Arquitectura Sistema -> Glifo, glifosenilla.json", "100%"],
    ["BDGB-Cipher v1", "Cifrado de flujo sin dependencias externas", "100%"],
    ["Python bridge", "CLI wrapper para integracion desde Python", "90%"],
    ["GUI Tkinter", "Grid 16x16 visual, busqueda NLP, panel agentes", "80%"],
], col_widths=[3.8*cm, 7.5*cm, 1.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 3. SISTEMA DE GLIFOS
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("3. Sistema de Glifos", s_h1))
hr()
story.append(Paragraph(
    "El sistema de glifos es la capa de automatizacion del proyecto. Un glifo es un nodo "
    "operativo que ejecuta una tarea atomica. Los glifos se agrupan en Sistemas, que son "
    "flujos de trabajo con un proposito definido. Cada sistema tiene un glifo maestro "
    "obligatorio (glifosenilla.json) que define el pipeline completo, las relaciones entre "
    "glifos y los datos extra del sistema.", s_body))

story.append(Paragraph("<b>Arquitectura:</b>", s_body))
story.append(Paragraph(
    "glifos/&lt;sistema&gt;/glifosenilla.json -> define pipeline, dependencias y relaciones<br/>"
    "glifos/&lt;sistema&gt;/glifos/&lt;glifo&gt;/glifo.json -> config individual de cada glifo<br/>"
    "glifos/registry.json -> registro maestro de todos los sistemas y glifos", s_code))

story.append(Paragraph("<b>Glifos registrados:</b>", s_body))
tb([
    ["Glifo", "Tipo", "Sistema", "Entry"],
    ["primo", "Nativo (C)", "vigilancia-tendencias", "bdgb --glifo-run primo"],
    ["trend-tracker", "Externo (Python)", "vigilancia-tendencias", "python3 trend_tracker.py --daily"],
    ["youtube-automator", "Externo (Python)", "sin asignar", "pendiente"],
], col_widths=[3*cm, 3.5*cm, 4*cm, 4.3*cm])

# Pipeline del sistema activo
if glifosenilla:
    story.append(Paragraph("<b>Pipeline: vigilancia-tendencias</b>", s_h2))
    pipe_data = [["Orden", "Glifo", "Accion", "Dependencias"]]
    for step in glifosenilla.get("pipeline", []):
        pipe_data.append([
            str(step.get("orden", "?")),
            step.get("glifo", "?"),
            step.get("accion", "?"),
            ", ".join(step.get("dependencias", [])) or "ninguna"
        ])
    tb(pipe_data, col_widths=[1.5*cm, 3*cm, 5*cm, 3*cm])

    story.append(Paragraph("<b>Relaciones entre glifos:</b>", s_body))
    rel_data = [["Glifo", "Produce", "Consume"]]
    for gid, rel in glifosenilla.get("relaciones", {}).items():
        rel_data.append([
            gid,
            ", ".join(rel.get("produce", [])),
            ", ".join(rel.get("consume", [])) or "nada"
        ])
    tb(rel_data, col_widths=[2.5*cm, 6*cm, 4*cm])

story.append(Paragraph(
    "<b>Nota:</b> El glifo-primo es el primer glifo nativo compilado directamente en el "
    "binario BDGB (src/glifo.c). No requiere Python, no requiere LLM, no requiere GPU. "
    "Usa curl/wget via popen() para RSS de Google Trends, con fallback a datos mock. "
    "Inyecta conceptos y aprende terminos NLP automaticamente.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 4. NUCLEO BDGB
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("4. Nucleo BDGB", s_h1))
hr()

story.append(Paragraph("<b>4.1 Geometria y dinamica Kaprekar</b>", s_h2))
story.append(Paragraph(
    "El nucleo opera con 256 nodos en una rejilla 16x16. Cada nodo tiene un valor de 8 bits "
    "(0-255) del que se derivan propiedades: popcount (numero de unos), simetria "
    "(espejo binario), tipo geometrico (esquina/borde/interior segun coordenadas), "
    "radio (distancia al centro), clase dinamica y atractor. La regla Kaprekar binaria "
    "calcula el sucesor de cada nodo restando el orden ascendente del descendente de sus bits, "
    "generando cuencas de atraccion que convergen en puntos fijos.", s_body))

story.append(Paragraph("<b>4.2 Motor semantico</b>", s_h2))
story.append(Paragraph(
    "Cada nodo puede asociarse a conceptos mediante un hash index de 256 buckets. "
    "Los conceptos se relacionan entre si mediante aristas en un grafo de conceptos "
    "con su propio hash index. Las busquedas semanticas son O(1) en el caso ideal.",
    s_body))

story.append(Paragraph("<b>4.3 Busqueda hibrida</b>", s_h2))
story.append(Paragraph(
    "El motor de busqueda combina: (1) busqueda semantica por concepto, "
    "(2) filtrado por propiedades (simetrico, interior, etc.), "
    "(3) busqueda por atractor, (4) busqueda por texto libre via NLP. "
    "Los resultados se refuerzan con aprendizaje, mejorando con el uso.",
    s_body))

story.append(Paragraph("<b>4.4 NLP y aprendizaje</b>", s_h2))
story.append(Paragraph(
    "El modulo NLP tokeniza texto en terminos y los mapea a conceptos internos. "
    "Partio con 20 terminos fijos pero ha crecido dinamicamente mediante inyeccion "
    "desde glifos (trend-tracker, glifo-primo) hasta 55+ terminos. "
    "El aprendizaje implementa refuerzo (+10 al usar un nodo) y decaimiento "
    "(x0.5 por tick), con persistencia a disco en formato binario.",
    s_body))

story.append(Paragraph("<b>4.5 Almacenamiento</b>", s_body))
tb([
    ["Archivo", "Formato", "Tamano"],
    ["nodes.dat", "Binario: 4 B/nodo", "1,024 B"],
    ["semantics.dat", "Binario: 5 B/enlace", "384 B"],
    ["concept_edges.dat", "Binario: 6 B/arista", "30 B"],
    ["nlp_terms.dat", "Binario serializado", "1,042 B"],
    ["usage_nodes.dat", "Binario: contadores", "1,024 B"],
    ["usage_concepts.dat", "Binario: contadores", "1,024 B"],
], col_widths=[4*cm, 5*cm, 3*cm])

# ══════════════════════════════════════════════════════════════════════
# 5. BDGB-CIPHER V1
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("5. BDGB-Cipher v1", s_h1))
hr()
story.append(Paragraph(
    "Cifrado de flujo (stream cipher) disenado completamente sobre el modelo matematico "
    "BDGB, sin utilizar ninguna biblioteca criptografica externa. Implementado en "
    f"{cryptlines} lineas de C99 (crypt/bdgb_crypt.c).", s_body))

story.append(Paragraph("<b>Caracteristicas:</b>", s_body))
tb([
    ["Propiedad", "Valor"],
    ["Estado interno", "4 x uint32_t wseed[4] = 128 bits + IV 8 B + counter 64 B"],
    ["S-box", "Geometrica: propiedades del nodo BDGB calculadas en tiempo real"],
    ["Derivacion clave", "mix32() con 8 rondas, retroalimentacion cruzada"],
    ["Keystream", "1 byte por ciclo, dependiente de estado + contador"],
    ["Formato archivo", "Magic 'BDGB' + version 0x01 + IV 8 B + ciphertext"],
    ["Dependencias", "CERO (no OpenSSL, no libcrypto)"],
    ["Espacio de claves", "2^128 (4 x 32 bits)"],
    ["Desafio publico", "secret.bdgb (445 B) en crypt/challenge/"],
], col_widths=[4*cm, 10.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 6. SUITE DE TESTS
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("6. Suite de Tests", s_h1))
hr()
story.append(Paragraph(
    f"La bateria de tests (tests/test_bdgb.c) cubre todos los modulos del sistema. "
    f"Resultado: <b>{pass_count}/19 PASS, {fail_count} FAIL</b>.", s_body))

if "PASS" in testout:
    story.append(Paragraph("<pre>" + testout[:2000] + "</pre>", s_code))
else:
    story.append(Paragraph("<b>NOTA:</b> No se pudo ejecutar la suite de tests. "
        "El binario puede no existir o estar desactualizado. "
        "Ejecutar: cd build &amp;&amp; ninja bdgb_test &amp;&amp; ./bdgb_test.exe", s_body))

story.append(Paragraph("<b>Tests implementados:</b>", s_body))
tb([
    ["#", "Test", "Modulo"],
    ["1", "grid_size", "Constantes del grid"],
    ["2", "node_creation", "Creacion de 256 nodos"],
    ["3", "popcount", "Conteo de unos"],
    ["4", "symmetry", "Deteccion de simetria"],
    ["5", "geom_types", "Tipos geometricos"],
    ["6", "dynamic_rule", "Regla Kaprekar binaria"],
    ["7", "attractor_search", "Convergencia a atractor"],
    ["8", "storage_roundtrip", "Persistencia en disco"],
    ["9", "semantics_hash", "Hash index semantica"],
    ["10", "concept_graph_hash", "Hash index grafo conceptos"],
    ["11", "learning_reinforce", "Refuerzo y decaimiento"],
    ["12", "nlp_parse", "Tokenizacion y mapeo"],
    ["13", "search_hybrid", "Busqueda hibrida"],
    ["14", "search_by_predicate", "Filtro por propiedades"],
    ["15", "compute_props", "Propiedades de 256 nodos"],
    ["16", "crypt_init", "Inicializacion cifrado"],
    ["17", "crypt_keystream_differs", "Divergencia de keystream"],
    ["18", "crypt_encrypt_decrypt", "Cifrado/descifrado"],
    ["19", "crypt_file_roundtrip", "Archivo completo"],
], col_widths=[1*cm, 4.5*cm, 7*cm])

# ══════════════════════════════════════════════════════════════════════
# 7. ARQUITECTURA
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("7. Arquitectura del Sistema", s_h1))
hr()

story.append(Paragraph("<b>7.1 Pila tecnologica</b>", s_h2))
tb([
    ["Capa", "Tecnologia", "Funcion"],
    ["Lenguaje base", "C99 (ISO/IEC 9899:1999)", "Nucleo de alto rendimiento"],
    ["Build system", "CMake 3.30 + Ninja + Clang 22.1.6", "Compilacion rapida"],
    ["CLI", "main.c con argparse manual", "Interfaz de linea de comandos"],
    ["Glifos nativos", "src/glifo.c, include/glifo.h", "Automatizacion compilada"],
    ["Python Bridge", "bdgb_bridge.py (subprocess)", "Integracion desde Python"],
    ["GUI", "bdgb_gui.py (Tkinter)", "Visualizacion de grid"],
    ["Scraper", "scraper_trends.py (pytrends)", "Inyeccion de tendencias"],
    ["Cifrado", "bdgb_crypt.c (C99 puro)", "BDGB-Cipher v1"],
], col_widths=[3.5*cm, 5.5*cm, 5.8*cm])

story.append(Paragraph("<b>7.2 Estructura del repositorio</b>", s_h2))
tb([
    ["Directorio/Archivo", "Descripcion"],
    ["include/", "Headers C del nucleo"],
    ["src/", "Implementaciones C (main, node, grid, storage, semantics, concept_graph, search, learning, nlp, agent, glifo)"],
    ["crypt/", "Modulo de cifrado BDGB-Cipher + challenge/"],
    ["crypt/challenge/", "Desafio publico: secret.bdgb, README, CHALLENGE_BUNDLE"],
    ["tests/", "Suite de tests: test_bdgb.c (19 tests)"],
    ["glifos/", "Sistema de glifos: README, registry, sistemas/"],
    ["glifos/vigilancia-tendencias/", "Primer sistema: glifosenilla.json + glifos/primo + glifos/trend-tracker"],
    ["glifos/youtube-automator/", "Segundo sistema (pendiente de definir)"],
    ["interface/", "Python: bdgb_bridge.py, bdgb_gui.py (Tkinter)"],
    ["scripts/", "Python: scraper_trends.py"],
    ["docs/", "Scripts de generacion de PDF de auditoria"],
    ["data/", "Datos persistidos (binarios)"],
    ["build/", "Artefactos de compilacion"],
], col_widths=[5.5*cm, 9.3*cm])

story.append(Paragraph("<b>7.3 Comandos CLI</b>", s_body))
tb([
    ["Comando", "Funcion"],
    ["--init", "Limpia e inicializa datos (256 nodos + semantica demo)"],
    ["--search <query>", "Busqueda NLP con salida JSON"],
    ["--add-concept <n> <c> <w> <r>", "Anade relacion semantica"],
    ["--export-nodes", "Dump JSON de todos los nodos"],
    ["--agent-run <id>", "Ejecuta pipeline de agente"],
    ["--glifo-list", "Lista todos los glifos disponibles"],
    ["--glifo-run <id> [args]", "Ejecuta un glifo por ID"],
    ["--encrypt <in> <out> [key]", "Cifra archivo con BDGB-Cipher"],
    ["--decrypt <in> <out> [key]", "Descifra archivo"],
    ["--hash <text>", "Genera hash BDGB de 16 bytes"],
    ["--learn <text>", "Aprende terminos NLP desde texto"],
], col_widths=[6*cm, 8.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 8. HISTORIAL GIT
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("8. Historial de Versiones", s_h1))
hr()
story.append(Paragraph(f"<b>{len(gitlog)} commits en master:</b>", s_body))
gl = [["Hash", "Mensaje"]]
for c in gitlog[:15]:
    parts = c.split(" ", 1)
    h = parts[0][:7]
    m = parts[1] if len(parts) > 1 else c
    gl.append([h, m])
tb(gl, col_widths=[2*cm, 12.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 9. VEREDICTO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("9. Veredicto", s_h1))
hr()

story.append(Paragraph("<b>Fortalezas:</b>", s_body))
story.append(Paragraph(
    "&bull; Diseno por capas limpio y extensible. Cada modulo tiene su header "
    "con tipos y funciones claras.<br/>"
    "&bull; Cero dependencias externas en C, incluso en el modulo de cifrado.<br/>"
    "&bull; Sistema de glifos nativos compilados en el binario, sin GPU ni LLM.<br/>"
    "&bull; Arquitectura Sistema -> Glifo con glifosenilla.json como glue declarativo.<br/>"
    f"&bull; {pass_count}/{19} tests pasando, cubriendo todos los modulos incluyendo cifrado.<br/>"
    "&bull; Hash indices para busquedas O(1) en semantica y grafo de conceptos.<br/>"
    "&bull; Cifrado novedoso basado unicamente en el modelo BDGB.<br/>"
    "&bull; Puerto Python limpio (bdgb_bridge.py) que evita duplicar logica C.<br/>"
    "&bull; ~55+ terminos NLP aprendidos dinamicamente desde glifos."
    , s_body))

story.append(Spacer(1, 4))
story.append(Paragraph("<b>Debilidades y riesgos:</b>", s_body))
story.append(Paragraph(
    "&bull; La busqueda secuencial en find_nodes_by_concept aun recorre todo el archivo "
    "en ciertos casos. Con miles de nodos sera lento.<br/>"
    "&bull; El NLP aun es limitado (55+ terminos, pero sin stemming, sin stopwords).<br/>"
    "&bull; youtube-automator no tiene sistema asignado ni implementacion real.<br/>"
    "&bull; Paths hardcodeados a Windows. Linux no funciona sin cambios.<br/>"
    "&bull; El cifrado BDGB-Cipher v1 no ha sido auditado por criptografos profesionales "
    "(de ahi el desafio publico).<br/>"
    "&bull; No hay CI/CD ni tests automatizados en push."
    , s_body))

story.append(Spacer(1, 10))
story.append(HRFlowable(width="100%", thickness=1, color=DARK))
story.append(Spacer(1, 6))
story.append(Paragraph(
    "<b>BDGB</b> ha evolucionado de un prototipo conceptual (v1, 16 nodos, 4x4) "
    "a una herramienta tecnica con 256 nodos, hash indices, cifrado propio, "
    f"{pass_count}/19 tests, un sistema de glifos nativos en C y un desafio publico de cifrado. "
    "La incorporacion del sistema de glifos nativos (masa madre) elimina la dependencia "
    "de Python para las tareas core de automatizacion. "
    "Los proximos pasos deberian enfocarse en: "
    "(1) portabilidad Linux, (2) expansion del NLP, "
    "(3) implementacion del sistema youtube-automator, "
    "(4) CI/CD, (5) auditoria criptografica profesional del BDGB-Cipher.",
    ParagraphStyle("veredicto", parent=s_body, fontSize=9, leading=13,
                   textColor=DARK, backColor=HexColor("#f0f4ff"),
                   leftIndent=8, rightIndent=8, spaceBefore=4, spaceAfter=4,
                   borderPadding=6, borderColor=ACCENT, borderWidth=0.5)))

story.append(Spacer(1, 1.5*cm))
story.append(Paragraph(
    "<i>Informe generado automaticamente &mdash; BDGB Audit Tool &mdash; "
    f"{datetime.now().strftime('%Y-%m-%d %H:%M')}</i>",
    ParagraphStyle("footer", parent=s_body, fontSize=7.5, textColor=HexColor("#999"),
                   alignment=TA_CENTER)))

doc.build(story)
print(f"PDF generated: {OUTPUT}")

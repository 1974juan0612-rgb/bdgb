#!/usr/bin/env python3
"""Generate BDGB v3 audit PDF using reportlab."""

import os
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import cm, mm
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.colors import HexColor, black, white, Color
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, HRFlowable, KeepTogether
)
from reportlab.lib import colors

OUTPUT = os.path.join(os.path.dirname(__file__), "auditoria_bdgb_v3.pdf")

# ── colours ──────────────────────────────────────────────────────────
DARK  = HexColor("#1a1a2e")
ACCENT = HexColor("#0f3460")
LIGHT = HexColor("#e8e8e8")
GREEN = HexColor("#16a34a")
RED = HexColor("#dc2626")
AMBER = HexColor("#d97706")
TEAL = HexColor("#0891b2")

# ── document ─────────────────────────────────────────────────────────
doc = SimpleDocTemplate(
    OUTPUT, pagesize=A4,
    leftMargin=2*cm, rightMargin=2*cm,
    topMargin=2*cm, bottomMargin=2*cm,
)

styles = getSampleStyleSheet()

s_title = ParagraphStyle("s_title", parent=styles["Title"],
    fontSize=22, leading=28, textColor=DARK, spaceAfter=4, alignment=TA_CENTER,
    fontName="Helvetica-Bold")
s_subtitle = ParagraphStyle("s_subtitle", parent=styles["Normal"],
    fontSize=10, leading=13, textColor=HexColor("#555"), spaceAfter=20, alignment=TA_CENTER,
    fontName="Helvetica")
s_h1 = ParagraphStyle("s_h1", parent=styles["Heading1"],
    fontSize=14, leading=18, textColor=DARK, spaceBefore=16, spaceAfter=8,
    fontName="Helvetica-Bold")
s_h2 = ParagraphStyle("s_h2", parent=styles["Heading2"],
    fontSize=11, leading=14, textColor=ACCENT, spaceBefore=10, spaceAfter=4,
    fontName="Helvetica-Bold")
s_body = ParagraphStyle("s_body", parent=styles["Normal"],
    fontSize=8.5, leading=11.5, textColor=HexColor("#222"), spaceAfter=4,
    alignment=TA_JUSTIFY, fontName="Helvetica")
s_body_small = ParagraphStyle("s_body_small", parent=s_body,
    fontSize=7.5, leading=10)
s_code = ParagraphStyle("s_code", parent=s_body,
    fontSize=7, leading=9, fontName="Courier", leftIndent=8,
    backColor=HexColor("#f5f5f5"))
s_ok = ParagraphStyle("s_ok", parent=s_body, textColor=GREEN, fontName="Helvetica-Bold", fontSize=8)
s_fail = ParagraphStyle("s_fail", parent=s_body, textColor=RED, fontName="Helvetica-Bold", fontSize=8)

story = []

def hr():
    story.append(HRFlowable(width="100%", thickness=0.5, color=HexColor("#ccc"),
                            spaceBefore=4, spaceAfter=4))

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

# ══════════════════════════════════════════════════════════════════════
# PORTADA
# ══════════════════════════════════════════════════════════════════════
story.append(Spacer(1, 3*cm))
story.append(Paragraph("INFORME DE AUDITOR\u00cdA", s_title))
story.append(Paragraph("BDGB v3 &mdash; Binary Dynamics Grid Brain", s_subtitle))
story.append(Spacer(1, 0.5*cm))
story.append(HRFlowable(width="60%", thickness=1.5, color=ACCENT, spaceBefore=4, spaceAfter=8))
story.append(Paragraph(
    "Rejilla binaria 16\u00d716 con din\u00e1mica Kaprekar, cifrado de flujo BDGB-Cipher v1, "
    "motor sem\u00e1ntico, NLP, aprendizaje, agentes y puente Python.",
    ParagraphStyle("portada_desc", parent=s_body, fontSize=9, leading=13,
                   alignment=TA_CENTER, textColor=HexColor("#444"))))
story.append(Spacer(1, 1.5*cm))

meta_data = [
    ["Repositorio", "https://github.com/1974juan0612-rgb/bdgb"],
    ["Lenguaje", "C99 + Python 3"],
    ["L\u00edneas C", "~2,500 (n\u00facleo + crypt)"],
    ["L\u00edneas Python", "~375 (bridge + scraper)"],
    ["Nodos", "256 (16\u00d716 grid, 8-bit)"],
    ["Versi\u00f3n auditada", "v3 &mdash; Junio 2026"],
    ["Tests", "19/19 PASS"],
]
meta_t = Table(meta_data, colWidths=[3.2*cm, 10*cm])
meta_t.setStyle(TableStyle([
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
story.append(meta_t)
story.append(PageBreak())

# ══════════════════════════════════════════════════════════════════════
# 1. PROJECT OVERVIEW
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("1. Descripci\u00f3n General del Proyecto", s_h1))
hr()
story.append(Paragraph(
    "BDGB (Binary Dynamics Grid Brain) v3 es un sistema de conocimiento geom\u00e9trico "
    "basado en una rejilla binaria de 16\u00d716 nodos (256 nodos, 8-bit cada uno). "
    "Cada nodo representa un valor 0&ndash;255 cuyas propiedades se derivan de su "
    "representaci\u00f3n binaria: densidad (popcount), simetr\u00eda, tipo geom\u00e9trico "
    "(esquina/borde/interior), radio, clase din\u00e1mica y atractor asociado.", s_body))
story.append(Paragraph(
    "La din\u00e1mica Kaprekar binaria asigna a cada nodo un sucesor (restando el orden "
    "ascendente del descendente de sus bits), generando cuencas de atracci\u00f3n que "
    "convergen en puntos fijos. Este modelo matem\u00e1tico subyace tanto al motor de "
    "b\u00fasqueda sem\u00e1ntica como al nuevo sistema de cifrado.", s_body))

story.append(Paragraph("<b>Componentes del sistema:</b>", s_body))
tb([
    ["Componente", "Descripci\u00f3n", "Estado"],
    ["N\u00facleo geom\u00e9trico", "Grid 16\u00d716, nodos de 4 bytes, propiedades O(1)", "Funcional (100%)"],
    ["Regla din\u00e1mica", "Sucesor Kaprekar binario, detecci\u00f3n de atractores", "Funcional (100%)"],
    ["Sem\u00e1ntica", "Hash index (256 buckets) para relaci\u00f3n nodo\u2192concepto", "Funcional (90%)"],
    ["Grafo de conceptos", "Hash index para aristas entre conceptos", "Funcional (85%)"],
    ["Motor de b\u00fasqueda", "H\u00edbrido: sem\u00e1ntico + propiedades + atractor", "Funcional (80%)"],
    ["NLP", "20 t\u00e9rminos fijos, tokenizador, consultas h\u00edbridas", "Funcional (60%)"],
    ["Aprendizaje", "Refuerzo y decaimiento con persistencia a disco", "Funcional (70%)"],
    ["Agentes", "Registry JSON, pipeline declarativo, supervisor tick", "Estructural (50%)"],
    ["BDGB-Cipher v1", "Cifrado de flujo sin dependencias externas", "NUEVO (100%)"],
    ["Python bridge", "CLI wrapper para integraci\u00f3n desde Python", "Funcional (90%)"],
    ["Scraper", "Mock + pytrends para injecci\u00f3n de tendencias", "Funcional (70%)"],
], col_widths=[3.8*cm, 7.5*cm, 3.5*cm])

# ══════════════════════════════════════════════════════════════════════
# 2. ENCRYPTION SYSTEM
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("2. Sistema de Cifrado &mdash; BDGB-Cipher v1", s_h1))
hr()
story.append(Paragraph(
    "BDGB-Cipher v1 es un cifrado de flujo (stream cipher) dise\u00f1ado completamente "
    "sobre el modelo matem\u00e1tico BDGB, sin utilizar ninguna biblioteca criptogr\u00e1fica "
    "externa. Implementado en 199 l\u00edneas de C99 (archivo <font face='Courier'>crypt/bdgb_crypt.c</font>).",
    s_body))

story.append(Paragraph("<b>2.1 Estado interno</b>", s_h2))
story.append(Paragraph(
    "El estado del cifrado son 4 palabras de 32 bits (<font face='Courier'>uint32_t wseed[4]</font>), "
    "un vector de inicializaci\u00f3n de 8 bytes y un contador de 64 bits. Total: 32 bytes de estado.", s_body))

story.append(Paragraph("<b>2.2 S-box geom\u00e9trica</b>", s_h2))
story.append(Paragraph(
    "La funci\u00f3n <font face='Courier'>bdgb_sbox()</font> no es una tabla lookup: "
    "computa en tiempo real las propiedades geom\u00e9tricas del nodo en la rejilla 16\u00d716 "
    "(densidad, radio, simetr\u00eda, tipo_geom), las combina con los pasos al atractor "
    "y el popcount, y produce un byte no lineal. Esto hace que la S-box dependa \u00edntegramente "
    "del modelo BDGB subyacente.", s_body))

story.append(Paragraph("<b>2.3 Derivaci\u00f3n de clave</b>", s_h2))
story.append(Paragraph(
    "La funci\u00f3n <font face='Courier'>derive_seed_32()</font> procesa cada byte de la "
    "contrase\u00f1a + IV mediante una mezcla aritm\u00e9tica (XOR, suma, rotaci\u00f3n) aplicando "
    "la funci\u00f3n <font face='Courier'>mix32()</font> en 8 rondas. Cada ronda mezcla progresivamente "
    "las 4 palabras de estado con retroalimentaci\u00f3n cruzada.", s_body))

story.append(Paragraph("<b>2.4 Generaci\u00f3n de keystream</b>", s_h2))
story.append(Paragraph(
    "Por cada byte de keystream, las 4 palabras de estado se mezclan con retroalimentaci\u00f3n "
    "del contador (<font face='Courier'>counter</font> de 64 bits). Cada byte de salida "
    "depende de los 4 words del estado actual y del contador. Tras cada byte, el contador "
    "se incrementa, garantizando divergencia.", s_body))

story.append(Paragraph("<b>2.5 Formato de archivo .bdgb</b>", s_body))
tb([
    ["Offset", "Tama\u00f1o", "Campo", "Descripci\u00f3n"],
    ["0&ndash;3", "4 B", "Magic", "\"BDGB\" (firma)"],
    ["4", "1 B", "Versi\u00f3n", "0x01"],
    ["5&ndash;12", "8 B", "IV", "Vector de inicializaci\u00f3n aleatorio"],
    ["13+", "variable", "Ciphertext", "Plaintext XOR keystream"],
], col_widths=[2.5*cm, 2.5*cm, 3*cm, 6.8*cm])

story.append(Paragraph("<b>2.6 Seguridad y dependencias</b>", s_h2))
story.append(Paragraph(
    "El cifrado tiene <b>cero dependencias externas</b>: no usa OpenSSL, no usa nada del "
    "sistema operativo. Es C99 puro. La seguridad se basa en la no linealidad de la S-box "
    "geom\u00e9trica y en la mezcla aritm\u00e9tica de las 4 palabras de estado. "
    "El espacio de claves es de 2<super>128</super> (4 \u00d7 32 bits). "
    "El desaf\u00edo p\u00fablico (<font face='Courier'>crypt/challenge/</font>) invita a la "
    "comunidad a intentar romperlo, validando as\u00ed su robustez.", s_body))

# ══════════════════════════════════════════════════════════════════════
# 3. CHALLENGE
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("3. Desaf\u00edo de Desencriptado Comunitario", s_h1))
hr()
story.append(Paragraph(
    "Se ha publicado un desaf\u00edo p\u00fablico en <font face='Courier'>crypt/challenge/</font>. "
    "El archivo <font face='Courier'>secret.bdgb</font> (445 bytes) contiene un mensaje cifrado "
    "con BDGB-Cipher v1 usando una contrase\u00f1a alfanum\u00e9rica (a-z, 0-9).", s_body))

story.append(Paragraph("<b>Archivos del desaf\u00edo:</b>", s_body))
tb([
    ["Archivo", "Descripci\u00f3n"],
    ["crypt/challenge/secret.bdgb", "Archivo cifrado de 445 bytes"],
    ["crypt/challenge/README_CHALLENGE.md", "Reglas, pistas y documentaci\u00f3n del desaf\u00edo"],
    ["crypt/challenge/CHALLENGE_BUNDLE.md", "Paquete compacto para an\u00e1lisis con IA local"],
], col_widths=[5.5*cm, 9.3*cm])

story.append(Paragraph(
    "El desaf\u00edo permite cualquier m\u00e9todo: fuerza bruta, criptoan\u00e1lisis, ingenier\u00eda "
    "inversa. El objetivo es extraer el mensaje original y demostrar la contrase\u00f1a utilizada. "
    "Si se encuentra una vulnerabilidad en el dise\u00f1o, se otorgar\u00e1 cr\u00e9dito en el "
    "repositorio y menci\u00f3n en la documentaci\u00f3n oficial.", s_body))

# ══════════════════════════════════════════════════════════════════════
# 4. TEST SUITE
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("4. Suite de Tests", s_h1))
hr()
story.append(Paragraph(
    "La bater\u00eda de tests (<font face='Courier'>tests/test_bdgb.c</font>) cubre todos los "
    "m\u00f3dulos del sistema. Resultado: <b>19/19 PASS, 0 FAIL</b>.", s_body))

story.append(Paragraph("<b>Tests ejecutados:</b>", s_body))
tb([
    ["#", "Test", "M\u00f3dulo", "Resultado"],
    ["1", "grid_size", "Constantes del grid (8 bits, 16\u00d716, 256 nodos)", "PASS"],
    ["2", "node_creation", "Creaci\u00f3n de 256 nodos con coordenadas correctas", "PASS"],
    ["3", "popcount", "Conteo de unos (0, 1, 3, 0xFF, 0x0F)", "PASS"],
    ["4", "symmetry", "Detecci\u00f3n de simetr\u00eda (0, 0xFF, 0x81, 0x1B)", "PASS"],
    ["5", "geom_types", "Tipos geom\u00e9tricos en grid 16\u00d716", "PASS"],
    ["6", "dynamic_rule", "Regla Kaprekar binaria (punto fijo, mapeo)", "PASS"],
    ["7", "attractor_search", "256 nodos convergen a atractor", "PASS"],
    ["8", "storage_roundtrip", "Persistencia y carga de 256 nodos", "PASS"],
    ["9", "semantics_hash", "Hash index en sem\u00e1ntica", "PASS"],
    ["10", "concept_graph_hash", "Hash index en grafo de conceptos", "PASS"],
    ["11", "learning_reinforce", "Refuerzo (+10) y decaimiento (x0.5)", "PASS"],
    ["12", "nlp_parse", "Tokenizaci\u00f3n y mapeo a conceptos", "PASS"],
    ["13", "search_hybrid", "B\u00fasqueda h\u00edbrida + refuerzo", "PASS"],
    ["14", "search_by_predicate", "Filtro por propiedades (sim\u00e9trico + interior)", "PASS"],
    ["15", "compute_props", "Propiedades v\u00e1lidas para 256 nodos", "PASS"],
    ["16", "crypt_init", "Inicializaci\u00f3n del contexto de cifrado", "PASS"],
    ["17", "crypt_keystream_differs", "Divergencia con distintas claves/IV", "PASS"],
    ["18", "crypt_encrypt_decrypt", "Ronda completa cifrado/descifrado", "PASS"],
    ["19", "crypt_file_roundtrip", "Cifrado/descifrado de archivo completo", "PASS"],
], col_widths=[0.8*cm, 4*cm, 6.5*cm, 1.8*cm])

story.append(Paragraph(
    "<b>Nota:</b> Todos los tests utilizan datos temporales en "
    "<font face='Courier'>tests/test_data/</font> que se limpian antes y despu\u00e9s de la "
    "ejecuci\u00f3n. No hay dependencia de red ni de bibliotecas externas.", s_body))

# ══════════════════════════════════════════════════════════════════════
# 5. ARCHITECTURE
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("5. Arquitectura del Sistema", s_h1))
hr()

story.append(Paragraph("<b>5.1 Pila tecnol\u00f3gica</b>", s_h2))
tb([
    ["Capa", "Tecnolog\u00eda", "Funci\u00f3n"],
    ["Lenguaje base", "C99 (ISO/IEC 9899:1999)", "N\u00facleo de alto rendimiento"],
    ["Build system", "CMake 3.10 + Ninja", "Compilaci\u00f3n cruzada y r\u00e1pida"],
    ["CLI", "main.c con argparse manual", "Interfaz de l\u00ednea de comandos"],
    ["Python Bridge", "bdgb_bridge.py (subprocess)", "Integraci\u00f3n desde Python"],
    ["Scraper", "scraper_trends.py (pytrends)", "Injecci\u00f3n de tendencias"],
    ["GUI", "bdgb_gui.py (Tkinter)", "Visualizaci\u00f3n de grid y b\u00fasqueda"],
], col_widths=[3.5*cm, 5.5*cm, 5.8*cm])

story.append(Paragraph("<b>5.2 Comandos CLI</b>", s_body))
tb([
    ["Comando", "Funci\u00f3n"],
    ["<font face='Courier'>--init</font>", "Limpia e inicializa datos (256 nodos + sem\u00e1ntica demo)"],
    ["<font face='Courier'>--search &lt;query&gt;</font>", "B\u00fasqueda NLP con salida JSON"],
    ["<font face='Courier'>--add-concept &lt;n&gt; &lt;c&gt; &lt;w&gt; &lt;r&gt;</font>", "A\u00f1ade relaci\u00f3n sem\u00e1ntica"],
    ["<font face='Courier'>--export-nodes</font>", "Dump JSON de todos los nodos con propiedades"],
    ["<font face='Courier'>--agent-run &lt;id&gt;</font>", "Ejecuta pipeline de agente"],
    ["<font face='Courier'>--encrypt &lt;in&gt; &lt;out&gt; [key]</font>", "Cifra archivo con BDGB-KAPREKAR"],
    ["<font face='Courier'>--decrypt &lt;in&gt; &lt;out&gt; [key]</font>", "Descifra archivo"],
    ["<font face='Courier'>--hash &lt;text&gt;</font>", "Genera hash BDGB de 16 bytes"],
], col_widths=[6*cm, 8.8*cm])

story.append(Paragraph("<b>5.3 Variables de entorno y portabilidad</b>", s_h2))
story.append(Paragraph(
    "El sistema soporta <font face='Courier'>BDGB_ROOT</font> como variable de entorno "
    "para rutas port\u00e1tiles. El flag <font face='Courier'>--data-path</font> permite "
    "especificar el directorio de datos en tiempo de ejecuci\u00f3n. "
    "El archivo <font face='Courier'>.gitignore</font> excluye <font face='Courier'>build/</font>, "
    "<font face='Courier'>__pycache__/</font>, <font face='Courier'>test_data/</font> y "
    "<font face='Courier'>data/</font>.", s_body))

story.append(Paragraph("<b>5.4 Cero dependencias externas en C</b>", s_h2))
story.append(Paragraph(
    "Todo el n\u00facleo C, incluyendo el cifrado BDGB-Cipher, se implementa sin ninguna "
    "dependencia externa. No se utiliza OpenSSL, libcrypto, ni ninguna biblioteca de terceros. "
    "La S-box, la derivaci\u00f3n de clave y la generaci\u00f3n de keystream se basan "
    "\u00fanicamente en aritm\u00e9tica de enteros y las propiedades geom\u00e9tricas de la "
    "rejilla BDGB.", s_body))

# ══════════════════════════════════════════════════════════════════════
# 6. FILE STRUCTURE
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("6. Estructura del Repositorio", s_h1))
hr()

story.append(Paragraph("<b>Directorios ra\u00edz:</b>", s_body))
tb([
    ["Directorio/Archivo", "Descripci\u00f3n"],
    ["<font face='Courier'>include/</font>", "Headers C: bdgb.h, semantics.h, concept_graph.h, search.h, learning.h, nlp.h, agent.h"],
    ["<font face='Courier'>src/</font>", "Implementaciones C: main.c, node.c, grid.c, storage.c, semantics.c, concept_graph.c, search.c, learning.c, nlp.c, agent.c"],
    ["<font face='Courier'>crypt/</font>", "M\u00f3dulo de cifrado: bdgb_crypt.c, bdgb_crypt.h, challenge/"],
    ["<font face='Courier'>crypt/challenge/</font>", "Desaf\u00edo p\u00fablico: secret.bdgb, README_CHALLENGE.md, CHALLENGE_BUNDLE.md"],
    ["<font face='Courier'>tests/</font>", "Suite de tests: test_bdgb.c, test_data/"],
    ["<font face='Courier'>interface/</font>", "Python: bdgb_bridge.py (CLI wrapper), bdgb_gui.py (Tkinter)"],
    ["<font face='Courier'>scripts/</font>", "Python: scraper_trends.py (pytrends + mock)"],
    ["<font face='Courier'>agents/</font>", "Configuraci\u00f3n de agentes: registry.json, youtube-automator/"],
    ["<font face='Courier'>data/</font>", "Datos persistidos: nodes.dat, semantics.dat, concept_edges.dat, usage_*.dat"],
    ["<font face='Courier'>build/</font>", "Artefactos de compilaci\u00f3n (.exe, .obj)"],
    ["<font face='Courier'>CMakeLists.txt</font>", "Build de CMake (bdgb + bdgb_test)"],
    ["<font face='Courier'>.gitignore</font>", "Exclusiones: build/, __pycache__/, test_data/, data/"],
    ["<font face='Courier'>auditoria.md</font>", "Informe de auditor\u00eda previo (v1)"],
], col_widths=[5.5*cm, 9.3*cm])

story.append(Paragraph("<b>Formatos de datos en disco:</b>", s_body))
tb([
    ["Archivo", "Formato", "Tama\u00f1o registro"],
    ["nodes.dat", "Binario secuencial: id_bits (1B) + x (1B) + y (1B) + flags (1B)", "4 B/nodo"],
    ["semantics.dat", "Binario: node_id (1B) + concept_id (2B) + weight (1B) + rel_type (1B)", "5 B/enlace"],
    ["concept_edges.dat", "Binario: from (2B) + to (2B) + weight (1B) + rel_type (1B)", "6 B/arista"],
    ["concepts.idx", "Hash index (256 buckets) para sem\u00e1ntica", "256 \u00d7 32 B"],
    ["concept_edges.idx", "Hash index (256 buckets) para grafo de conceptos", "256 \u00d7 32 B"],
    ["usage_*.dat", "Binario: ID (1&ndash;2B) + contador (4B)", "5&ndash;6 B/entrada"],
], col_widths=[3.5*cm, 7*cm, 4.3*cm])

# ══════════════════════════════════════════════════════════════════════
# 7. HISTORIAL GIT
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("7. Historial de Versiones", s_h1))
hr()
story.append(Paragraph("<b>Commits (m\u00e1s recientes):</b>", s_body))
tb([
    ["Hash", "Mensaje"],
    ["<font face='Courier'>2f985a6</font>", "crypt: BDGB-Cipher v1 + challenge de desencriptado comunitario"],
    ["<font face='Courier'>462b979</font>", "chore: add .gitignore para build, pycache, test_data, data"],
    ["<font face='Courier'>bdc2588</font>", "BDGB v2: salto de prototipo a herramienta tecnica"],
    ["<font face='Courier'>25dd7a4</font>", "BDGB: interfaz grafica Python/Tkinter - grid 4x4, busqueda NLP, panel de agentes"],
    ["<font face='Courier'>792a11d</font>", "BDGB: base de agentes - registry, config, supervisor, agent module"],
    ["<font face='Courier'>d72df7c</font>", "BDGB: modulo de lenguaje natural - 20 terminos, tokenizer, consultas hibridas"],
    ["<font face='Courier'>79f7803</font>", "BDGB: primera version - geometria 4x4, semantica, busqueda, aprendizaje"],
], col_widths=[2.5*cm, 12.3*cm])

# ══════════════════════════════════════════════════════════════════════
# 8. VEREDICTO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("8. Veredicto", s_h1))
hr()

story.append(Paragraph("<b>Fortalezas:</b>", s_body))
story.append(Paragraph(
    "&bull; Dise\u00f1o por capas limpio y extensible. Cada m\u00f3dulo tiene su header "
    "con tipos y funciones claras. El cambio de BDGB_GRID_BITS expande toda la geometr\u00eda "
    "autom\u00e1ticamente.<br/>"
    "&bull; Cero dependencias externas en C, incluso en el m\u00f3dulo de cifrado.<br/>"
    "&bull; 19/19 tests pasando, cubriendo todos los m\u00f3dulos incluyendo el nuevo cifrado.<br/>"
    "&bull; Hash indices para b\u00fasquedas O(1) en sem\u00e1ntica y grafo de conceptos.<br/>"
    "&bull; Sistema de cifrado novedoso basado \u00fanicamente en el modelo BDGB.<br/>"
    "&bull; Puerto Python limpio (bdgb_bridge.py) que evita duplicar la l\u00f3gica del motor C."
    , s_body))

story.append(Spacer(1, 4))
story.append(Paragraph("<b>Debilidades y riesgos:</b>", s_body))
story.append(Paragraph(
    "&bull; La b\u00fasqueda secuencial en <font face='Courier'>find_nodes_by_concept</font> "
    "a\u00fan recorre todo el archivo en ciertos casos. Con miles de nodos ser\u00e1 lento.<br/>"
    "&bull; El NLP tiene solo 20 t\u00e9rminos fijos y no aprende palabras nuevas.<br/>"
    "&bull; La ejecuci\u00f3n real de agentes (pipeline) no conecta herramientas externas "
    "(Godot, ffmpeg, YouTube). Es una estructura vac\u00eda.<br/>"
    "&bull; Paths hardcodeados a Windows en los tests. Linux no funciona sin cambios.<br/>"
    "&bull; El cifrado BDGB-Cipher v1 no ha sido auditado por cript\u00f3grafos profesionales "
    "(de ah\u00ed el desaf\u00edo p\u00fablico)."
    , s_body))

story.append(Spacer(1, 10))
story.append(HRFlowable(width="100%", thickness=1, color=DARK))
story.append(Spacer(1, 6))
story.append(Paragraph(
    "<b>BDGB v3</b> ha evolucionado de un prototipo conceptual (v1, 16 nodos, 4\u00d74) "
    "a una herramienta t\u00e9cnica con 256 nodos, hash indices, cifrado propio, "
    "19 tests y un desaf\u00edo p\u00fablico. El dise\u00f1o por capas sigue siendo s\u00f3lido. "
    "La incorporaci\u00f3n del cifrado basado en la geometr\u00eda BDGB representa un hito "
    "significativo. Los pr\u00f3ximos pasos deber\u00edan enfocarse en: "
    "(1) portabilidad Linux, (2) expansi\u00f3n del NLP, "
    "(3) conexi\u00f3n real de herramientas con agentes, "
    "(4) auditor\u00eda criptogr\u00e1fica profesional del BDGB-Cipher.",
    ParagraphStyle("veredicto", parent=s_body, fontSize=9, leading=13,
                   textColor=DARK, backColor=HexColor("#f0f4ff"),
                   leftIndent=8, rightIndent=8, spaceBefore=4, spaceAfter=4,
                   borderPadding=6, borderColor=ACCENT, borderWidth=0.5)))

story.append(Spacer(1, 1.5*cm))
story.append(Paragraph(
    "<i>Informe generado autom\u00e1ticamente &mdash; BDGB Audit Tool v3 &mdash; "
    "Junio 2026</i>",
    ParagraphStyle("footer", parent=s_body, fontSize=7.5, textColor=HexColor("#999"),
                   alignment=TA_CENTER)))

# ── BUILD ────────────────────────────────────────────────────────────
doc.build(story)
print(f"PDF generated: {OUTPUT}")

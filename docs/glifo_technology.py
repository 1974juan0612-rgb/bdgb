#!/usr/bin/env python3
"""BDGB Glifo Technology — whitepaper PDF"""

import os, json, subprocess, sys
from datetime import datetime
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import cm
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.colors import HexColor, black, white
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle,
    PageBreak, HRFlowable, Image
)
from reportlab.lib import colors

BDGB_ROOT = os.environ.get(
    "BDGB_ROOT",
    os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
)
OUTPUT = os.path.join(BDGB_ROOT, "glifo-technology.pdf")

DARK  = HexColor("#1a1a2e")
ACCENT = HexColor("#0f3460")
GREEN = HexColor("#16a34a")
TEAL = HexColor("#0891b2")
BGLIGHT = HexColor("#f0f4ff")

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
s_h3 = ParagraphStyle("s_h3", parent=styles["Heading3"],
    fontSize=9.5, leading=12, textColor=HexColor("#333"), spaceBefore=6, spaceAfter=3, fontName="Helvetica-Bold")
s_body = ParagraphStyle("s_body", parent=styles["Normal"],
    fontSize=8.5, leading=11.5, textColor=HexColor("#222"), spaceAfter=4,
    alignment=TA_JUSTIFY, fontName="Helvetica")
s_code = ParagraphStyle("s_code", parent=s_body,
    fontSize=7, leading=9, fontName="Courier", leftIndent=8, backColor=HexColor("#f5f5f5"))
s_hl = ParagraphStyle("s_hl", parent=s_body,
    fontSize=9, leading=12, textColor=DARK, backColor=BGLIGHT,
    leftIndent=8, rightIndent=8, spaceBefore=4, spaceAfter=4,
    borderPadding=6, borderColor=ACCENT, borderWidth=0.5)

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

# ══════════════════════════════════════════════════════════════════════
# PORTADA
# ══════════════════════════════════════════════════════════════════════
story.append(Spacer(1, 3*cm))
story.append(Paragraph("GLIFO", s_title))
story.append(Paragraph("Tecnologia de Automatización Geométrica", s_subtitle))
story.append(Spacer(1, 0.5*cm))
story.append(HRFlowable(width="60%", thickness=1.5, color=ACCENT, spaceBefore=4, spaceAfter=8))
story.append(Spacer(1, 2*cm))
story.append(Paragraph(
    "Especificacion tecnica completa del sistema de glifos BDGB: "
    "que es un glifo, sus fundamentos matematicos, "
    "su arquitectura de sistemas y su modelo de funcionamiento.",
    ParagraphStyle("portada_desc", parent=s_body, fontSize=9, leading=13,
                   alignment=TA_CENTER, textColor=HexColor("#444"))))
story.append(Spacer(1, 1.5*cm))

meta = [
    ["Tecnologia", "BDGB — Binary Dynamics Grid Brain"],
    ["Version", "1.0 (2026-06-08)"],
    ["Autor", "Juan Diego RGB"],
    ["Lenguaje", "C99, Python 3"],
    ["Clasificacion", "Primo, Semilla, Comun"],
    ["Ramas", "Online, Local, Hibrida"],
]
mt = Table(meta, colWidths=[3.5*cm, 9.7*cm])
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
# 1. INTRODUCCION
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("1. Introduccion", s_h1))
hr()
story.append(Paragraph(
    "Este documento define formalmente la tecnologia <b>Glifo</b> del sistema BDGB "
    "(Binary Dynamics Grid Brain). Un glifo es una unidad computacional atomica que "
    "manipula informacion usando herramientas como instrumentos para cumplir un flujo "
    "de trabajo (Sistema). Su logica es de orquestacion: decide que herramienta invocar, "
    "con que argumentos, en que orden, y como procesar el resultado para la siguiente etapa. "
    "Las herramientas (scripts, APIs, binarios) ejecutan las operaciones concretas; "
    "el glifo las orquesta para completar la tarea asignada.",
    s_body))
story.append(Paragraph(
    "El sistema Glifo se basa en una rejilla binaria 16x16 con dinamica Kaprekar, "
    "un motor semantico con hash index, busqueda hibrida, NLP integrado, "
    "cifrado de flujo propio (BDGB-Cipher v1) y aprendizaje por refuerzo. "
    "Todo el nucleo opera sin GPU, sin LLM, y sin dependencias externas en C.",
    s_body))
story.append(Paragraph(
    "Este documento esta dirigido a desarrolladores, arquitectos de software y "
    "curiosos que quieran entender como funciona BDGB Glifo por dentro, "
    "desde sus fundamentos matematicos hasta su implementacion concreta.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 2. QUE ES UN GLIFO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("2. Que es un Glifo", s_h1))
hr()
story.append(Paragraph(
    "Un <b>Glifo</b> es un nodo operativo dentro de un Sistema. "
    "Su funcion es manipular informacion para cumplir el flujo de trabajo "
    "que se le encarga. Para lograrlo, invoca herramientas como instrumentos: "
    "les pasa datos de entrada, recibe resultados, y decide el siguiente paso. "
    "El glifo no ejecuta la logica de dominio directamente, pero si orquesta "
    "el flujo completo usando las herramientas adecuadas en cada etapa.",
    s_body))

story.append(Paragraph("<b>Analogia:</b>", s_h3))
story.append(Paragraph(
    "Imagina un chef en una cocina. El chef (glifo) no cultiva vegetales ni "
    "fabrica ollas: usa herramientas (cuchillos, horno, ingredientes) para "
    "preparar el plato. Decide que herramienta usar, cuando, y en que orden. "
    "El plato final es responsabilidad del chef, no de las herramientas.",
    s_hl))

story.append(Paragraph("<b>Naturaleza del Glifo:</b>", s_h3))
tb([
    ["Propiedad", "Descripcion"],
    ["Orquestador", "Invoca herramientas, pasa datos, recibe resultados, decide el siguiente paso"],
    ["Manipula informacion", "Toma datos de entrada, los transforma usando herramientas, produce resultados"],
    ["Orientado al flujo", "Su proposito es completar el workflow completo, no solo transportar"],
    ["Declarativo", "Su pipeline se define en JSON (semilla.json)"],
    ["Portatil", "Usa herramientas del SO: funciona en Windows, Linux, macOS, Android"],
    ["Componible", "Varios glifos forman un pipeline; un panal puede ser glifo de otro panal mayor"],
    ["Estado medible", "Cada glifo cuenta ejecuciones, exitos y fallos"],
], col_widths=[3.5*cm, 11.3*cm])

# ══════════════════════════════════════════════════════════════════════
# 2.1. CLASIFICACION
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("2.1 Clasificacion de los Glifos", s_h2))
story.append(Paragraph(
    "Los glifos se clasifican en tres tipos segun su naturaleza y funcion dentro del sistema:",
    s_body))

tb([
    ["Clase", "Que es", "Ejemplo"],
    ["Primo", "El codigo original del que se derivan todos los demas glifos. "
     "Es el molde, la masa madre. Existe uno solo por implementacion.",
     "src/glifo.c, include/glifo.h"],
    ["Semilla", "El archivo semilla.json que define y crea un panal. "
     "Sin semilla no hay panal. Es el ADN del flujo de trabajo.",
     "vigilancia-tendencias/semilla.json"],
    ["Comun", "Glifos operativos que ejecutan las tareas dentro de un panal. "
     "Pueden ser nativos (C) o externos (Python, etc.).",
     "primo, trend-tracker"],
], col_widths=[2.5*cm, 8*cm, 4.3*cm])

story.append(Paragraph("Subtipos de glifo comun:", s_h3))
tb([
    ["Subtipo", "Descripcion", "Ventaja", "Desventaja"],
    ["Nativo", "Compilado en el binario BDGB (C). "
     "No requiere interprete ni runtime externo.",
     "Maximo rendimiento, portatil", "Requiere compilar para cada plataforma"],
    ["Externo", "Script (Python, bash, etc.) dentro de la "
     "carpeta del sistema.",
     "Facil de modificar y extender",
     "Requiere runtime (Python, etc.)"],
], col_widths=[2.5*cm, 5.5*cm, 3.5*cm, 3.3*cm])

# ══════════════════════════════════════════════════════════════════════
# 3. MATEMATICAS
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("3. Matematicas del Glifo", s_h1))
hr()
story.append(Paragraph(
    "Todo el sistema BDGB se basa en una rejilla binaria de 16x16 nodos "
    "(256 nodos, 8 bits por nodo). Cada nodo tiene un valor numerico "
    "del que se derivan todas sus propiedades geometricas y dinamicas. "
    "Estas propiedades alimentan el motor semantico, la busqueda, el NLP y el cifrado.",
    s_body))

story.append(Paragraph("3.1 La Rejilla Binaria (Grid)", s_h2))
story.append(Paragraph(
    "La rejilla es una matriz de 16x16 = 256 posiciones. Cada posicion "
    "almacena un byte (8 bits, valor 0-255). Las coordenadas (x, y) "
    "se derivan directamente del ID del nodo:",
    s_body))
story.append(Paragraph(
    "Nodo ID = 0..255<br/>"
    "x = ID &amp; 0x0F  (bits bajos, 0-15)<br/>"
    "y = ID &gt;&gt; 4    (bits altos, 0-15)",
    s_code))
story.append(Paragraph(
    "La estructura en memoria del nodo es de 4 bytes: id_bits (uint8), "
    "x (uint8), y (uint8), y un byte de relleno para alineacion.",
    s_body))

story.append(Paragraph("<b>Propiedades derivadas de cada nodo:</b>", s_h3))
tb([
    ["Propiedad", "Formula", "Rango", "Uso"],
    ["Popcount", "bits = popcount(n)", "0-8", "Densidad del nodo"],
    ["Simetria", "n == reverse_bits(n)", "0/1", "Filtro de busqueda"],
    ["Tipo geometrico", "esquina(x,y) / borde / interior", "0-2", "Clasificacion espacial"],
    ["Radio", "distancia al centro (8,8)", "0-12", "Proximidad al centro"],
    ["Clase dinamica", "punto_fijo / ciclo / atractor", "0-2", "Comportamiento temporal"],
    ["Pasos a atractor", "iteraciones hasta converger", "0-16", "Complejidad dinamica"],
], col_widths=[3*cm, 4.5*cm, 1.5*cm, 5.8*cm])

story.append(Paragraph("3.2 Dinamica Kaprekar Binaria", s_h2))
story.append(Paragraph(
    "Cada nodo tiene un sucesor calculado por la regla Kaprekar binaria:",
    s_body))
story.append(Paragraph(
    "1. Ordenar los bits del valor actual en orden ascendente (asc)<br/>"
    "2. Ordenar los bits en orden descendente (desc)<br/>"
    "3. Sucesor = desc - asc (en binario)<br/>"
    "4. Repetir hasta alcanzar un punto fijo (atractor)",
    s_code))
story.append(Paragraph(
    "Por ejemplo: el nodo con valor 0x1B (00011011 en binario):<br/>"
    "ascendente = 00000111 (7), descendente = 11100000 (224)<br/>"
    "sucesor = 224 - 7 = 217 (11011001)<br/>"
    "Este proceso se repite hasta que sucesor == nodo actual (punto fijo).",
    s_body))
story.append(Paragraph(
    "<b>Propiedades de la dinamica:</b> Todos los 256 nodos convergen a puntos fijos "
    "(no hay ciclos de longitud &gt; 1). Los puntos fijos son valores cuyo "
    "binario en orden ascendente y descendente son iguales. "
    "Esto genera cuencas de atraccion que son la base del motor de busqueda.",
    s_hl))

story.append(Paragraph("3.3 Relacion con el Cifrado BDGB-Cipher", s_h2))
story.append(Paragraph(
    "El mismo modelo matematico de la rejilla se utiliza para el cifrado de flujo "
    "BDGB-Cipher v1. El estado interno del cifrado son 4 semillas de 32 bits "
    "(128 bits total) que se mezclan mediante la funcion geometrica del nodo. "
    "La S-box se deriva de las propiedades del nodo BDGB calculadas en tiempo real. "
    "No utiliza ninguna biblioteca criptografica externa.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 4. SISTEMA DE FUNCIONAMIENTO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("4. Sistema de Funcionamiento", s_h1))
hr()
story.append(Paragraph(
    "Un <b>Panal</b> es un flujo de trabajo con un proposito definido. "
    "Contiene glifos, relaciones entre ellos, un pipeline de ejecucion, "
    "un control de activacion (Tiempo) y datos propios del panal. "
    "Cada panal tiene una <b>Semilla</b> (`semilla.json`) en su raiz que "
    "es la autoridad del panal.",
    s_body))

story.append(Paragraph("4.1 Arquitectura Panal &gt; Glifo", s_h2))
story.append(Paragraph(
    "La jerarquia es simple: un Panal contiene uno o mas Glifos. "
    "Un Glifo no existe fuera de un Panal (aunque su codigo pueda crearse aparte, "
    "no se ejecuta hasta que se declara en una semilla.json).",
    s_body))

story.append(Paragraph("<b>Estructura de directorios:</b>", s_h3))
story.append(Paragraph(
    "glifos/<br/>"
    "  registry.json                    &lt;- registro maestro de panales y glifos<br/>"
    "  &lt;panal&gt;/<br/>"
    "    semilla.json                   &lt;- SEMILLA: define todo el panal<br/>"
    "    glifos/<br/>"
    "      &lt;glifo&gt;/<br/>"
    "        glifo.json                 &lt;- config individual del glifo<br/>"
    "        ...codigo del glifo...",
    s_code))

story.append(Paragraph("4.2 La Semilla (semilla.json)", s_h2))
story.append(Paragraph(
    "Este archivo es la autoridad del panal. Define que glifos existen, "
    "como se relacionan, en que orden se ejecutan, el Tiempo de activacion, "
    "y si cada glifo es actualizable. Sin este archivo no hay panal.",
    s_body))

tb([
    ["Campo", "Descripcion"],
    ["id", "Identificador unico del panal"],
    ["nombre", "Nombre humano"],
    ["tipo", "Siempre 'panal'"],
    ["rama", "online, local, o hibrida"],
    ["tiempo", "Control de activacion y terminacion (tipo + modalidad repetitivo/encargo + fin)"],
    ["glifos[]", "Lista de glifos del panal (id, nombre, tipo, entry, actualizable)"],
    ["relaciones[]", "Conexiones entre glifos (de, a, tipo, flujo)"],
    ["pipeline.orden[]", "Orden de ejecucion con pasos y dependencias"],
    ["datos_panal", "Datos personalizados segun la tarea del panal"],
    ["metricas", "Contadores de ejecucion"],
], col_widths=[3.5*cm, 11.3*cm])

story.append(Paragraph("4.3 El Tiempo: glifo de control", s_h2))
story.append(Paragraph(
    "Cada panal tiene un <b>Tiempo</b>: el glifo de control que arranca primero "
    "y determina cuando se activa el panal. Hay dos tipos:",
    s_body))

tb([
    ["Tipo", "Descripcion", "Ejemplo"],
    ["Autonomo", "Se activa por horario propio. "
     "Funciona sin intervencion externa.",
     "Monitoreo diario a las 08:00"],
    ["Dirigido", "Lo activa un agente externo: usuario, "
     "programa, o senal de otro panal.",
     "Panal que se ejecuta al abrir un programa"],
], col_widths=[2.5*cm, 5.5*cm, 6.8*cm])

story.append(Paragraph("4.4 Actualizacion de glifos", s_h2))
story.append(Paragraph(
    "Cada glifo declara explicitamente si es <b>actualizable</b> "
    "(`actualizable: true/false`) en la semilla. "
    "Un glifo fijo (nativo C) no puede modificarse sin cambiar la semilla. "
    "Un glifo externo (script Python) puede actualizarse independientemente.",
    s_body))

story.append(Paragraph("4.5 Pipeline y Relaciones", s_h2))
story.append(Paragraph(
    "El <b>pipeline</b> define el orden de ejecucion de los glifos. "
    "Cada paso tiene un numero, un glifo asignado, una accion, argumentos opcionales "
    "y dependencias. Las <b>relaciones</b> definen conexiones conceptuales entre glifos "
    "(ejecucion, dependencia, datos compartidos).",
    s_body))

story.append(Paragraph("<b>Ejemplo de pipeline:</b>", s_h3))
story.append(Paragraph(
    "Paso 1: glifo 'primo' - fetch RSS + reporte + inyeccion en BDGB<br/>"
    "Paso 2: glifo 'trend-tracker' --daily (depende de primo)<br/>"
    "Paso 3: glifo 'trend-tracker' --weekly (cada 7 dias, depende de primo)",
    s_code))

story.append(Paragraph("4.6 Ramas de Panales", s_h2))
story.append(Paragraph(
    "Cada panal pertenece a una de tres ramas segun el origen de sus herramientas:",
    s_body))

tb([
    ["Rama", "Herramientas", "Ejemplos"],
    ["Online", "Servicios web, APIs externas. "
     "Requieren conexion a internet.",
     "Google Trends, YouTube API, OpenAI, scraping web"],
    ["Local", "Corren 100% en el dispositivo. "
     "No requieren internet.",
     "Scripts Python, binarios C, herramientas CLI del SO"],
    ["Hibrida", "Combinan herramientas online y locales. "
     "Lo mas comun en la practica.",
     "Scraper web + procesamiento local + API cloud"],
], col_widths=[2.5*cm, 5.5*cm, 6.8*cm])

# ══════════════════════════════════════════════════════════════════════
# 5. MOTOR SEMANTICO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("5. Motor Semantico", s_h1))
hr()
story.append(Paragraph(
    "El motor semantico asocia nodos de la rejilla con conceptos numericos "
    "mediante un sistema de hash index. Cada nodo puede tener multiples conceptos, "
    "y los conceptos se relacionan entre si mediante un grafo de aristas.",
    s_body))

story.append(Paragraph("5.1 Hash Index de Conceptos", s_h2))
story.append(Paragraph(
    "La semantica usa un hash de 256 buckets (0-255) derivado del concepto ID:",
    s_body))
story.append(Paragraph(
    "hash = (concept_id ^ (concept_id &gt;&gt; 4)) &amp; 0xFF",
    s_code))
story.append(Paragraph(
    "Cada bucket tiene una cadena maxima de 16 entradas. En la practica, "
    "con menos de 50 conceptos, las colisiones son minimas y la busqueda es O(1).",
    s_body))

story.append(Paragraph("5.2 Grafo de Conceptos", s_h2))
story.append(Paragraph(
    "Los conceptos se relacionan entre si mediante aristas con peso y tipo "
    "(definicion, ejemplo, metafora, funcion, causa). "
    "El grafo usa el mismo sistema de hash index para busquedas rapidas "
    "desde cualquier concepto hacia sus vecinos.",
    s_body))

story.append(Paragraph("5.3 Busqueda Hibrida", s_h2))
story.append(Paragraph(
    "La busqueda combina cuatro estrategias:",
    s_body))
story.append(Paragraph(
    "1. <b>Semantica:</b> busca nodos asociados a un concepto especifico<br/>"
    "2. <b>Propiedades:</b> filtra por simetria, densidad, tipo geometrico, etc.<br/>"
    "3. <b>Atractor:</b> busca nodos cerca de un atractor especifico<br/>"
    "4. <b>NLP:</b> tokeniza texto libre y mapea a conceptos + predicados",
    s_body))
story.append(Paragraph(
    "Los resultados se refuerzan con aprendizaje (incremento de uso) "
    "y se ordenan por puntuacion descendente.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 6. NLP
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("6. NLP (Procesamiento de Lenguaje Natural)", s_h1))
hr()
story.append(Paragraph(
    "El modulo NLP permite consultar el sistema BDGB usando texto libre. "
    "Convierte palabras en terminos que mapean a conceptos y predicados dentro del sistema.",
    s_body))

story.append(Paragraph("6.1 Tokenizacion", s_h2))
story.append(Paragraph(
    "El texto de entrada se divide en tokens (palabras de 3-20 caracteres, "
    "ignorando puntuacion y espacios). Cada token se busca en un diccionario "
    "de terminos que puede crecer dinamicamente.",
    s_body))

story.append(Paragraph("6.2 Sistema de Predicados", s_h2))
story.append(Paragraph(
    "Algunos terminos NO mapean a conceptos sino a predicados: funciones "
    "que filtran nodos por propiedades. Por ejemplo:",
    s_body))
tb([
    ["Termino", "Predicado", "Filtro"],
    ["simetrico", "simetria == 1", "Nodos simetricos"],
    ["denso", "densidad >= 3", "Nodos densos"],
    ["interior", "tipo_geom == INTERIOR", "Nodos interiores"],
    ["esquina", "tipo_geom == ESQUINA", "Esquinas"],
    ["borde", "tipo_geom == BORDE", "Bordes"],
    ["estable", "clase_dinamica == ATTRACTOR", "Puntos fijos"],
], col_widths=[3*cm, 4.5*cm, 4.5*cm])

story.append(Paragraph("6.3 Aprendizaje Dinamico", s_h2))
story.append(Paragraph(
    "El diccionario NLP no es estatico. Cuando un glifo inyecta datos en BDGB, "
    "el modulo nlp_learn_from_text() extrae automaticamente nuevos terminos "
    "del texto de entrada, los asocia a conceptos numericos y los persiste en disco. "
    "El sistema comenzo con ~20 terminos fijos y ha crecido a 55+ terminos "
    "aprendidos desde glifos externos.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 7. CIFRADO
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("7. BDGB-Cipher v1", s_h1))
hr()
story.append(Paragraph(
    "BDGB-Cipher es un cifrado de flujo (stream cipher) disenado completamente "
    "sobre el modelo matematico BDGB, sin utilizar ninguna biblioteca criptografica "
    "externa. Es la demostracion de que el mismo modelo que genera propiedades "
    "geometricas puede servir para cifrar datos.",
    s_body))

story.append(Paragraph("<b>Caracteristicas del cifrado:</b>", s_h3))
tb([
    ["Propiedad", "Valor"],
    ["Estado interno", "4 x uint32_t = 128 bits + IV 8 bytes + contador 64 bytes"],
    ["S-box", "Geometrica: propiedades del nodo BDGB en tiempo real"],
    ["Derivacion de clave", "mix32() con 8 rondas, retroalimentacion cruzada"],
    ["Keystream", "1 byte por ciclo, estado + contador"],
    ["Formato archivo", "Magic 'BDGB' + version 0x01 + IV 8 B + ciphertext"],
    ["Dependencias", "CERO (no OpenSSL, no libcrypto)"],
    ["Espacio de claves", "2^128"],
], col_widths=[4*cm, 10.8*cm])

story.append(Paragraph(
    "El cifrado demuestra la versatilidad del modelo matematico BDGB: "
    "lo que sirve para buscar informacion semanticamente tambien sirve "
    "para protegerla. Es la misma maquina con dos propositos distintos.",
    s_hl))

# ══════════════════════════════════════════════════════════════════════
# 8. RENDIMIENTO Y PORTABILIDAD
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("8. Rendimiento y Portabilidad", s_h1))
hr()
story.append(Paragraph(
    "El sistema Glifo esta disenado desde cero para ser portatil y eficiente.",
    s_body))

story.append(Paragraph("<b>Portabilidad:</b>", s_h3))
story.append(Paragraph(
    "La logica de orquestacion del glifo (decidir que herramienta invocar, "
    "con que argumentos, en que orden) es inherentemente portable: funciona "
    "identico en Windows, Linux, macOS, Android, iOS, tablets y cualquier "
    "sistema con un sistema de archivos. El nucleo C compila con C99 estandar "
    "en cualquier plataforma. Las herramientas externas pueden requerir "
    "runtimes especificos (Python, etc.), pero el glifo que las orquesta no.",
    s_body))

story.append(Paragraph("<b>Rendimiento:</b>", s_h3))
tb([
    ["Metrica", "Valor"],
    ["Binario", "~150 KB (bdgb.exe)"],
    ["Tiempo de inicio", "< 10 ms"],
    ["Busqueda semantica", "O(1) con hash index"],
    ["Busqueda hibrida", "O(n) sobre 256 nodos"],
    ["Tokenizacion NLP", "O(m) sobre m caracteres de entrada"],
    ["Cifrado/descifrado", "~2 MB/s en CPU moderna"],
    ["Memoria en reposo", "~300 KB"],
], col_widths=[4.5*cm, 10.3*cm])

story.append(Paragraph("<b>Sin dependencias:</b>", s_h3))
story.append(Paragraph(
    "El nucleo C (incluyendo el cifrado) tiene CERO dependencias externas. "
    "No necesita OpenSSL, libcurl, libxml, ni ninguna biblioteca de terceros. "
    "El unico requisito para compilar es un compilador C99 y CMake. "
    "Las herramientas externas (Python) son opcionales y se usan solo "
    "para glifos externos; el nucleo funciona sin ellas.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 9. RAMAS DETALLADAS
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("9. Ramas de Sistemas en Detalle", s_h1))
hr()

story.append(Paragraph("9.1 Rama Online", s_h2))
story.append(Paragraph(
    "Los sistemas de la rama <b>online</b> utilizan herramientas que dependen de "
    "servicios web o APIs externas. Ejemplos: Google Trends RSS, YouTube API, "
    "OpenAI API, scraping de paginas web. Estos sistemas requieren conexion a "
    "internet y manejan autenticacion (API keys, tokens).",
    s_body))
story.append(Paragraph(
    "El glifo orquesta las herramientas online: invoca la herramienta adecuada "
    "(ej: trend_tracker.py), recibe el resultado, y decide si pasarlo a la siguiente "
    "etapa del pipeline, combinarlo con otros datos, o reprocesarlo. "
    "El glifo no hace la llamada HTTP directamente, pero es quien decide "
    "que llamada hacer y cuando.",
    s_body))

story.append(Paragraph("9.2 Rama Local", s_h2))
story.append(Paragraph(
    "Los sistemas de la rama <b>local</b> funcionan completamente sin internet. "
    "Todas las herramientas corren en el dispositivo: scripts Python, binarios C, "
    "herramientas CLI del sistema operativo. Los datos se generan y procesan "
    "localmente.",
    s_body))
story.append(Paragraph(
    "Un sistema local tipico: el glifo invoca un escaner local que produce un JSON, "
    "toma ese JSON y lo pasa a una herramienta de analisis, recibe el reporte, "
    "y lo entrega a una herramienta de visualizacion. El glifo decide el flujo "
    "completo. Todo offline.",
    s_body))

story.append(Paragraph("9.3 Rama Hibrida", s_h2))
story.append(Paragraph(
    "Los sistemas de la rama <b>hibrida</b> combinan herramientas online y locales. "
    "Es la rama mas comun en la practica. Por ejemplo: un scraper web (online) "
    "descarga datos, un procesador local (Python) los analiza, y el resultado "
    "se sube a una API cloud (online).",
    s_body))
story.append(Paragraph(
    "El glifo abstrae esta complejidad: para el, cada herramienta es un modulo "
    "con entradas y salidas. No le importa si la herramienta hizo una llamada HTTP "
    "o un calculo local. Lo que importa es que el flujo completo se complete "
    "correctamente.",
    s_body))

# ══════════════════════════════════════════════════════════════════════
# 10. CONCLUSION
# ══════════════════════════════════════════════════════════════════════
story.append(Paragraph("10. Conclusion", s_h1))
hr()
story.append(Paragraph(
    "La tecnologia Glifo de BDGB es un enfoque unico para la automatizacion "
    "de flujos de trabajo. Su innovacion principal es la orquestacion declarativa: "
    "el glifo manipula informacion usando herramientas como instrumentos, "
    "y su logica se define en JSON. Esto hace que los glifos sean portatiles, "
    "componibles y faciles de encadenar en pipelines complejos.",
    s_body))
story.append(Paragraph(
    "Los fundamentos matematicos (rejilla binaria, dinamica Kaprekar, "
    "hash index semantico) proporcionan una base solida y predecible "
    "para el sistema, sin depender de bibliotecas externas, GPUs o modelos "
    "de lenguaje masivos. El mismo modelo matematico sirve para buscar, "
    "aprender y cifrar.",
    s_body))
story.append(Paragraph(
    "La clasificacion en tres ramas (Online, Local, Hibrida) organiza "
    "los panales segun la naturaleza de sus herramientas, y las tres "
    "clases de glifos (Primo, Semilla, Comun) establecen una jerarquia "
    "conceptual clara desde la implementacion base hasta los nodos operativos. "
    "El Tiempo (glifo de control) diferencia panales autonomos de dirigidos, "
    "y el flag actualizable permite que glifos externos se actualicen sin "
    "modificar la semilla.",
    s_body))

story.append(Spacer(1, 8))
story.append(HRFlowable(width="100%", thickness=1, color=DARK))
story.append(Spacer(1, 6))
story.append(Paragraph(
    "<b>BDGB Glifo Technology</b> v1.0<br/>"
    "Documento generado el " + datetime.now().strftime("%Y-%m-%d %H:%M") + "<br/>"
    "Tecnologia creada por Juan Diego RGB &mdash; 2026",
    ParagraphStyle("footer", parent=s_body, fontSize=7.5, textColor=HexColor("#999"),
                   alignment=TA_CENTER)))

doc.build(story)
print(f"PDF generated: {OUTPUT}")

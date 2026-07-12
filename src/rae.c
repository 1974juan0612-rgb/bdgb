#include "rae.h"
#include "bdgb.h"
#include "semantics.h"
#include "concept_graph.h"
#include "nlp.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_TOKENS 64
#define MAX_CATS_PER_TOKEN 8

static int rae_inited = 0;

/* ---- helpers ---- */

static void lowercase(const char *src, char *dst, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = (char)tolower(src[i]);
        i++;
    }
    dst[i] = 0;
}

static int get_categorias(uint16_t concept_id, uint16_t *out_cats, int max) {
    if (!out_cats || max <= 0) return 0;
    ConceptEdge edges[16];
    int n = get_related_concepts(concept_id, edges, 16);
    int count = 0;
    for (int i = 0; i < n && count < max; i++) {
        if (edges[i].rel_type == REL_PERTENECE &&
            edges[i].to_concept >= CAT_SUSTANTIVO &&
            edges[i].to_concept <= CAT_PRONOMBRE) {
            out_cats[count++] = edges[i].to_concept;
        }
    }
    return count;
}

/* ---- Registro ---- */

static uint16_t next_vocab_id = 7000;

static void reg_concepto(uint16_t id, const char *nombre, uint16_t cat, uint16_t set_id) {
    nlp_add_term(nombre, id, NULL, -1, -1);
    if (cat)
        add_concept_edge(id, cat, 255, REL_PERTENECE);
    if (cat) {
        uint8_t rtype = 0;
        switch (cat) {
            case CAT_SUSTANTIVO:  rtype = REL_SUSTANTIVO; break;
            case CAT_VERBO:       rtype = REL_VERBO; break;
            case CAT_ADJETIVO:    rtype = REL_ADJETIVO; break;
            case CAT_ADVERBIO:    rtype = REL_ADVERBIO; break;
            case CAT_ARTICULO:    rtype = REL_ARTICULO; break;
            case CAT_PREPOSICION: rtype = REL_PREPOSICION; break;
            case CAT_PRONOMBRE:   rtype = REL_PRONOMBRE; break;
        }
        if (rtype) {
            uint8_t node_id = (uint8_t)(id % BDGB_GRID_NODES);
            add_concept(node_id, id, 200, rtype);
        }
    }
    if (set_id)
        add_concept_edge(id, set_id, 255, REL_PERTENECE);
}

/* helper para registrar vocabulario con id auto-incrementado */
static void reg_vocab(const char *word, uint16_t cat, uint16_t set_id) {
    reg_concepto(next_vocab_id++, word, cat, set_id);
}

static void sembrar_vocabulario(void) {
    /* Sustantivos */
    reg_concepto(101, "personaje", CAT_SUSTANTIVO, SET_PERSONAJE);
    reg_concepto(102, "escena", CAT_SUSTANTIVO, SET_ESCENA);
    reg_concepto(103, "archivo", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(104, "proyecto", CAT_SUSTANTIVO, SET_PROYECTO);
    reg_concepto(105, "imagen", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(106, "color", CAT_SUSTANTIVO, SET_CUALIDAD);
    reg_concepto(107, "forma", CAT_SUSTANTIVO, SET_CUALIDAD);
    reg_concepto(108, "lienzo", CAT_SUSTANTIVO, SET_HERRAMIENTA);
    reg_concepto(109, "carpeta", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(110, "documento", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(111, "pagina", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(112, "vineta", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(113, "bocadillo", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(114, "plano", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(115, "sprite", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(116, "nivel", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(117, "escenario", CAT_SUSTANTIVO, SET_ESCENA);
    reg_concepto(118, "audio", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(119, "video", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(120, "animacion", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(121, "idea", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(122, "concepto", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(123, "diseno", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(124, "prototipo", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(125, "referencia", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(126, "usuario", CAT_SUSTANTIVO, SET_PERSONAJE);
    reg_concepto(127, "sistema", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(128, "memoria", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(129, "texto", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(130, "comando", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(131, "recurso", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(132, "version", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(133, "capa", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(134, "pincel", CAT_SUSTANTIVO, SET_HERRAMIENTA);
    reg_concepto(135, "paleta", CAT_SUSTANTIVO, SET_HERRAMIENTA);
    reg_concepto(136, "modelo", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(137, "fotograma", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(138, "clip", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(139, "trama", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(140, "portada", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(141, "informe", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(142, "nota", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(143, "entrada", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(144, "salida", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(145, "estado", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(146, "ahora", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_concepto(147, "modulo", CAT_SUSTANTIVO, SET_OBJETO);
    reg_concepto(148, "yo", CAT_SUSTANTIVO, SET_PERSONAJE);

    /* Verbos */
    reg_concepto(201, "crear", CAT_VERBO, SET_ACCION_PROYECTO);
    reg_concepto(202, "guardar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(203, "cargar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(204, "mostrar", CAT_VERBO, 0);
    reg_concepto(205, "listar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(206, "decir", CAT_VERBO, 0);
    reg_concepto(207, "ejecutar", CAT_VERBO, SET_ACCION_PROYECTO);
    reg_concepto(208, "hacer", CAT_VERBO, SET_ACCION_PROYECTO);
    reg_concepto(209, "saber", CAT_VERBO, 0);
    reg_concepto(210, "aprender", CAT_VERBO, 0);
    reg_concepto(211, "abrir", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(212, "cerrar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(213, "editar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(214, "pintar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(215, "dibujar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(216, "colorear", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(217, "rellenar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(218, "buscar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(219, "seleccionar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(220, "inspeccionar", CAT_VERBO, 0);
    reg_concepto(221, "renderizar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(222, "previsualizar", CAT_VERBO, 0);
    reg_concepto(223, "probar", CAT_VERBO, 0);
    reg_concepto(224, "importar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(225, "exportar", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(226, "convertir", CAT_VERBO, SET_ACCION_ARCHIVO);
    reg_concepto(227, "recortar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(228, "rotar", CAT_VERBO, SET_ACCION_DIBUJO);
    reg_concepto(229, "escalar", CAT_VERBO, SET_ACCION_DIBUJO);

    /* Adjetivos */
    reg_concepto(301, "rojo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(302, "azul", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(303, "verde", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(304, "grande", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(305, "pequeno", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(306, "rapido", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(307, "lento", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(308, "nuevo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(309, "viejo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(310, "bonito", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(311, "feo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(312, "claro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(313, "oscuro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(314, "fino", CAT_ADJETIVO, SET_CUALIDAD);
    reg_concepto(315, "grueso", CAT_ADJETIVO, SET_CUALIDAD);

    /* Articulos */
    reg_concepto(401, "el", CAT_ARTICULO, 0);
    reg_concepto(402, "la", CAT_ARTICULO, 0);
    reg_concepto(403, "los", CAT_ARTICULO, 0);
    reg_concepto(404, "las", CAT_ARTICULO, 0);
    reg_concepto(405, "un", CAT_ARTICULO, 0);
    reg_concepto(406, "una", CAT_ARTICULO, 0);
    reg_concepto(407, "unos", CAT_ARTICULO, 0);
    reg_concepto(408, "unas", CAT_ARTICULO, 0);

    /* Preposiciones */
    reg_concepto(501, "en", CAT_PREPOSICION, 0);
    reg_concepto(502, "de", CAT_PREPOSICION, 0);
    reg_concepto(503, "con", CAT_PREPOSICION, 0);
    reg_concepto(504, "para", CAT_PREPOSICION, 0);
    reg_concepto(505, "por", CAT_PREPOSICION, 0);
    reg_concepto(506, "a", CAT_PREPOSICION, 0);
    reg_concepto(507, "sobre", CAT_PREPOSICION, 0);
    reg_concepto(508, "entre", CAT_PREPOSICION, 0);
    reg_concepto(509, "sin", CAT_PREPOSICION, 0);
    reg_concepto(510, "hacia", CAT_PREPOSICION, 0);
    reg_concepto(511, "desde", CAT_PREPOSICION, 0);
    reg_concepto(512, "hasta", CAT_PREPOSICION, 0);
    reg_concepto(513, "durante", CAT_PREPOSICION, SET_TIEMPO);

    /* Pronombres */
    reg_concepto(601, "yo", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(602, "tu", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(603, "el", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(604, "ella", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(605, "nosotros", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(606, "ellos", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(607, "ellas", CAT_PRONOMBRE, SET_PERSONAJE);
    reg_concepto(608, "esto", CAT_PRONOMBRE, 0);
    reg_concepto(609, "eso", CAT_PRONOMBRE, 0);
    reg_concepto(610, "aquello", CAT_PRONOMBRE, 0);
    reg_concepto(611, "que", CAT_PRONOMBRE, 0);
    reg_concepto(612, "quien", CAT_PRONOMBRE, 0);
    reg_concepto(613, "cual", CAT_PRONOMBRE, 0);
    reg_concepto(614, "mi", CAT_PRONOMBRE, 0);
    reg_concepto(615, "me", CAT_PRONOMBRE, 0);
    reg_concepto(616, "se", CAT_PRONOMBRE, 0);
    reg_concepto(617, "le", CAT_PRONOMBRE, 0);
}

static void sembrar_vocabulario_general(void) {
    /* ============ SUSTANTIVOS ============ */

    /* Familia y personas */
    reg_vocab("madre", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("padre", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("hermano", CAT_SUSTANTIVO, SET_FAMILIA);  reg_vocab("hermana", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("hijo", CAT_SUSTANTIVO, SET_FAMILIA);     reg_vocab("hija", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("abuelo", CAT_SUSTANTIVO, SET_FAMILIA);   reg_vocab("abuela", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("tio", CAT_SUSTANTIVO, SET_FAMILIA);      reg_vocab("tia", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("primo", CAT_SUSTANTIVO, SET_FAMILIA);    reg_vocab("prima", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("sobrino", CAT_SUSTANTIVO, SET_FAMILIA);  reg_vocab("sobrina", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("esposo", CAT_SUSTANTIVO, SET_FAMILIA);   reg_vocab("esposa", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("marido", CAT_SUSTANTIVO, SET_FAMILIA);   reg_vocab("mujer", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("nino", CAT_SUSTANTIVO, SET_FAMILIA);     reg_vocab("nina", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("bebe", CAT_SUSTANTIVO, SET_FAMILIA);     reg_vocab("adulto", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("gente", CAT_SUSTANTIVO, SET_FAMILIA);    reg_vocab("persona", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("nene", CAT_SUSTANTIVO, SET_FAMILIA);     reg_vocab("nena", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("senyor", CAT_SUSTANTIVO, SET_FAMILIA);   reg_vocab("senyora", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("chico", CAT_SUSTANTIVO, SET_FAMILIA);    reg_vocab("chica", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("amigo", CAT_SUSTANTIVO, SET_FAMILIA);    reg_vocab("amiga", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("vecino", CAT_SUSTANTIVO, SET_FAMILIA);   reg_vocab("companero", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("familia", CAT_SUSTANTIVO, SET_FAMILIA);  reg_vocab("papa", CAT_SUSTANTIVO, SET_FAMILIA);
    reg_vocab("mama", CAT_SUSTANTIVO, SET_FAMILIA);

    /* Animales */
    reg_vocab("perro", CAT_SUSTANTIVO, SET_ANIMAL);     reg_vocab("gato", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("caballo", CAT_SUSTANTIVO, SET_ANIMAL);   reg_vocab("pajaro", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("pez", CAT_SUSTANTIVO, SET_ANIMAL);       reg_vocab("mariposa", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("tortuga", CAT_SUSTANTIVO, SET_ANIMAL);   reg_vocab("conejo", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("vaca", CAT_SUSTANTIVO, SET_ANIMAL);      reg_vocab("toro", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("oveja", CAT_SUSTANTIVO, SET_ANIMAL);     reg_vocab("cerdo", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("gallina", CAT_SUSTANTIVO, SET_ANIMAL);   reg_vocab("pato", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("aguila", CAT_SUSTANTIVO, SET_ANIMAL);    reg_vocab("serpiente", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("leon", CAT_SUSTANTIVO, SET_ANIMAL);      reg_vocab("tigre", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("elefante", CAT_SUSTANTIVO, SET_ANIMAL);  reg_vocab("mono", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("rana", CAT_SUSTANTIVO, SET_ANIMAL);      reg_vocab("lobo", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("oso", CAT_SUSTANTIVO, SET_ANIMAL);       reg_vocab("raton", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("mosca", CAT_SUSTANTIVO, SET_ANIMAL);     reg_vocab("arana", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("abeja", CAT_SUSTANTIVO, SET_ANIMAL);     reg_vocab("hormiga", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("delfin", CAT_SUSTANTIVO, SET_ANIMAL);    reg_vocab("ballena", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("tiburon", CAT_SUSTANTIVO, SET_ANIMAL);   reg_vocab("animal", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("mascota", CAT_SUSTANTIVO, SET_ANIMAL);   reg_vocab("pollo", CAT_SUSTANTIVO, SET_ANIMAL);
    reg_vocab("cocodrilo", CAT_SUSTANTIVO, SET_ANIMAL); reg_vocab("buho", CAT_SUSTANTIVO, SET_ANIMAL);

    /* Naturaleza, plantas, geografia */
    reg_vocab("arbol", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("flor", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("hoja", CAT_SUSTANTIVO, SET_NATURALEZA);  reg_vocab("rama", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("semilla", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("raiz", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("tronco", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("bosque", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("montanya", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("rio", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("lago", CAT_SUSTANTIVO, SET_NATURALEZA);  reg_vocab("mar", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("oceano", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("playa", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("isla", CAT_SUSTANTIVO, SET_NATURALEZA);  reg_vocab("tierra", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("cielo", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("lluvia", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("nieve", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("viento", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("sol", CAT_SUSTANTIVO, SET_NATURALEZA);   reg_vocab("luna", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("estrella", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("nube", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("fuego", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("agua", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("aire", CAT_SUSTANTIVO, SET_NATURALEZA);  reg_vocab("roca", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("piedra", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("arena", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("campo", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("valle", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("colina", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("selva", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("desierto", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("planta", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("jardin", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("hierba", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("fruta", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("volcan", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("cascada", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("tormenta", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("arcoiris", CAT_SUSTANTIVO, SET_NATURALEZA); reg_vocab("relampago", CAT_SUSTANTIVO, SET_NATURALEZA);
    reg_vocab("trueno", CAT_SUSTANTIVO, SET_NATURALEZA);

    /* Comida y bebida */
    reg_vocab("pan", CAT_SUSTANTIVO, SET_COMIDA);       reg_vocab("leche", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("vino", CAT_SUSTANTIVO, SET_COMIDA);      reg_vocab("carne", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("pescado", CAT_SUSTANTIVO, SET_COMIDA);   reg_vocab("verdura", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("sopa", CAT_SUSTANTIVO, SET_COMIDA);      reg_vocab("arroz", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("pasta", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("queso", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("huevo", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("mantequilla", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("aceite", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("sal", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("azucar", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("cafe", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("te", CAT_SUSTANTIVO, SET_COMIDA);        reg_vocab("chocolate", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("pastel", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("galleta", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("ensalada", CAT_SUSTANTIVO, SET_COMIDA);  reg_vocab("manzana", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("naranja", CAT_SUSTANTIVO, SET_COMIDA);   reg_vocab("platano", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("uva", CAT_SUSTANTIVO, SET_COMIDA);       reg_vocab("fresa", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("limon", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("sandia", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("cebolla", CAT_SUSTANTIVO, SET_COMIDA);   reg_vocab("ajo", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("tomate", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("lechuga", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("zanahoria", CAT_SUSTANTIVO, SET_COMIDA); reg_vocab("papa", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("maiz", CAT_SUSTANTIVO, SET_COMIDA);      reg_vocab("frijol", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("jamon", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("comida", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("bebida", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("desayuno", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("almuerzo", CAT_SUSTANTIVO, SET_COMIDA);  reg_vocab("cena", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("cereal", CAT_SUSTANTIVO, SET_COMIDA);    reg_vocab("legumbre", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("dulce", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("helado", CAT_SUSTANTIVO, SET_COMIDA);
    reg_vocab("yogur", CAT_SUSTANTIVO, SET_COMIDA);     reg_vocab("oreja", CAT_SUSTANTIVO, SET_COMIDA);

    /* Cuerpo y salud */
    reg_vocab("cabeza", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("cara", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("ojo", CAT_SUSTANTIVO, SET_CUERPO);       reg_vocab("nariz", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("boca", CAT_SUSTANTIVO, SET_CUERPO);      reg_vocab("oreja", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("pelo", CAT_SUSTANTIVO, SET_CUERPO);      reg_vocab("frente", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("mejilla", CAT_SUSTANTIVO, SET_CUERPO);   reg_vocab("labio", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("diente", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("lengua", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("cuello", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("hombro", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("brazo", CAT_SUSTANTIVO, SET_CUERPO);     reg_vocab("codo", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("mano", CAT_SUSTANTIVO, SET_CUERPO);      reg_vocab("dedo", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("pierna", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("rodilla", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("pie", CAT_SUSTANTIVO, SET_CUERPO);       reg_vocab("espalda", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("pecho", CAT_SUSTANTIVO, SET_CUERPO);     reg_vocab("estomago", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("corazon", CAT_SUSTANTIVO, SET_CUERPO);   reg_vocab("pulmon", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("higado", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("rinon", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("sangre", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("hueso", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("piel", CAT_SUSTANTIVO, SET_CUERPO);      reg_vocab("musculo", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("cerebro", CAT_SUSTANTIVO, SET_CUERPO);   reg_vocab("cuerpo", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("salud", CAT_SUSTANTIVO, SET_CUERPO);     reg_vocab("enfermedad", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("medico", CAT_SUSTANTIVO, SET_CUERPO);    reg_vocab("dolor", CAT_SUSTANTIVO, SET_CUERPO);
    reg_vocab("uña", CAT_SUSTANTIVO, SET_CUERPO);

    /* Hogar y objetos */
    reg_vocab("casa", CAT_SUSTANTIVO, SET_HOGAR);       reg_vocab("puerta", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("ventana", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("pared", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("techo", CAT_SUSTANTIVO, SET_HOGAR);      reg_vocab("piso", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cuarto", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("sala", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cocina", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("banio", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("dormitorio", CAT_SUSTANTIVO, SET_HOGAR); reg_vocab("comedor", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cama", CAT_SUSTANTIVO, SET_HOGAR);       reg_vocab("mesa", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("silla", CAT_SUSTANTIVO, SET_HOGAR);      reg_vocab("sofa", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("estante", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("armario", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("lampara", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("espejo", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cortina", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("alfombra", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cojin", CAT_SUSTANTIVO, SET_HOGAR);      reg_vocab("manta", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("toalla", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("jabon", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cepillo", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("peine", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("llave", CAT_SUSTANTIVO, SET_HOGAR);      reg_vocab("cerradura", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cocina", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("horno", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("nevera", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("lavadora", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("escoba", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("cubo", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("vaso", CAT_SUSTANTIVO, SET_HOGAR);       reg_vocab("plato", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("tenedor", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("cuchillo", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("cuchara", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("olla", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("sarten", CAT_SUSTANTIVO, SET_HOGAR);     reg_vocab("taza", CAT_SUSTANTIVO, SET_HOGAR);
    reg_vocab("botella", CAT_SUSTANTIVO, SET_HOGAR);    reg_vocab("bolsa", CAT_SUSTANTIVO, SET_HOGAR);

    /* Ropa y accesorios */
    reg_vocab("camisa", CAT_SUSTANTIVO, SET_ROPA);      reg_vocab("pantalon", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("falda", CAT_SUSTANTIVO, SET_ROPA);       reg_vocab("vestido", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("chaqueta", CAT_SUSTANTIVO, SET_ROPA);    reg_vocab("abrigo", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("bufanda", CAT_SUSTANTIVO, SET_ROPA);     reg_vocab("sombrero", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("gorra", CAT_SUSTANTIVO, SET_ROPA);       reg_vocab("calcetin", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("zapato", CAT_SUSTANTIVO, SET_ROPA);      reg_vocab("bota", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("sandalia", CAT_SUSTANTIVO, SET_ROPA);    reg_vocab("cinturon", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("corbata", CAT_SUSTANTIVO, SET_ROPA);     reg_vocab("guante", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("gafas", CAT_SUSTANTIVO, SET_ROPA);       reg_vocab("reloj", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("anillo", CAT_SUSTANTIVO, SET_ROPA);      reg_vocab("collar", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("bolso", CAT_SUSTANTIVO, SET_ROPA);       reg_vocab("mochila", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("panuelo", CAT_SUSTANTIVO, SET_ROPA);     reg_vocab("ropa", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("traje", CAT_SUSTANTIVO, SET_ROPA);       reg_vocab("camiseta", CAT_SUSTANTIVO, SET_ROPA);
    reg_vocab("sueter", CAT_SUSTANTIVO, SET_ROPA);

    /* Ciudad, transporte, lugares */
    reg_vocab("calle", CAT_SUSTANTIVO, SET_CIUDAD);     reg_vocab("plaza", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("parque", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("edificio", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("tienda", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("mercado", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("escuela", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("hospital", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("iglesia", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("museo", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("teatro", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("cine", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("estadio", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("puente", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("carretera", CAT_SUSTANTIVO, SET_CIUDAD); reg_vocab("camino", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("esquina", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("acera", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("farola", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("banco", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("biblioteca", CAT_SUSTANTIVO, SET_CIUDAD); reg_vocab("oficina", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("fabrica", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("estacion", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("aeropuerto", CAT_SUSTANTIVO, SET_CIUDAD); reg_vocab("puerto", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("autobus", CAT_SUSTANTIVO, SET_CIUDAD);   reg_vocab("coche", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("camion", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("moto", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("bicicleta", CAT_SUSTANTIVO, SET_CIUDAD); reg_vocab("tren", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("avion", CAT_SUSTANTIVO, SET_CIUDAD);     reg_vocab("barco", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("taxi", CAT_SUSTANTIVO, SET_CIUDAD);      reg_vocab("ciudad", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("pueblo", CAT_SUSTANTIVO, SET_CIUDAD);    reg_vocab("pais", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("mundo", CAT_SUSTANTIVO, SET_CIUDAD);     reg_vocab("casa", CAT_SUSTANTIVO, SET_CIUDAD);
    reg_vocab("hotel", CAT_SUSTANTIVO, SET_CIUDAD);     reg_vocab("restaurante", CAT_SUSTANTIVO, SET_CIUDAD);

    /* Tiempo y clima (ya existian algunos, agregamos mas) */
    reg_vocab("dia", CAT_SUSTANTIVO, SET_TIEMPO);       reg_vocab("noche", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("maniana", CAT_SUSTANTIVO, SET_TIEMPO);   reg_vocab("tarde", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("semana", CAT_SUSTANTIVO, SET_TIEMPO);    reg_vocab("mes", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("anyo", CAT_SUSTANTIVO, SET_TIEMPO);      reg_vocab("siglo", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("hora", CAT_SUSTANTIVO, SET_TIEMPO);      reg_vocab("minuto", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("segundo", CAT_SUSTANTIVO, SET_TIEMPO);   reg_vocab("instante", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("fecha", CAT_SUSTANTIVO, SET_TIEMPO);     reg_vocab("ayer", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("hoy", CAT_SUSTANTIVO, SET_TIEMPO);       reg_vocab("verano", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("invierno", CAT_SUSTANTIVO, SET_TIEMPO);  reg_vocab("primavera", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("otonio", CAT_SUSTANTIVO, SET_TIEMPO);    reg_vocab("amanecer", CAT_SUSTANTIVO, SET_TIEMPO);
    reg_vocab("atardecer", CAT_SUSTANTIVO, SET_TIEMPO); reg_vocab("anochecer", CAT_SUSTANTIVO, SET_TIEMPO);

    /* Emociones y sentimientos */
    reg_vocab("amor", CAT_SUSTANTIVO, SET_EMOCION);     reg_vocab("odio", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("miedo", CAT_SUSTANTIVO, SET_EMOCION);    reg_vocab("alegria", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("tristeza", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("ira", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("enojo", CAT_SUSTANTIVO, SET_EMOCION);    reg_vocab("sorpresa", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("calma", CAT_SUSTANTIVO, SET_EMOCION);    reg_vocab("paz", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("ansiedad", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("esperanza", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("deseo", CAT_SUSTANTIVO, SET_EMOCION);    reg_vocab("pasion", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("ternura", CAT_SUSTANTIVO, SET_EMOCION);  reg_vocab("felicidad", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("placer", CAT_SUSTANTIVO, SET_EMOCION);   reg_vocab("confianza", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("duda", CAT_SUSTANTIVO, SET_EMOCION);     reg_vocab("respeto", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("admiracion", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("orgullo", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("verguenza", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("culpa", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("pereza", CAT_SUSTANTIVO, SET_EMOCION);   reg_vocab("aburrimiento", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("entusiasmo", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("gratitud", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("nostalgia", CAT_SUSTANTIVO, SET_EMOCION); reg_vocab("sentimiento", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("emocion", CAT_SUSTANTIVO, SET_EMOCION);  reg_vocab("soledad", CAT_SUSTANTIVO, SET_EMOCION);
    reg_vocab("ilusion", CAT_SUSTANTIVO, SET_EMOCION);

    /* ============ VERBOS ============ */
    /* Verbos basicos y cotidianos */
    reg_vocab("ser", CAT_VERBO, 0);
    reg_vocab("estar", CAT_VERBO, 0);
    reg_vocab("haber", CAT_VERBO, 0);
    reg_vocab("tener", CAT_VERBO, 0);
    reg_vocab("poder", CAT_VERBO, 0);
    reg_vocab("querer", CAT_VERBO, 0);
    reg_vocab("venir", CAT_VERBO, 0);
    reg_vocab("ir", CAT_VERBO, 0);
    reg_vocab("ver", CAT_VERBO, 0);
    reg_vocab("dar", CAT_VERBO, 0);
    reg_vocab("poner", CAT_VERBO, 0);
    reg_vocab("salir", CAT_VERBO, 0);
    reg_vocab("llegar", CAT_VERBO, 0);
    reg_vocab("hablar", CAT_VERBO, 0);
    reg_vocab("comer", CAT_VERBO, 0);
    reg_vocab("beber", CAT_VERBO, 0);
    reg_vocab("dormir", CAT_VERBO, 0);
    reg_vocab("vivir", CAT_VERBO, 0);
    reg_vocab("morir", CAT_VERBO, 0);
    reg_vocab("nacer", CAT_VERBO, 0);
    reg_vocab("crecer", CAT_VERBO, 0);
    reg_vocab("cambiar", CAT_VERBO, 0);
    reg_vocab("mover", CAT_VERBO, 0);
    reg_vocab("pensar", CAT_VERBO, 0);
    reg_vocab("sentir", CAT_VERBO, 0);
    reg_vocab("amar", CAT_VERBO, 0);
    reg_vocab("odiar", CAT_VERBO, 0);
    reg_vocab("temer", CAT_VERBO, 0);
    reg_vocab("leer", CAT_VERBO, 0);
    reg_vocab("escribir", CAT_VERBO, 0);
    reg_vocab("contar", CAT_VERBO, 0);
    reg_vocab("cantar", CAT_VERBO, 0);
    reg_vocab("bailar", CAT_VERBO, 0);
    reg_vocab("jugar", CAT_VERBO, 0);
    reg_vocab("correr", CAT_VERBO, 0);
    reg_vocab("saltar", CAT_VERBO, 0);
    reg_vocab("nadar", CAT_VERBO, 0);
    reg_vocab("volar", CAT_VERBO, 0);
    reg_vocab("trabajar", CAT_VERBO, 0);
    reg_vocab("estudiar", CAT_VERBO, 0);
    reg_vocab("ensenyar", CAT_VERBO, 0);
    reg_vocab("comprar", CAT_VERBO, 0);
    reg_vocab("vender", CAT_VERBO, 0);
    reg_vocab("pagar", CAT_VERBO, 0);
    reg_vocab("cobrar", CAT_VERBO, 0);
    reg_vocab("cocinar", CAT_VERBO, 0);
    reg_vocab("limpiar", CAT_VERBO, 0);
    reg_vocab("lavar", CAT_VERBO, 0);
    reg_vocab("llamar", CAT_VERBO, 0);
    reg_vocab("enviar", CAT_VERBO, 0);
    reg_vocab("recibir", CAT_VERBO, 0);
    reg_vocab("esperar", CAT_VERBO, 0);
    reg_vocab("viajar", CAT_VERBO, 0);
    reg_vocab("caminar", CAT_VERBO, 0);
    reg_vocab("parar", CAT_VERBO, 0);
    reg_vocab("empezar", CAT_VERBO, 0);
    reg_vocab("terminar", CAT_VERBO, 0);
    reg_vocab("continuar", CAT_VERBO, 0);
    reg_vocab("seguir", CAT_VERBO, 0);
    reg_vocab("acabar", CAT_VERBO, 0);
    reg_vocab("ayudar", CAT_VERBO, 0);
    reg_vocab("servir", CAT_VERBO, 0);
    reg_vocab("traer", CAT_VERBO, 0);
    reg_vocab("llevar", CAT_VERBO, 0);
    reg_vocab("usar", CAT_VERBO, 0);
    reg_vocab("necesitar", CAT_VERBO, 0);
    reg_vocab("gustar", CAT_VERBO, 0);
    reg_vocab("doler", CAT_VERBO, 0);
    reg_vocab("parecer", CAT_VERBO, 0);
    reg_vocab("sonar", CAT_VERBO, 0);
    reg_vocab("oler", CAT_VERBO, 0);
    reg_vocab("tocar", CAT_VERBO, 0);
    reg_vocab("mirar", CAT_VERBO, 0);
    reg_vocab("escuchar", CAT_VERBO, 0);
    reg_vocab("preguntar", CAT_VERBO, 0);
    reg_vocab("responder", CAT_VERBO, 0);
    reg_vocab("pedir", CAT_VERBO, 0);
    reg_vocab("ofrecer", CAT_VERBO, 0);
    reg_vocab("mostrar", CAT_VERBO, 0);
    reg_vocab("esconder", CAT_VERBO, 0);
    reg_vocab("compartir", CAT_VERBO, 0);
    reg_vocab("soltar", CAT_VERBO, 0);
    reg_vocab("agarrar", CAT_VERBO, 0);
    reg_vocab("lanzar", CAT_VERBO, 0);
    reg_vocab("empujar", CAT_VERBO, 0);
    reg_vocab("tirar", CAT_VERBO, 0);
    reg_vocab("romper", CAT_VERBO, 0);
    reg_vocab("arreglar", CAT_VERBO, 0);
    reg_vocab("construir", CAT_VERBO, 0);
    reg_vocab("destruir", CAT_VERBO, 0);
    reg_vocab("ganar", CAT_VERBO, 0);
    reg_vocab("perder", CAT_VERBO, 0);
    reg_vocab("deber", CAT_VERBO, 0);
    reg_vocab("recordar", CAT_VERBO, 0);
    reg_vocab("olvidar", CAT_VERBO, 0);
    reg_vocab("entender", CAT_VERBO, 0);
    reg_vocab("conocer", CAT_VERBO, 0);
    reg_vocab("descansar", CAT_VERBO, 0);
    reg_vocab("sonreir", CAT_VERBO, 0);
    reg_vocab("llorar", CAT_VERBO, 0);
    reg_vocab("gritar", CAT_VERBO, 0);
    reg_vocab("susurrar", CAT_VERBO, 0);
    reg_vocab("mentir", CAT_VERBO, 0);
    reg_vocab("confiar", CAT_VERBO, 0);
    reg_vocab("cumplir", CAT_VERBO, 0);
    reg_vocab("faltar", CAT_VERBO, 0);
    reg_vocab("importar", CAT_VERBO, 0);
    reg_vocab("significar", CAT_VERBO, 0);
    reg_vocab("decidir", CAT_VERBO, 0);
    reg_vocab("elegir", CAT_VERBO, 0);
    reg_vocab("merecer", CAT_VERBO, 0);
    reg_vocab("advertir", CAT_VERBO, 0);
    reg_vocab("permitir", CAT_VERBO, 0);
    reg_vocab("prohibir", CAT_VERBO, 0);
    reg_vocab("ser", CAT_VERBO, 0);
    reg_vocab("estar", CAT_VERBO, 0);
    reg_vocab("saber", CAT_VERBO, 0);
    reg_vocab("haber", CAT_VERBO, 0);

    /* ============ ADJETIVOS ============ */
    /* Colores */
    reg_vocab("blanco", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("negro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("gris", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("marrón", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("rosa", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("violeta", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("amarillo", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("dorado", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("plateado", CAT_ADJETIVO, SET_CUALIDAD);

    /* Tamaño y forma */
    reg_vocab("alto", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("bajo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("largo", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("corto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("ancho", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("estrecho", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("profundo", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("superficial", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("enorme", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("diminuto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("gigante", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("mediano", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("redondo", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("cuadrado", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("triangular", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("recto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("curvo", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("plano", CAT_ADJETIVO, SET_CUALIDAD);

    /* Textura y sensacion */
    reg_vocab("suave", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("aspero", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("liso", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("rugoso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("blando", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("duro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("flexible", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("rigido", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("pegajoso", CAT_ADJETIVO, SET_CUALIDAD);

    /* Temperatura y sabor */
    reg_vocab("caliente", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("frio", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("tibio", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("helado", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("ardiente", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("fresco", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("dulce", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("salado", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("amargo", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("acido", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("picante", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("sabroso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("insipido", CAT_ADJETIVO, SET_CUALIDAD);

    /* Estado */
    reg_vocab("abierto", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("cerrado", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("vacio", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("lleno", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("entero", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("roto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("completo", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("vivo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("muerto", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("encendido", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("apagado", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("presente", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("ausente", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("visible", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("invisible", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("dificil", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("facil", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("posible", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("imposible", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("seguro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("peligroso", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("libre", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("ocupado", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("limpio", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("sucio", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("seco", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("mojado", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("fuerte", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("debíl", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("sano", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("enfermo", CAT_ADJETIVO, SET_CUALIDAD);

    /* Personalidad y caracter */
    reg_vocab("bueno", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("malo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("amable", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("cruel", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("alegre", CAT_ADJETIVO, SET_CUALIDAD);    reg_vocab("triste", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("valiente", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("cobarde", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("listo", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("tonto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("sabio", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("ignorante", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("rico", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("pobre", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("joven", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("viejo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("educado", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("grosero", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("sincero", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("falso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("humilde", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("orgulloso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("generoso", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("egoista", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("tranquilo", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("nervioso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("divertido", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("serio", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("simpatico", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("inteligente", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("estupido", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("honesto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("mentiroso", CAT_ADJETIVO, SET_CUALIDAD);

    /* Otros adjetivos */
    reg_vocab("mismo", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("otro", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("diferente", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("igual", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("unico", CAT_ADJETIVO, SET_CUALIDAD);     reg_vocab("comun", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("raro", CAT_ADJETIVO, SET_CUALIDAD);      reg_vocab("normal", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("extrano", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("cierto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("verdadero", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("real", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("imaginario", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("similar", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("opuesto", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("proximo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("anterior", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("posterior", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("siguiente", CAT_ADJETIVO, SET_CUALIDAD); reg_vocab("ultimo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("primero", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("segundo", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("tercero", CAT_ADJETIVO, SET_CUALIDAD);   reg_vocab("perfecto", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("horrible", CAT_ADJETIVO, SET_CUALIDAD);  reg_vocab("hermoso", CAT_ADJETIVO, SET_CUALIDAD);
    reg_vocab("maravilloso", CAT_ADJETIVO, SET_CUALIDAD);

    /* ============ ADVERBIOS ============ */
    reg_vocab("aqui", CAT_ADVERBIO, 0);
    reg_vocab("alli", CAT_ADVERBIO, 0);
    reg_vocab("alla", CAT_ADVERBIO, 0);
    reg_vocab("ahi", CAT_ADVERBIO, 0);
    reg_vocab("cerca", CAT_ADVERBIO, 0);
    reg_vocab("lejos", CAT_ADVERBIO, 0);
    reg_vocab("dentro", CAT_ADVERBIO, 0);
    reg_vocab("fuera", CAT_ADVERBIO, 0);
    reg_vocab("arriba", CAT_ADVERBIO, 0);
    reg_vocab("abajo", CAT_ADVERBIO, 0);
    reg_vocab("delante", CAT_ADVERBIO, 0);
    reg_vocab("detras", CAT_ADVERBIO, 0);
    reg_vocab("encima", CAT_ADVERBIO, 0);
    reg_vocab("debajo", CAT_ADVERBIO, 0);
    reg_vocab("antes", CAT_ADVERBIO, 0);
    reg_vocab("despues", CAT_ADVERBIO, 0);
    reg_vocab("ahora", CAT_ADVERBIO, 0);
    reg_vocab("luego", CAT_ADVERBIO, 0);
    reg_vocab("pronto", CAT_ADVERBIO, 0);
    reg_vocab("tarde", CAT_ADVERBIO, 0);
    reg_vocab("temprano", CAT_ADVERBIO, 0);
    reg_vocab("siempre", CAT_ADVERBIO, 0);
    reg_vocab("nunca", CAT_ADVERBIO, 0);
    reg_vocab("jamas", CAT_ADVERBIO, 0);
    reg_vocab("ya", CAT_ADVERBIO, 0);
    reg_vocab("todavia", CAT_ADVERBIO, 0);
    reg_vocab("aun", CAT_ADVERBIO, 0);
    reg_vocab("mucho", CAT_ADVERBIO, 0);
    reg_vocab("poco", CAT_ADVERBIO, 0);
    reg_vocab("bastante", CAT_ADVERBIO, 0);
    reg_vocab("demasiado", CAT_ADVERBIO, 0);
    reg_vocab("casi", CAT_ADVERBIO, 0);
    reg_vocab("solo", CAT_ADVERBIO, 0);
    reg_vocab("tambien", CAT_ADVERBIO, 0);
    reg_vocab("tampoco", CAT_ADVERBIO, 0);
    reg_vocab("si", CAT_ADVERBIO, 0);
    reg_vocab("no", CAT_ADVERBIO, 0);
    reg_vocab("quizas", CAT_ADVERBIO, 0);
    reg_vocab("acaso", CAT_ADVERBIO, 0);
    reg_vocab("asi", CAT_ADVERBIO, 0);
    reg_vocab("bien", CAT_ADVERBIO, 0);
    reg_vocab("mal", CAT_ADVERBIO, 0);
    reg_vocab("apenas", CAT_ADVERBIO, 0);
    reg_vocab("seguramente", CAT_ADVERBIO, 0);
    reg_vocab("probablemente", CAT_ADVERBIO, 0);
    reg_vocab("ciertamente", CAT_ADVERBIO, 0);
    reg_vocab("efectivamente", CAT_ADVERBIO, 0);
    reg_vocab("entonces", CAT_ADVERBIO, 0);
    reg_vocab("ademas", CAT_ADVERBIO, 0);
    reg_vocab("finalmente", CAT_ADVERBIO, 0);
    reg_vocab("inmediatamente", CAT_ADVERBIO, 0);
    reg_vocab("rapidamente", CAT_ADVERBIO, 0);
    reg_vocab("lentamente", CAT_ADVERBIO, 0);
    reg_vocab("facilmente", CAT_ADVERBIO, 0);
    reg_vocab("dificilmente", CAT_ADVERBIO, 0);

    /* ============ CONJUNCIONES ============ */
    reg_vocab("y", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("e", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("o", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("u", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("pero", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("mas", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("sino", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("aunque", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("porque", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("pues", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("mientras", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("cuando", CAT_ARTICULO, SET_CONJUNCION);
    reg_vocab("asi", CAT_ARTICULO, SET_CONJUNCION);

    /* ============ INTERROGATIVOS ============ */
    reg_vocab("que", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("quien", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("quienes", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("cual", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("cuales", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("como", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("donde", CAT_PRONOMBRE, SET_INTERROGATIVO);
    reg_vocab("cuando", CAT_PRONOMBRE, SET_INTERROGATIVO);

    /* ============ NUMERALES ============ */
    reg_vocab("cero", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("uno", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("dos", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("tres", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("cuatro", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("cinco", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("seis", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("siete", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("ocho", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("nueve", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("diez", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("veinte", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("cien", CAT_SUSTANTIVO, SET_NUMERAL);
    reg_vocab("mil", CAT_SUSTANTIVO, SET_NUMERAL);
}

/* ---- Aprendizaje automatico de palabras ---- */

/* adivina categoria por terminacion */
static uint16_t adivinar_categoria(const char *palabra) {
    int len = (int)strlen(palabra);
    if (len < 3) return CAT_SUSTANTIVO;

    /* verbos */
    if (len >= 2) {
        if (strcmp(&palabra[len-2], "ar") == 0 ||
            strcmp(&palabra[len-2], "er") == 0 ||
            strcmp(&palabra[len-2], "ir") == 0)
            return CAT_VERBO;
    }

    /* terminaciones de adverbio */
    if (len >= 4 && strcmp(&palabra[len-4], "mente") == 0)
        return CAT_ADVERBIO;

    /* terminaciones de adjetivo comunes */
    const char *adj_suf[] = {"oso", "osa", "ivo", "iva", "ble", "al", "ico"};
    for (int i = 0; i < 7; i++) {
        int slen = (int)strlen(adj_suf[i]);
        if (len >= slen && strcmp(&palabra[len-slen], adj_suf[i]) == 0)
            return CAT_ADJETIVO;
    }

    return CAT_SUSTANTIVO;
}

int rae_aprender_de_texto(const char *texto) {
    if (!texto || !*texto) return 0;

    char buf[1024];
    strncpy(buf, texto, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    char *tokens[128];
    int n_tokens = 0;
    char *p = strtok(buf, " \t\n\r.,;:!?\"'()[]{}¿¡");
    while (p && n_tokens < 128) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        tokens[n_tokens++] = p;
        p = strtok(NULL, " \t\n\r.,;:!?\"'()[]{}¿¡");
    }

    int aprendidas = 0;
    for (int i = 0; i < n_tokens; i++) {
        char lower[64];
        lowercase(tokens[i], lower, sizeof(lower));
        if (strlen(lower) < 2) continue;

        /* ver si ya existe en NLP */
        uint16_t ids[4];
        int n_ids = nlp_lookup_all(lower, ids, 4);
        if (n_ids > 0) continue;

        /* ver si existe via stemmer */
        char stemmed[64];
        /* usar stemmer simple: quitar 'o','a','s','n' comunes */
        int slen = (int)strlen(lower);
        int stemmed_flag = 0;
        if (slen > 4 && (lower[slen-1] == 'o' || lower[slen-1] == 'a' ||
                         lower[slen-1] == 's' || lower[slen-1] == 'n')) {
            strncpy(stemmed, lower, slen-1);
            stemmed[slen-1] = 0;
            stemmed_flag = 1;
        }
        if (stemmed_flag) {
            n_ids = nlp_lookup_all(stemmed, ids, 4);
            if (n_ids > 0) continue;
        }

        /* palabra nueva: asignar ID y aprender */
        uint16_t cat = adivinar_categoria(lower);
        uint16_t concept_id = next_vocab_id++;
        nlp_add_term(lower, concept_id, NULL, -1, -1);
        add_concept_edge(concept_id, cat, 200, REL_PERTENECE);
        add_concept_edge(concept_id, SET_DESCONOCIDO, 150, REL_PERTENECE);
        uint8_t rtype = REL_SUSTANTIVO;
        if (cat == CAT_VERBO) rtype = REL_VERBO;
        else if (cat == CAT_ADJETIVO) rtype = REL_ADJETIVO;
        else if (cat == CAT_ADVERBIO) rtype = REL_ADVERBIO;
        uint8_t node_id = (uint8_t)(concept_id % BDGB_GRID_NODES);
        add_concept(node_id, concept_id, 150, rtype);
        aprendidas++;
    }

    if (aprendidas > 0) {
        nlp_save();
    }
    return aprendidas;
}

/* ---- API publica ---- */

int rae_init(const char *data_path) {
    if (rae_inited) return 0;

    reg_concepto(CAT_SUSTANTIVO, "sustantivo", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_VERBO, "verbo", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_ADJETIVO, "adjetivo", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_ADVERBIO, "adverbio", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_ARTICULO, "articulo", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_PREPOSICION, "preposicion", 0, SET_VOCABLO_BASE);
    reg_concepto(CAT_PRONOMBRE, "pronombre", 0, SET_VOCABLO_BASE);

    reg_concepto(SET_PERSONAJE, "conjunto_personaje", 0, 0);
    reg_concepto(SET_ESCENA, "conjunto_escena", 0, 0);
    reg_concepto(SET_ACCION_DIBUJO, "conjunto_accion_dibujo", 0, 0);
    reg_concepto(SET_ACCION_ARCHIVO, "conjunto_accion_archivo", 0, 0);
    reg_concepto(SET_HERRAMIENTA, "conjunto_herramienta", 0, 0);
    reg_concepto(SET_LUGAR, "conjunto_lugar", 0, 0);
    reg_concepto(SET_TIEMPO, "conjunto_tiempo", 0, 0);
    reg_concepto(SET_CUALIDAD, "conjunto_cualidad", 0, 0);
    reg_concepto(SET_PROYECTO, "conjunto_proyecto", 0, 0);
    reg_concepto(SET_OBJETO, "conjunto_objeto", 0, 0);
    reg_concepto(SET_ACCION_GIT, "conjunto_accion_git", 0, 0);
    reg_concepto(SET_ACCION_PROYECTO, "conjunto_accion_proyecto", 0, 0);
    reg_concepto(SET_VOCABLO_BASE, "conjunto_vocablo_base", 0, 0);
    reg_concepto(SET_FAMILIA, "conjunto_familia", 0, 0);
    reg_concepto(SET_ANIMAL, "conjunto_animal", 0, 0);
    reg_concepto(SET_NATURALEZA, "conjunto_naturaleza", 0, 0);
    reg_concepto(SET_COMIDA, "conjunto_comida", 0, 0);
    reg_concepto(SET_CUERPO, "conjunto_cuerpo", 0, 0);
    reg_concepto(SET_HOGAR, "conjunto_hogar", 0, 0);
    reg_concepto(SET_ROPA, "conjunto_ropa", 0, 0);
    reg_concepto(SET_CIUDAD, "conjunto_ciudad", 0, 0);
    reg_concepto(SET_EMOCION, "conjunto_emocion", 0, 0);
    reg_concepto(SET_CONJUNCION, "conjunto_conjuncion", 0, 0);
    reg_concepto(SET_INTERROGATIVO, "conjunto_interrogativo", 0, 0);
    reg_concepto(SET_NUMERAL, "conjunto_numeral", 0, 0);
    reg_concepto(SET_DESCONOCIDO, "conjunto_desconocido", 0, 0);

    sembrar_vocabulario();
    sembrar_vocabulario_general();

    rae_inited = 1;
    return 0;
}

int rae_aprender_palabra(const char *palabra, uint16_t concepto_id, uint16_t categoria) {
    if (!palabra || !*palabra) return -1;
    char lower[64];
    lowercase(palabra, lower, sizeof(lower));

    nlp_add_term(lower, concepto_id, NULL, -1, -1);

    if (categoria) {
        add_concept_edge(concepto_id, categoria, 200, REL_PERTENECE);
        uint8_t rtype = 0;
        switch (categoria) {
            case CAT_SUSTANTIVO:  rtype = REL_SUSTANTIVO; break;
            case CAT_VERBO:       rtype = REL_VERBO; break;
            case CAT_ADJETIVO:    rtype = REL_ADJETIVO; break;
            case CAT_ADVERBIO:    rtype = REL_ADVERBIO; break;
            case CAT_ARTICULO:    rtype = REL_ARTICULO; break;
            case CAT_PREPOSICION: rtype = REL_PREPOSICION; break;
            case CAT_PRONOMBRE:   rtype = REL_PRONOMBRE; break;
        }
        if (rtype) {
            uint8_t node_id = (uint8_t)(concepto_id % BDGB_GRID_NODES);
            add_concept(node_id, concepto_id, 150, rtype);
        }
    }
    nlp_save();
    return 0;
}

struct CatEntry {
    uint16_t concept_id;
    uint16_t cats[MAX_CATS_PER_TOKEN];
    int      n_cats;
    int      es_interrogativo;
};

static int buscar_palabra(const char *palabra, struct CatEntry *out) {
    if (!palabra || !*palabra || !out) return -1;
    char lower[64];
    lowercase(palabra, lower, sizeof(lower));

    out->n_cats = 0;
    out->concept_id = 0;
    out->es_interrogativo = 0;

    uint16_t ids[8];
    int n_ids = nlp_lookup_all(lower, ids, 8);
    if (n_ids <= 0) return -1;

    out->concept_id = ids[0];

    for (int i = 0; i < n_ids; i++) {
        uint16_t cats[MAX_CATS_PER_TOKEN];
        int nc = get_categorias(ids[i], cats, MAX_CATS_PER_TOKEN);
        for (int j = 0; j < nc && out->n_cats < MAX_CATS_PER_TOKEN; j++) {
            int dup = 0;
            for (int k = 0; k < out->n_cats; k++)
                if (out->cats[k] == cats[j]) { dup = 1; break; }
            if (!dup)
                out->cats[out->n_cats++] = cats[j];
        }
        /* ver si el concepto pertenece a SET_INTERROGATIVO */
        ConceptEdge sets[8];
        int ns = get_related_concepts(ids[i], sets, 8);
        for (int j = 0; j < ns; j++)
            if (sets[j].rel_type == REL_PERTENECE && sets[j].to_concept == SET_INTERROGATIVO)
                out->es_interrogativo = 1;
    }
    return 0;
}

/* devuelve la categoria mas util dado el estado actual */
static int tiene_articulo_y_pron(struct CatEntry *e) {
    int a = 0, p = 0;
    for (int i = 0; i < e->n_cats; i++) {
        if (e->cats[i] == CAT_ARTICULO) a = 1;
        if (e->cats[i] == CAT_PRONOMBRE) p = 1;
    }
    return a && p;
}

static int mejor_categoria(struct CatEntry *e, int fase, int paciente_ocupado) {
    if (e->n_cats == 0) return 0;
    if (e->n_cats == 1) return e->cats[0];

    int ambos = tiene_articulo_y_pron(e);
    int best = e->cats[0];
    int best_score = -999;

    for (int i = 0; i < e->n_cats; i++) {
        int cat = e->cats[i];
        int score = 0;

        /* cuando una palabra es Ambos articulo Y pronombre:
           en fases de busqueda de actante/paciente preferir articulo
           para que pase y el siguiente sustantivo tome el rol */
        int penalizado = (ambos && cat == CAT_PRONOMBRE &&
                          (fase == 0 || fase == 2));

        switch (fase) {
            case 0: /* FASE_ACTANTE */
                if (cat == CAT_PRONOMBRE && e->es_interrogativo) score = 60;
                else if (cat == CAT_VERBO)       score = 50;
                else if (cat == CAT_SUSTANTIVO) score = 40;
                else if (cat == CAT_PRONOMBRE)  score = penalizado ? 5 : 30;
                else if (cat == CAT_ARTICULO) score = 20;
                else score = -10;
                break;
            case 1: /* FASE_ACCION */
                if (cat == CAT_VERBO)       score = 50;
                else if (cat == CAT_ADVERBIO) score = 10;
                else score = -10;
                break;
            case 2: /* FASE_PACIENTE */
                if (!paciente_ocupado) {
                    if (cat == CAT_SUSTANTIVO) score = 40;
                    else if (cat == CAT_PRONOMBRE) score = penalizado ? 5 : 30;
                    else if (cat == CAT_ADJETIVO) score = 15;
                    else if (cat == CAT_ARTICULO) score = 20;
                    else if (cat == CAT_PREPOSICION) score = 10;
                    else score = -10;
                } else {
                    if (cat == CAT_ADJETIVO) score = 10;
                    else if (cat == CAT_PREPOSICION) score = 10;
                    else if (cat == CAT_ARTICULO) score = 5;
                    else score = -5;
                }
                break;
            case 3: /* FASE_CONTEXTO */
                if (cat == CAT_SUSTANTIVO)  score = 30;
                else if (cat == CAT_ARTICULO) score = 5;
                else score = -5;
                break;
        }
        if (score > best_score) {
            best_score = score;
            best = cat;
        }
    }
    return best;
}

/* ---- Verbo auxiliar ---- */
static int es_verbo_auxiliar(const char *palabra) {
    if (!palabra || !*palabra) return 0;
    char low[64];
    int i;
    for (i = 0; palabra[i] && i < 63; i++) low[i] = (char)tolower(palabra[i]);
    low[i] = 0;
    const char *aux[] = {
        "querer", "quiero", "quieres", "quiere", "queremos", "quieren",
        "poder", "puedo", "puedes", "puede", "podemos", "pueden",
        "deber", "debo", "debes", "debe", "debemos", "deben",
        "necesitar", "necesito", "necesitas", "necesita", "necesitamos", "necesitan",
        "ir", "voy", "vas", "va", "vamos", "van",
        "soler", "suelo", "sueles", "suele", "solemos", "suelen",
        NULL
    };
    for (int a = 0; aux[a]; a++)
        if (strcmp(low, aux[a]) == 0) return 1;
    return 0;
}

/* ---- Analisis de frase ---- */

int rae_analizar(const char *texto, FraseAnalizada *out) {
    if (!texto || !*texto || !out) return -1;

    memset(out, 0, sizeof(FraseAnalizada));
    strncpy(out->raw, texto, sizeof(out->raw) - 1);

    char buf[512];
    strncpy(buf, texto, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *tokens_str[MAX_TOKENS];
    int n_tokens = 0;
    char *p = strtok(buf, " \t\n\r.,;:!?\"'()[]{}");
    while (p && n_tokens < MAX_TOKENS) {
        while (*p == ' ') p++;
        if (!*p) { p = strtok(NULL, " \t\n\r.,;:!?\"'()[]{}"); continue; }
        tokens_str[n_tokens++] = p;
        p = strtok(NULL, " \t\n\r.,;:!?\"'()[]{}");
    }

    if (n_tokens == 0) return -1;

    struct CatEntry entries[MAX_TOKENS];
    int conocido[MAX_TOKENS];

    for (int i = 0; i < n_tokens; i++) {
        if (buscar_palabra(tokens_str[i], &entries[i]) == 0)
            conocido[i] = 1;
        else
            conocido[i] = 0;
    }

    enum { FASE_ACTANTE, FASE_ACCION, FASE_PACIENTE, FASE_CONTEXTO };
    int fase = FASE_ACTANTE;
    int confianza = 0;

    for (int i = 0; i < n_tokens; i++) {
        if (!conocido[i]) continue;

        uint16_t cat = mejor_categoria(&entries[i], fase, out->paciente_id != 0);
        uint16_t cid = entries[i].concept_id;

        switch (fase) {
            case FASE_ACTANTE:
                if (cat == CAT_ARTICULO) {
                    confianza += 10;
                } else if (cat == CAT_PRONOMBRE && entries[i].es_interrogativo) {
                    /* interrogativo: guardar tipo de pregunta */
                    char low[64];
                    lowercase(tokens_str[i], low, sizeof(low));
                    strncpy(out->pregunta, low, sizeof(out->pregunta) - 1);
                    confianza += 30;
                    fase = FASE_ACCION;
                } else if (cat == CAT_SUSTANTIVO || cat == CAT_PRONOMBRE) {
                    out->actante_id = cid;
                    strncpy(out->actante, tokens_str[i], sizeof(out->actante) - 1);
                    for (int c = 0; out->actante[c]; c++)
                        out->actante[c] = (char)tolower(out->actante[c]);
                    confianza += 40;
                    fase = FASE_ACCION;
                } else if (cat == CAT_VERBO) {
                    if (!es_verbo_auxiliar(tokens_str[i])) {
                        out->accion_id = cid;
                        strncpy(out->accion, tokens_str[i], sizeof(out->accion) - 1);
                        for (int c = 0; out->accion[c]; c++)
                            out->accion[c] = (char)tolower(out->accion[c]);
                        confianza += 30;
                        fase = FASE_PACIENTE;
                    } else {
                        /* auxiliar: pasar a FASE_ACCION */
                        confianza += 5;
                        fase = FASE_ACCION;
                    }
                }
                break;

            case FASE_ACCION:
                if (cat == CAT_VERBO) {
                    if (es_verbo_auxiliar(tokens_str[i])) {
                        confianza += 10;
                    } else {
                        out->accion_id = cid;
                        strncpy(out->accion, tokens_str[i], sizeof(out->accion) - 1);
                        for (int c = 0; out->accion[c]; c++)
                            out->accion[c] = (char)tolower(out->accion[c]);
                        confianza += 50;
                        fase = FASE_PACIENTE;
                    }
                } else if (cat == CAT_ADVERBIO) {
                    confianza += 10;
                }
                break;

            case FASE_PACIENTE:
                if (cat == CAT_ARTICULO) {
                    confianza += 5;
                } else if (cat == CAT_SUSTANTIVO || cat == CAT_PRONOMBRE) {
                    if (!out->paciente_id) {
                        out->paciente_id = cid;
                        strncpy(out->paciente, tokens_str[i], sizeof(out->paciente) - 1);
                        for (int c = 0; out->paciente[c]; c++)
                            out->paciente[c] = (char)tolower(out->paciente[c]);
                        confianza += 30;
                    } else if (cat == CAT_SUSTANTIVO) {
                        /* segundo sustantivo: paciente compuesto (multi-word patient) */
                        size_t plen = strlen(out->paciente);
                        snprintf(out->paciente + plen, sizeof(out->paciente) - plen,
                                 " %s", tokens_str[i]);
                        confianza += 15;
                    }
                } else if (cat == CAT_ADJETIVO) {
                    if (out->paciente_id) {
                        size_t plen = strlen(out->paciente);
                        snprintf(out->paciente + plen, sizeof(out->paciente) - plen,
                                 " %s", tokens_str[i]);
                        confianza += 10;
                    }
                } else if (cat == CAT_PREPOSICION) {
                    fase = FASE_CONTEXTO;
                }
                break;

            case FASE_CONTEXTO:
                if (cat == CAT_ARTICULO) {
                    confianza += 5;
                } else if (cat == CAT_SUSTANTIVO) {
                    if (!out->contexto_id) {
                        out->contexto_id = cid;
                        strncpy(out->contexto, tokens_str[i], sizeof(out->contexto) - 1);
                        confianza += 20;
                    }
                }
                break;
        }
    }

    /* Determinar intencion */
    /* Fallback: si llegamos a FASE_ACCION via auxiliar pero no hay verbo principal,
       usar el primer auxiliar como accion */
    if (out->accion_id == 0) {
        for (int i = 0; i < n_tokens; i++) {
            if (!conocido[i]) continue;
            uint16_t cat = mejor_categoria(&entries[i], -1, 0);
            if (cat == CAT_VERBO && es_verbo_auxiliar(tokens_str[i])) {
                out->accion_id = entries[i].concept_id;
                strncpy(out->accion, tokens_str[i], sizeof(out->accion) - 1);
                for (int c = 0; out->accion[c]; c++)
                    out->accion[c] = (char)tolower(out->accion[c]);
                confianza += 15;
                break;
            }
        }
    }

    if (out->accion_id) {
        ConceptEdge sets[16];
        int n = get_related_concepts(out->accion_id, sets, 16);
        int found = 0;
        for (int i = 0; i < n && !found; i++) {
            if (sets[i].rel_type == REL_PERTENECE &&
                sets[i].to_concept >= SET_ACCION_DIBUJO &&
                sets[i].to_concept <= SET_ACCION_PROYECTO) {
                strncpy(out->intencion, "ejecutar", sizeof(out->intencion) - 1);
                found = 1;
            }
        }
        if (!found)
            strncpy(out->intencion, "responder", sizeof(out->intencion) - 1);
    } else if (out->pregunta[0]) {
        strncpy(out->intencion, "responder", sizeof(out->intencion) - 1);
    } else {
        /* Sin accion ni pregunta: ver si es saludo o consulta */
        const char *saludos[] = {"hola", "buenos", "buenas", "saludos",
                                 "hey", "adios", "chao", "bye",
                                 "gracias", "thank", "thanks", NULL};
        int es_conversacion = 0;
        char lowcheck[256];
        for (int i = 0; texto[i] && i < 255; i++)
            lowcheck[i] = (char)tolower(texto[i]);
        lowcheck[255] = 0;
        for (int s = 0; saludos[s]; s++) {
            if (strstr(lowcheck, saludos[s]) == lowcheck) {
                es_conversacion = 1; break;
            }
        }
        if (es_conversacion)
            strncpy(out->intencion, "responder", sizeof(out->intencion) - 1);
        else
            strncpy(out->intencion, "consultar", sizeof(out->intencion) - 1);
    }

    out->confianza = confianza > 255 ? 255 : confianza;
    return 0;
}

/* ---- API de categorizacion (multi-categoria) ---- */

int rae_categorizar(const char *palabra, uint16_t *concepto_id, uint16_t *categoria) {
    if (!palabra || !*palabra) return -1;

    struct CatEntry e;
    if (buscar_palabra(palabra, &e) != 0) return -1;

    if (concepto_id) *concepto_id = e.concept_id;
    if (categoria)   *categoria  = (e.n_cats > 0) ? e.cats[0] : 0;
    return 0;
}

int rae_categorizar_multi(const char *palabra, uint16_t *conceptos,
                          uint16_t *categorias, int max_out) {
    if (!palabra || !*palabra || !conceptos || !categorias || max_out <= 0) return -1;
    char lower[64];
    lowercase(palabra, lower, sizeof(lower));

    int count = 0;
    uint16_t ids[8];
    int n_ids = nlp_lookup_all(lower, ids, 8);

    for (int i = 0; i < n_ids && count < max_out; i++) {
        uint16_t cats[MAX_CATS_PER_TOKEN];
        int nc = get_categorias(ids[i], cats, MAX_CATS_PER_TOKEN);
        if (nc == 0) {
            conceptos[count] = ids[i];
            categorias[count] = 0;
            count++;
        } else {
            for (int j = 0; j < nc && count < max_out; j++) {
                conceptos[count] = ids[i];
                categorias[count] = cats[j];
                count++;
            }
        }
    }
    return count;
}

/* ---- Teoria de conjuntos ---- */

int rae_pertenece_a_set(uint16_t concepto_id, uint16_t set_id) {
    ConceptEdge edges[16];
    int n = get_related_concepts(concepto_id, edges, 16);
    for (int i = 0; i < n; i++)
        if (edges[i].rel_type == REL_PERTENECE && edges[i].to_concept == set_id)
            return 1;
    return 0;
}

int rae_agregar_a_set(uint16_t concepto_id, uint16_t set_id) {
    if (rae_pertenece_a_set(concepto_id, set_id)) return 0;
    return add_concept_edge(concepto_id, set_id, 200, REL_PERTENECE);
}

int rae_obtener_miembros(uint16_t set_id, uint16_t *out, int max_out) {
    if (!out || max_out <= 0) return 0;
    int count = 0;
    for (int i = 0; i < BDGB_GRID_NODES; i++) {
        SemanticLink links[16];
        int n = find_concepts_by_node((uint8_t)i, links, 16);
        for (int j = 0; j < n && count < max_out; j++) {
            ConceptEdge edges[16];
            int m = get_related_concepts(links[j].concept_id, edges, 16);
            for (int k = 0; k < m && count < max_out; k++) {
                if (edges[k].rel_type == REL_PERTENECE && edges[k].to_concept == set_id) {
                    out[count++] = links[j].concept_id;
                }
            }
        }
    }
    return count;
}

int rae_listar_sets(uint16_t concepto_id, uint16_t *out, int max_out) {
    if (!out || max_out <= 0) return 0;
    ConceptEdge edges[32];
    int n = get_related_concepts(concepto_id, edges, 32);
    int count = 0;
    for (int i = 0; i < n && count < max_out; i++)
        if (edges[i].rel_type == REL_PERTENECE)
            out[count++] = edges[i].to_concept;
    return count;
}

void rae_imprimir_frase(const FraseAnalizada *f) {
    if (!f) return;
    printf("[RAE] '%s'\n", f->raw);
    printf("  ACTANTE:  %s (id=%d)\n", f->actante, f->actante_id);
    printf("  ACCION:   %s (id=%d)\n", f->accion, f->accion_id);
    printf("  PACIENTE: %s (id=%d)\n", f->paciente, f->paciente_id);
    printf("  CONTEXTO: %s (id=%d)\n", f->contexto, f->contexto_id);
    printf("  INTENCION: %s\n", f->intencion);
    if (f->pregunta[0])
        printf("  PREGUNTA: %s\n", f->pregunta);
    printf("  CONFIANZA: %d/255\n", f->confianza);
}

int rae_cargar_diccionario(const char *archivo) {
    (void)archivo;
    return 0;
}

int rae_salvar_diccionario(void) {
    nlp_save();
    return 0;
}

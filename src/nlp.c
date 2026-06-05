#include "nlp.h"
#include "bdgb.h"
#include "semantics.h"
#include "learning.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char word[32];
    uint16_t concept_id;
    int (*predicate)(const bdgb_props_t *);
    int dynamic_filter;
    int geom_type_filter;
} NLPTerm;

static NLPTerm dictionary[MAX_TERMS];
static int dict_count = 0;

void nlp_add_term(const char *word, uint16_t concept_id,
                  int (*pred)(const bdgb_props_t *),
                  int dyn_filter, int geom_filter) {
    if (dict_count >= MAX_TERMS) return;
    NLPTerm *t = &dictionary[dict_count++];
    strncpy(t->word, word, sizeof(t->word) - 1);
    t->concept_id = concept_id;
    t->predicate = pred;
    t->dynamic_filter = dyn_filter;
    t->geom_type_filter = geom_filter;
}

static int pred_simetrico(const bdgb_props_t *p) { return p->simetria; }
static int pred_denso(const bdgb_props_t *p) { return p->densidad >= 3; }
static int pred_radio1(const bdgb_props_t *p) { return p->radio == 1; }
static int pred_interior(const bdgb_props_t *p) { return p->tipo_geom == GEOM_INTERIOR; }
static int pred_esquina(const bdgb_props_t *p) { return p->tipo_geom == GEOM_CORNER; }
static int pred_borde(const bdgb_props_t *p) { return p->tipo_geom == GEOM_EDGE; }
static int pred_estable(const bdgb_props_t *p) { return p->clase_dinamica == DYN_ATTRACTOR; }

int nlp_init(void) {
    dict_count = 0;

    nlp_add_term("belleza",     1001, NULL,              -1, -1);
    nlp_add_term("resistencia", 2002, NULL,              -1, -1);
    nlp_add_term("volumen",     3003, NULL,              -1, -1);
    nlp_add_term("simetria",    4004, NULL,              -1, -1);

    nlp_add_term("simetrico",   0,    pred_simetrico,    -1, -1);
    nlp_add_term("simetrica",   0,    pred_simetrico,    -1, -1);
    nlp_add_term("denso",       0,    pred_denso,        -1, -1);
    nlp_add_term("densa",       0,    pred_denso,        -1, -1);
    nlp_add_term("estable",     0,    pred_estable,      -1, -1);
    nlp_add_term("interior",    0,    pred_interior,     -1, -1);
    nlp_add_term("esquina",     0,    pred_esquina,      -1, -1);
    nlp_add_term("borde",       0,    pred_borde,        -1, -1);
    nlp_add_term("central",     0,    pred_radio1,       -1, -1);

    nlp_add_term("atractor0",   0,    NULL,               0, -1);
    nlp_add_term("atractor7",   0,    NULL,               7, -1);
    nlp_add_term("atractor9",   0,    NULL,               9, -1);
    nlp_add_term("cerca0",      0,    NULL,               0, -1);
    nlp_add_term("cerca7",      0,    NULL,               7, -1);
    nlp_add_term("cerca9",      0,    NULL,               9, -1);

    return 0;
}

static int tokenize(const char *text, char tokens[][32], int max_tokens) {
    int count = 0;
    const char *p = text;
    while (*p && count < max_tokens) {
        while (*p == ' ' || *p == ',' || *p == '.') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && *p != ',' && *p != '.' && i < 31) {
            tokens[count][i++] = tolower((unsigned char)*p);
            p++;
        }
        tokens[count][i] = '\0';
        count++;
    }
    return count;
}

static NLPTerm *lookup_term(const char *word) {
    for (int i = 0; i < dict_count; i++) {
        if (strcmp(dictionary[i].word, word) == 0)
            return &dictionary[i];
    }
    return NULL;
}

int nlp_parse(const char *text, NLPQueryElement *out, int max_out) {
    char tokens[MAX_TOKENS][32];
    int n = tokenize(text, tokens, MAX_TOKENS);

    int count = 0;
    for (int i = 0; i < n && count < max_out; i++) {
        NLPTerm *term = lookup_term(tokens[i]);
        if (!term) continue;

        out[count].concept_id = term->concept_id;
        out[count].predicate = term->predicate;
        out[count].dynamic_filter = term->dynamic_filter;
        out[count].geom_type_filter = term->geom_type_filter;
        count++;
    }
    return count;
}

int nlp_search(const char *text, SearchResult *out, int max_out) {
    NLPQueryElement elems[16];
    int n = nlp_parse(text, elems, 16);

    uint16_t concept = 0;
    int (*pred)(const bdgb_props_t *) = NULL;
    int dyn = -1;
    int geom = -1;

    for (int i = 0; i < n; i++) {
        if (elems[i].concept_id) concept = elems[i].concept_id;
        if (elems[i].predicate)   pred = elems[i].predicate;
        if (elems[i].dynamic_filter != -1) dyn = elems[i].dynamic_filter;
        if (elems[i].geom_type_filter != -1) geom = elems[i].geom_type_filter;
    }

    int count = 0;

    if (concept > 0) {
        count = search_hybrid(concept, pred, (dyn >= 0) ? (uint8_t)dyn : 0,
                              (dyn >= 0) ? 1 : 0, out, max_out);
    } else if (dyn >= 0) {
        count = search_near_attractor((uint8_t)dyn, pred, out, max_out);
    } else if (geom >= 0) {
        count = search_by_geom_type((uint8_t)geom, pred, out, max_out);
    } else if (pred) {
        count = search_by_props(pred, out, max_out);
    }

    return count;
}

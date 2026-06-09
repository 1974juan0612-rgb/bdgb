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
static char nlp_data_path[512] = {0};

#define PRED_SIMETRICO  0
#define PRED_DENSO      1
#define PRED_RADIO1     2
#define PRED_INTERIOR   3
#define PRED_ESQUINA    4
#define PRED_BORDE      5
#define PRED_ESTABLE    6
#define PRED_NONE       255

static int pred_simetrico(const bdgb_props_t *p) { return p->simetria; }
static int pred_denso(const bdgb_props_t *p) { return p->densidad >= 3; }
static int pred_radio1(const bdgb_props_t *p) { return p->radio == 1; }
static int pred_interior(const bdgb_props_t *p) { return p->tipo_geom == GEOM_INTERIOR; }
static int pred_esquina(const bdgb_props_t *p) { return p->tipo_geom == GEOM_CORNER; }
static int pred_borde(const bdgb_props_t *p) { return p->tipo_geom == GEOM_EDGE; }
static int pred_estable(const bdgb_props_t *p) { return p->clase_dinamica == DYN_ATTRACTOR; }

static int (*predicate_table[])(const bdgb_props_t *) = {
    pred_simetrico, pred_denso, pred_radio1,
    pred_interior, pred_esquina, pred_borde, pred_estable
};
#define PRED_COUNT (sizeof(predicate_table)/sizeof(predicate_table[0]))

static int predicate_index(int (*pred)(const bdgb_props_t *)) {
    for (size_t i = 0; i < PRED_COUNT; i++)
        if (predicate_table[i] == pred) return (int)i;
    return PRED_NONE;
}

static int nlp_stem(const char *word, char *out, int outsz);
static int is_stopword(const char *word);

static int auto_predicate_idx(const char *word) {
    if (strstr(word, "simetr")) return PRED_SIMETRICO;
    if (strstr(word, "dens"))   return PRED_DENSO;
    if (strstr(word, "interior")) return PRED_INTERIOR;
    if (strcmp(word, "esquina") == 0 || strstr(word, "corner")) return PRED_ESQUINA;
    if (strcmp(word, "borde") == 0 || strstr(word, "edge")) return PRED_BORDE;
    if (strcmp(word, "estable") == 0 || strstr(word, "estabil")
        || strcmp(word, "attractor") == 0) return PRED_ESTABLE;
    if (strcmp(word, "central") == 0 || strcmp(word, "radio1") == 0) return PRED_RADIO1;
    return PRED_NONE;
}

void nlp_set_data_path(const char *path) {
    strncpy(nlp_data_path, path, sizeof(nlp_data_path) - 1);
}

static NLPTerm *lookup_term(const char *word) {
    for (int i = 0; i < dict_count; i++) {
        if (strcmp(dictionary[i].word, word) == 0) return &dictionary[i];
    }
    char stemmed[32];
    if (nlp_stem(word, stemmed, sizeof(stemmed))) {
        for (int i = 0; i < dict_count; i++) {
            if (strcmp(dictionary[i].word, stemmed) == 0) return &dictionary[i];
        }
    }
    return NULL;
}

void nlp_add_term(const char *word, uint16_t concept_id,
                  int (*pred)(const bdgb_props_t *),
                  int dyn_filter, int geom_filter) {
    if (dict_count >= MAX_TERMS) return;
    if (lookup_term(word)) return;
    NLPTerm *t = &dictionary[dict_count++];
    strncpy(t->word, word, sizeof(t->word) - 1);
    t->concept_id = concept_id;
    t->predicate = pred;
    t->dynamic_filter = dyn_filter;
    t->geom_type_filter = geom_filter;
}

/* ----- Persistence ----- */

static void make_nlp_path(char *buf, size_t bufsz, const char *filename) {
    if (nlp_data_path[0])
        snprintf(buf, bufsz, "%s/%s", nlp_data_path, filename);
    else
        snprintf(buf, bufsz, "%s", filename);
}

int nlp_save(void) {
    char path[512];
    make_nlp_path(path, sizeof(path), NLP_TERMS_FILE);
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    uint16_t count = (uint16_t)dict_count;
    fwrite(&count, sizeof(count), 1, f);
    for (int i = 0; i < dict_count; i++) {
        NLPTerm *t = &dictionary[i];
        uint8_t plen = (uint8_t)strlen(t->word);
        uint8_t pidx = (uint8_t)predicate_index(t->predicate);
        fwrite(&plen, 1, 1, f);
        fwrite(t->word, 1, plen, f);
        fwrite(&t->concept_id, sizeof(t->concept_id), 1, f);
        fwrite(&pidx, 1, 1, f);
        fwrite(&t->dynamic_filter, sizeof(t->dynamic_filter), 1, f);
        fwrite(&t->geom_type_filter, sizeof(t->geom_type_filter), 1, f);
    }
    fclose(f);
    return 0;
}

int nlp_load(void) {
    char path[512];
    make_nlp_path(path, sizeof(path), NLP_TERMS_FILE);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    uint16_t count = 0;
    if (fread(&count, sizeof(count), 1, f) != 1) { fclose(f); return 0; }
    int loaded = 0;
    for (uint16_t i = 0; i < count && dict_count < MAX_TERMS; i++) {
        uint8_t plen = 0;
        if (fread(&plen, 1, 1, f) != 1) break;
        if (plen >= 32) { fseek(f, plen + 2 + 1 + 4 + 4, SEEK_CUR); continue; }
        char word[32] = {0};
        if (fread(word, 1, plen, f) != plen) break;
        word[plen] = 0;
        uint16_t cid; uint8_t pidx; int dyn, geom;
        if (fread(&cid, sizeof(cid), 1, f) != 1) break;
        if (fread(&pidx, 1, 1, f) != 1) break;
        if (fread(&dyn, sizeof(dyn), 1, f) != 1) break;
        if (fread(&geom, sizeof(geom), 1, f) != 1) break;
        if (lookup_term(word)) continue;
        if (dict_count >= MAX_TERMS) break;
        NLPTerm *t = &dictionary[dict_count++];
        strncpy(t->word, word, sizeof(t->word) - 1);
        t->concept_id = cid;
        t->predicate = (pidx < PRED_COUNT) ? predicate_table[pidx] : NULL;
        t->dynamic_filter = dyn;
        t->geom_type_filter = geom;
        loaded++;
    }
    fclose(f);
    return loaded;
}

/* ----- Dynamic learning from text ----- */

int nlp_learn_from_text(const char *text, int max_terms) {
    if (!text) return 0;
    uint32_t next_cid = 5000;
    for (int i = 0; i < dict_count; i++) {
        if (dictionary[i].concept_id >= next_cid)
            next_cid = dictionary[i].concept_id + 1;
    }

    char tokens[MAX_TOKENS][32];
    int n = 0;
    const char *p = text;
    while (*p && n < MAX_TOKENS) {
        while (*p == ' ' || *p == ',' || *p == '.' || *p == '\n'
               || *p == '\r' || *p == '\t' || *p == '\"'
               || *p == '(' || *p == ')' || *p == '!' || *p == '?') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && *p != ',' && *p != '.' && *p != '\n'
               && *p != '\r' && *p != '\t' && i < 31) {
            tokens[n][i++] = tolower((unsigned char)*p);
            p++;
        }
        tokens[n][i] = 0;
        if (i >= 3 && i <= 20) n++;
        if (i > 20) n++;
    }

    int learned = 0;
    for (int i = 0; i < n && learned < max_terms; i++) {
        if (is_stopword(tokens[i])) continue;
        if (lookup_term(tokens[i])) continue;
        char stemmed[32];
        nlp_stem(tokens[i], stemmed, sizeof(stemmed));
        if (lookup_term(stemmed)) continue;
        const char *store = (stemmed[0] && strlen(stemmed) >= 3) ? stemmed : tokens[i];
        int pidx = auto_predicate_idx(store);
        int (*pred)(const bdgb_props_t *) = NULL;
        if (pidx < (int)PRED_COUNT) pred = predicate_table[pidx];

        uint16_t cid = (uint16_t)(next_cid & 0xFFFF);
        uint8_t node_id = (uint8_t)(cid % BDGB_GRID_NODES);
        nlp_add_term(store, cid, pred, -1, -1);
        uint8_t rel = (uint8_t)(cid % 5);
        add_concept(node_id, cid, 128, rel);
        learned++;
        next_cid++;
    }
    if (learned > 0) nlp_save();
    return learned;
}

int nlp_term_count(void) {
    return dict_count;
}

void nlp_list_terms(void) {
    printf("{\"count\":%d,\"terms\":[", dict_count);
    for (int i = 0; i < dict_count; i++) {
        if (i > 0) printf(",");
        printf("{\"word\":\"%s\",\"concept_id\":%u,\"dyn\":%d,\"geom\":%d}",
               dictionary[i].word, dictionary[i].concept_id,
               dictionary[i].dynamic_filter, dictionary[i].geom_type_filter);
    }
    printf("]}\n");
}

/* ----- Stopwords ----- */

static const char *stopwords[] = {
    "de", "la", "que", "el", "en", "y", "a", "los", "del", "se",
    "las", "por", "un", "para", "con", "no", "una", "su", "al",
    "es", "lo", "como", "mas", "pero", "sus", "le", "ya", "este",
    "entre", "porque", "este", "esta", "muy", "todo", "esa", "ese",
    "the", "and", "for", "are", "but", "not", "you", "all", "can",
    "had", "her", "was", "one", "our", "out", "has", "have", "been",
    NULL
};

static int is_stopword(const char *word) {
    for (int i = 0; stopwords[i]; i++) {
        if (strcmp(word, stopwords[i]) == 0) return 1;
    }
    return 0;
}

/* ----- Simple Spanish stemmer ----- */

static int nlp_stem(const char *word, char *out, int outsz) {
    int len = (int)strlen(word);
    if (len < 4) { strncpy(out, word, outsz - 1); out[outsz - 1] = 0; return 0; }

    char lower[64];
    for (int i = 0; i < len && i < 63; i++)
        lower[i] = tolower((unsigned char)word[i]);
    lower[len < 63 ? len : 63] = 0;
    len = (int)strlen(lower);

    int stemmed = 0;
    if (len > 6 && strcmp(&lower[len - 6], "mente") == 0)  { len -= 6; stemmed = 1; }
    if (len > 5 && strcmp(&lower[len - 5], "ción") == 0)   { len -= 5; stemmed = 1; }
    if (len > 5 && strcmp(&lower[len - 5], "sión") == 0)   { len -= 5; stemmed = 1; }
    if (len > 4 && strcmp(&lower[len - 4], "dad") == 0)    { len -= 4; stemmed = 1; }
    if (len > 4 && strcmp(&lower[len - 4], "tad") == 0)    { len -= 4; stemmed = 1; }
    if (!stemmed && len > 4 && strcmp(&lower[len - 4], "ando") == 0) { len -= 4; stemmed = 1; }
    if (len > 4 && strcmp(&lower[len - 4], "iendo") == 0)  { len -= 5; stemmed = 1; }

    if (len > 3 && (strcmp(&lower[len - 3], "aba") == 0 ||
                    strcmp(&lower[len - 3], "ido") == 0))  { len -= 3; stemmed = 1; }

    if (len > 2) {
        if (lower[len - 1] == 'o' || lower[len - 1] == 'a' ||
            lower[len - 1] == 's') {
            if (!(len > 3 && lower[len - 2] == 'u' && lower[len - 3] == 'q'))
                { len--; }
        }
    }

    if (len <= 0) len = 1;
    if (len >= outsz) len = outsz - 1;
    memcpy(out, lower, len);
    out[len] = 0;
    return stemmed;
}

/* ----- Core NLP ----- */

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

    nlp_load();
    return 0;
}

static int tokenize(const char *text, char tokens[][32], int max_tokens) {
    int count = 0;
    const char *p = text;
    while (*p && count < max_tokens) {
        while (*p == ' ' || *p == ',' || *p == '.' || *p == '\n'
               || *p == '\r' || *p == '\t' || *p == '\"'
               || *p == '(' || *p == ')' || *p == '!' || *p == '?'
               || *p == ';' || *p == ':' || *p == '-') p++;
        if (!*p) break;
        int i = 0;
        while (*p && *p != ' ' && *p != ',' && *p != '.' && *p != '\n'
               && *p != '\r' && *p != '\t' && *p != ';' && *p != ':'
               && *p != '-' && i < 31) {
            tokens[count][i++] = tolower((unsigned char)*p);
            p++;
        }
        tokens[count][i] = '\0';
        if (is_stopword(tokens[count])) continue;
        count++;
    }
    return count;
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

#include "concept_graph.h"
#include <stdio.h>
#include <string.h>

static char cg_path[256] = {0};
static int  cg_initialized = 0;

static void make_cg_path(char *buf, size_t bufsz, const char *filename) {
    snprintf(buf, bufsz, "%s/%s", cg_path, filename);
}

int concept_graph_init(const char *data_path) {
    strncpy(cg_path, data_path, sizeof(cg_path) - 1);
    cg_initialized = 1;
    return 0;
}

static int cg_file_exists(const char *filename) {
    char path[512];
    make_cg_path(path, sizeof(path), filename);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int add_concept_edge(uint16_t from, uint16_t to,
                     uint8_t weight, uint8_t rel_type) {
    if (!cg_initialized) return -1;

    ConceptEdge edge;
    edge.from_concept = from;
    edge.to_concept = to;
    edge.weight = weight;
    edge.rel_type = rel_type;

    char path[512];
    make_cg_path(path, sizeof(path), CONCEPT_EDGES_FILE);
    FILE *f = fopen(path, "ab");
    if (!f) return -1;

    size_t written = fwrite(&edge, sizeof(ConceptEdge), 1, f);
    fclose(f);

    return (written == 1) ? 0 : -1;
}

int get_related_concepts(uint16_t concept_id,
                         ConceptEdge *out, int max_out) {
    if (!cg_initialized) return -1;
    if (!cg_file_exists(CONCEPT_EDGES_FILE)) return 0;

    char path[512];
    make_cg_path(path, sizeof(path), CONCEPT_EDGES_FILE);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    ConceptEdge edge;
    int count = 0;

    while (fread(&edge, sizeof(ConceptEdge), 1, f) == 1 && count < max_out) {
        if (edge.from_concept == concept_id || edge.to_concept == concept_id) {
            out[count++] = edge;
        }
    }

    fclose(f);
    return count;
}

int concept_neighbors(uint16_t concept_id,
                      ConceptEdge *out, int max_out) {
    return get_related_concepts(concept_id, out, max_out);
}

static const char *cg_rel_type_name(uint8_t t) {
    switch (t) {
        case 0: return "definicion";
        case 1: return "ejemplo";
        case 2: return "metafora";
        case 3: return "funcion";
        case 4: return "causa";
        default: return "desconocido";
    }
}

void print_concept_edge(const ConceptEdge *edge) {
    printf("  Concepto %4u -> %4u (peso=%3u, tipo=%s)\n",
           edge->from_concept, edge->to_concept,
           edge->weight, cg_rel_type_name(edge->rel_type));
}

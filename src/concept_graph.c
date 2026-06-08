#include "concept_graph.h"
#include <stdio.h>
#include <string.h>

static char cg_path[512] = {0};
static int  cg_initialized = 0;

static CgHashBucket cg_hash[CG_HASH_BUCKETS];

static uint8_t cg_hash_u16(uint16_t key) {
    return (uint8_t)((key ^ (key >> 4)) & 0xFF);
}

static void make_cg_path(char *buf, size_t bufsz, const char *filename) {
    snprintf(buf, bufsz, "%s/%s", cg_path, filename);
}

int concept_graph_init(const char *data_path) {
    strncpy(cg_path, data_path, sizeof(cg_path) - 1);
    cg_initialized = 1;
    rebuild_cg_hash();
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

int rebuild_cg_hash(void) {
    memset(cg_hash, 0, sizeof(cg_hash));
    if (!cg_file_exists(CONCEPT_EDGES_FILE)) return 0;

    char path[512];
    make_cg_path(path, sizeof(path), CONCEPT_EDGES_FILE);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    ConceptEdge edge;
    while (fread(&edge, sizeof(ConceptEdge), 1, f) == 1) {
        uint8_t h = cg_hash_u16(edge.from_concept);
        CgHashBucket *bucket = &cg_hash[h];
        if (bucket->count < CG_HASH_CHAIN_MAX) {
            CgHashEntry *e = &bucket->entries[bucket->count++];
            e->from_concept = edge.from_concept;
            e->to_concept = edge.to_concept;
            e->weight = edge.weight;
            e->rel_type = edge.rel_type;
        }

        uint8_t h2 = cg_hash_u16(edge.to_concept);
        if (h2 != h) {
            CgHashBucket *bucket2 = &cg_hash[h2];
            if (bucket2->count < CG_HASH_CHAIN_MAX) {
                CgHashEntry *e = &bucket2->entries[bucket2->count++];
                e->from_concept = edge.from_concept;
                e->to_concept = edge.to_concept;
                e->weight = edge.weight;
                e->rel_type = edge.rel_type;
            }
        }
    }

    fclose(f);
    return 0;
}

int add_concept_edge(uint16_t from, uint16_t to,
                     uint8_t weight, uint8_t rel_type) {
    if (!cg_initialized) return -1;

    ConceptEdge existing[16];
    int n = get_related_concepts(from, existing, 16);
    for (int i = 0; i < n; i++) {
        if (existing[i].from_concept == from && existing[i].to_concept == to &&
            existing[i].rel_type == rel_type)
            return 0;
    }

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

    if (written != 1) return -1;

    uint8_t h = cg_hash_u16(from);
    CgHashBucket *bucket = &cg_hash[h];
    if (bucket->count < CG_HASH_CHAIN_MAX) {
        CgHashEntry *e = &bucket->entries[bucket->count++];
        e->from_concept = from;
        e->to_concept = to;
        e->weight = weight;
        e->rel_type = rel_type;
    }

    uint8_t h2 = cg_hash_u16(to);
    if (h2 != h) {
        CgHashBucket *bucket2 = &cg_hash[h2];
        if (bucket2->count < CG_HASH_CHAIN_MAX) {
            CgHashEntry *e = &bucket2->entries[bucket2->count++];
            e->from_concept = from;
            e->to_concept = to;
            e->weight = weight;
            e->rel_type = rel_type;
        }
    }

    return 0;
}

static int cg_entry_matches(CgHashEntry *e, uint16_t concept_id) {
    return (e->from_concept == concept_id || e->to_concept == concept_id);
}

int get_related_concepts(uint16_t concept_id,
                         ConceptEdge *out, int max_out) {
    if (!cg_initialized) return -1;

    uint8_t h = cg_hash_u16(concept_id);
    CgHashBucket *bucket = &cg_hash[h];
    int count = 0;

    for (int i = 0; i < bucket->count && count < max_out; i++) {
        CgHashEntry *e = &bucket->entries[i];
        if (cg_entry_matches(e, concept_id)) {
            out[count].from_concept = e->from_concept;
            out[count].to_concept = e->to_concept;
            out[count].weight = e->weight;
            out[count].rel_type = e->rel_type;
            count++;
        }
    }

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

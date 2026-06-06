#include "semantics.h"
#include "bdgb.h"
#include <stdio.h>
#include <string.h>

static char sem_path[512] = {0};
static int  sem_initialized = 0;

static SemHashBucket sem_hash[HASH_BUCKETS];

static uint8_t hash_u16(uint16_t key) {
    return (uint8_t)((key ^ (key >> 4)) & 0xFF);
}

static void make_sem_path(char *buf, size_t bufsz, const char *filename) {
    snprintf(buf, bufsz, "%s/%s", sem_path, filename);
}

int semantic_init(const char *data_path) {
    strncpy(sem_path, data_path, sizeof(sem_path) - 1);
    sem_initialized = 1;
    rebuild_sem_hash();
    return 0;
}

static int sem_file_exists(const char *filename) {
    char path[512];
    make_sem_path(path, sizeof(path), filename);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int rebuild_sem_hash(void) {
    memset(sem_hash, 0, sizeof(sem_hash));
    if (!sem_file_exists(SEMANTICS_FILE)) return 0;

    char path[512];
    make_sem_path(path, sizeof(path), SEMANTICS_FILE);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    SemanticLink link;
    while (fread(&link, sizeof(SemanticLink), 1, f) == 1) {
        uint8_t h = hash_u16(link.concept_id);
        SemHashBucket *bucket = &sem_hash[h];
        if (bucket->count < HASH_CHAIN_MAX) {
            SemHashEntry *e = &bucket->entries[bucket->count++];
            e->concept_id = link.concept_id;
            e->node_id = link.node_id;
            e->weight = link.weight;
            e->rel_type = link.rel_type;
        }
    }

    fclose(f);
    return 0;
}

int add_concept(uint8_t node_id, uint16_t concept_id,
                uint8_t weight, uint8_t rel_type) {
    if (!sem_initialized) return -1;

    SemanticLink link;
    link.node_id = node_id;
    link.concept_id = concept_id;
    link.weight = weight;
    link.rel_type = rel_type;

    char path[512];
    make_sem_path(path, sizeof(path), SEMANTICS_FILE);
    FILE *f = fopen(path, "ab");
    if (!f) return -1;

    size_t written = fwrite(&link, sizeof(SemanticLink), 1, f);
    fclose(f);

    if (written != 1) return -1;

    uint8_t h = hash_u16(concept_id);
    SemHashBucket *bucket = &sem_hash[h];
    if (bucket->count < HASH_CHAIN_MAX) {
        SemHashEntry *e = &bucket->entries[bucket->count++];
        e->concept_id = concept_id;
        e->node_id = node_id;
        e->weight = weight;
        e->rel_type = rel_type;
    }

    return 0;
}

int find_nodes_by_concept(uint16_t concept_id,
                          SemanticLink *out, int max_out) {
    if (!sem_initialized) return -1;

    uint8_t h = hash_u16(concept_id);
    SemHashBucket *bucket = &sem_hash[h];
    int count = 0;

    for (int i = 0; i < bucket->count && count < max_out; i++) {
        SemHashEntry *e = &bucket->entries[i];
        if (e->concept_id == concept_id) {
            out[count].node_id = e->node_id;
            out[count].concept_id = e->concept_id;
            out[count].weight = e->weight;
            out[count].rel_type = e->rel_type;
            count++;
        }
    }

    return count;
}

int find_concepts_by_node(uint8_t node_id,
                          SemanticLink *out, int max_out) {
    if (!sem_initialized) return -1;

    int count = 0;
    for (int b = 0; b < HASH_BUCKETS && count < max_out; b++) {
        SemHashBucket *bucket = &sem_hash[b];
        for (int i = 0; i < bucket->count && count < max_out; i++) {
            SemHashEntry *e = &bucket->entries[i];
            if (e->node_id == node_id) {
                out[count].node_id = e->node_id;
                out[count].concept_id = e->concept_id;
                out[count].weight = e->weight;
                out[count].rel_type = e->rel_type;
                count++;
            }
        }
    }

    return count;
}

int semantic_neighbors(uint8_t node_id,
                       SemanticLink *out, int max_out) {
    return find_concepts_by_node(node_id, out, max_out);
}

static const char *rel_type_name(uint8_t t) {
    switch (t) {
        case REL_DEFINICION: return "definicion";
        case REL_EJEMPLO:    return "ejemplo";
        case REL_METAFORA:   return "metafora";
        case REL_FUNCION:    return "funcion";
        case REL_CAUSA:      return "causa";
        default:             return "desconocido";
    }
}

void print_semantic_link(const SemanticLink *link) {
    printf("  Nodo %2u -> Concepto %4u (peso=%3u, tipo=%s)\n",
           link->node_id, link->concept_id,
           link->weight, rel_type_name(link->rel_type));
}

#ifndef BDGB_SEMANTICS_H
#define BDGB_SEMANTICS_H

#include <stdint.h>

#define SEMANTICS_FILE  "semantics.dat"
#define CONCEPTS_IDX    "concepts.idx"
#define HASH_BUCKETS    256
#define HASH_CHAIN_MAX  32

#define REL_DEFINICION  0
#define REL_EJEMPLO     1
#define REL_METAFORA    2
#define REL_FUNCION     3
#define REL_CAUSA       4

typedef struct {
    uint8_t  node_id;
    uint16_t concept_id;
    uint8_t  weight;
    uint8_t  rel_type;
} SemanticLink;

typedef struct {
    uint16_t concept_id;
    uint8_t  node_id;
    uint8_t  weight;
    uint8_t  rel_type;
} SemHashEntry;

typedef struct {
    SemHashEntry entries[HASH_CHAIN_MAX];
    int count;
} SemHashBucket;

int  add_concept(uint8_t node_id, uint16_t concept_id,
                 uint8_t weight, uint8_t rel_type);
int  semantic_init(const char *data_path);
int  find_nodes_by_concept(uint16_t concept_id,
                           SemanticLink *out, int max_out);
int  find_concepts_by_node(uint8_t node_id,
                           SemanticLink *out, int max_out);
int  semantic_neighbors(uint8_t node_id,
                        SemanticLink *out, int max_out);
void print_semantic_link(const SemanticLink *link);

int  rebuild_sem_hash(void);

#endif

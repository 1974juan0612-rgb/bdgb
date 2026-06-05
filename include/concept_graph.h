#ifndef BDGB_CONCEPT_GRAPH_H
#define BDGB_CONCEPT_GRAPH_H

#include <stdint.h>

#define CONCEPT_EDGES_FILE "concept_edges.dat"
#define CONCEPT_EDGES_IDX  "concept_edges.idx"

typedef struct {
    uint16_t from_concept;
    uint16_t to_concept;
    uint8_t  weight;
    uint8_t  rel_type;
} ConceptEdge;

int  add_concept_edge(uint16_t from, uint16_t to,
                      uint8_t weight, uint8_t rel_type);
int  concept_graph_init(const char *data_path);
int  get_related_concepts(uint16_t concept_id,
                          ConceptEdge *out, int max_out);
int  concept_neighbors(uint16_t concept_id,
                       ConceptEdge *out, int max_out);
void print_concept_edge(const ConceptEdge *edge);

#endif

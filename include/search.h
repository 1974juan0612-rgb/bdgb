#ifndef BDGB_SEARCH_H
#define BDGB_SEARCH_H

#include <stdint.h>
#include "bdgb.h"
#include "semantics.h"
#include "concept_graph.h"

#define MAX_RESULTS      128
#define SEARCH_VISITED   256

typedef struct {
    uint8_t  node_id;
    uint16_t concept_id;
    uint16_t score;
    uint8_t  path_len;
    uint16_t path[8];
} SearchResult;

typedef int (*PropPredicate)(const bdgb_props_t *props);

int search_by_props(PropPredicate pred, SearchResult *out, int max_out);

int search_by_concept_and_props(uint16_t concept_id, PropPredicate pred,
                                SearchResult *out, int max_out);

int search_semantic_deep(uint16_t concept_id, int depth,
                         SearchResult *out, int max_out);

int search_near_attractor(uint8_t attractor_id, PropPredicate pred,
                          SearchResult *out, int max_out);

int search_by_geom_type(uint8_t geom_type, PropPredicate pred,
                        SearchResult *out, int max_out);

int search_hybrid(uint16_t concept_id, PropPredicate pred,
                  uint8_t attractor_id, int has_attractor,
                  SearchResult *out, int max_out);

uint16_t compute_rank(uint8_t node_id, uint16_t concept_id,
                      uint8_t semantic_weight);

void reinforce_results(SearchResult *results, int count);
void reinforce_trajectory(const SearchResult *r);

void print_search_result(const SearchResult *r);

#endif

#ifndef BDGB_LEARNING_H
#define BDGB_LEARNING_H

#include <stdint.h>

#define USAGE_NODES_FILE  "usage_nodes.dat"
#define USAGE_CONCEPTS_FILE "usage_concepts.dat"
#define USAGE_EDGES_FILE  "usage_edges.dat"

#define REINFORCE_AMOUNT  10
#define DECAY_FACTOR      0.95f

int  learning_init(const char *data_path);

void reinforce_node(uint8_t node_id);
void reinforce_concept(uint16_t concept_id);
void reinforce_edge(uint16_t from, uint16_t to);

uint32_t get_node_usage(uint8_t node_id);
uint32_t get_concept_usage(uint16_t concept_id);
uint32_t get_edge_usage(uint16_t from, uint16_t to);

void decay_all(float factor);

uint16_t apply_learning_to_rank(uint8_t node_id, uint16_t base_score);

void save_usage(void);
void load_usage(void);

#endif

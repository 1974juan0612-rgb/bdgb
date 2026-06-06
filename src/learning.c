#include "learning.h"
#include "bdgb.h"
#include <stdio.h>
#include <string.h>

static char learn_path[512] = {0};
static int  learn_initialized = 0;

static uint32_t node_usage[BDGB_GRID_NODES];

#define MAX_CONCEPTS 256
static uint32_t concept_usage[MAX_CONCEPTS];

#define MAX_EDGES 256
static struct { uint16_t from; uint16_t to; uint32_t count; } edge_usage[MAX_EDGES];
static int edge_count = 0;

static void make_lpath(char *buf, size_t bufsz, const char *fn) {
    snprintf(buf, bufsz, "%s/%s", learn_path, fn);
}

int learning_init(const char *data_path) {
    strncpy(learn_path, data_path, sizeof(learn_path) - 1);
    learn_initialized = 1;

    memset(node_usage, 0, sizeof(node_usage));
    memset(concept_usage, 0, sizeof(concept_usage));
    memset(edge_usage, 0, sizeof(edge_usage));
    edge_count = 0;

    load_usage();
    return 0;
}

void reinforce_node(uint8_t node_id) {
    if (!learn_initialized) return;
    if ((uint16_t)node_id >= BDGB_GRID_NODES) return;
    if (node_usage[node_id] < 0xFFFFFFF0)
        node_usage[node_id] += REINFORCE_AMOUNT;
}

void reinforce_concept(uint16_t concept_id) {
    if (!learn_initialized) return;
    if (concept_id < MAX_CONCEPTS) {
        if (concept_usage[concept_id] < 0xFFFFFFF0)
            concept_usage[concept_id] += REINFORCE_AMOUNT;
    }
}

void reinforce_edge(uint16_t from, uint16_t to) {
    if (!learn_initialized) return;

    for (int i = 0; i < edge_count; i++) {
        if (edge_usage[i].from == from && edge_usage[i].to == to) {
            if (edge_usage[i].count < 0xFFFFFFF0)
                edge_usage[i].count += REINFORCE_AMOUNT;
            return;
        }
    }

    if (edge_count < MAX_EDGES) {
        edge_usage[edge_count].from = from;
        edge_usage[edge_count].to = to;
        edge_usage[edge_count].count = REINFORCE_AMOUNT;
        edge_count++;
    }
}

uint32_t get_node_usage(uint8_t node_id) {
    if (!learn_initialized || (uint16_t)node_id >= BDGB_GRID_NODES) return 0;
    return node_usage[node_id];
}

uint32_t get_concept_usage(uint16_t concept_id) {
    if (!learn_initialized) return 0;
    if (concept_id < MAX_CONCEPTS) return concept_usage[concept_id];
    return 0;
}

uint32_t get_edge_usage(uint16_t from, uint16_t to) {
    if (!learn_initialized) return 0;
    for (int i = 0; i < edge_count; i++) {
        if (edge_usage[i].from == from && edge_usage[i].to == to)
            return edge_usage[i].count;
    }
    return 0;
}

void decay_all(float factor) {
    if (!learn_initialized) return;

    for (int i = 0; i < BDGB_GRID_NODES; i++)
        node_usage[i] = (uint32_t)(node_usage[i] * factor);

    for (int i = 0; i < MAX_CONCEPTS; i++)
        concept_usage[i] = (uint32_t)(concept_usage[i] * factor);

    for (int i = 0; i < edge_count; i++)
        edge_usage[i].count = (uint32_t)(edge_usage[i].count * factor);
}

uint16_t apply_learning_to_rank(uint8_t node_id, uint16_t base_score) {
    if (!learn_initialized) return base_score;

    uint32_t usage = get_node_usage(node_id);
    uint16_t bonus = (usage > 1000) ? 150 :
                     (usage > 500)  ? 100 :
                     (usage > 100)  ? 50  :
                     (usage > 10)   ? 20  : 0;

    uint16_t result = base_score + bonus;
    return (result > 999) ? 999 : result;
}

void save_usage(void) {
    if (!learn_initialized) return;

    char path[512];

    make_lpath(path, sizeof(path), USAGE_NODES_FILE);
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(node_usage, sizeof(uint32_t), BDGB_GRID_NODES, f);
        fclose(f);
    }

    make_lpath(path, sizeof(path), USAGE_CONCEPTS_FILE);
    f = fopen(path, "wb");
    if (f) {
        fwrite(concept_usage, sizeof(uint32_t), MAX_CONCEPTS, f);
        fclose(f);
    }

    make_lpath(path, sizeof(path), USAGE_EDGES_FILE);
    f = fopen(path, "wb");
    if (f) {
        fwrite(&edge_count, sizeof(int), 1, f);
        fwrite(edge_usage, sizeof(edge_usage[0]), edge_count, f);
        fclose(f);
    }
}

void load_usage(void) {
    if (!learn_initialized) return;

    char path[512];

    make_lpath(path, sizeof(path), USAGE_NODES_FILE);
    FILE *f = fopen(path, "rb");
    if (f) {
        fread(node_usage, sizeof(uint32_t), BDGB_GRID_NODES, f);
        fclose(f);
    }

    make_lpath(path, sizeof(path), USAGE_CONCEPTS_FILE);
    f = fopen(path, "rb");
    if (f) {
        fread(concept_usage, sizeof(uint32_t), MAX_CONCEPTS, f);
        fclose(f);
    }

    make_lpath(path, sizeof(path), USAGE_EDGES_FILE);
    f = fopen(path, "rb");
    if (f) {
        fread(&edge_count, sizeof(int), 1, f);
        if (edge_count > MAX_EDGES) edge_count = MAX_EDGES;
        fread(edge_usage, sizeof(edge_usage[0]), edge_count, f);
        fclose(f);
    }
}

#ifndef BDGB_H
#define BDGB_H

#include <stdint.h>
#include <stdbool.h>

#define BDGB_GRID_BITS      8
#define BDGB_GRID_SIZE      16
#define BDGB_GRID_NODES     256
#define BDGB_MAX_NEIGHBORS  4
#define BDGB_VERSION        2

typedef enum {
    NODE_OK = 0,
    NODE_ERR_NOT_FOUND = -1,
    NODE_ERR_IO = -2,
    NODE_ERR_INVALID = -3
} bdgb_error_t;

typedef enum {
    GEOM_CORNER  = 0,
    GEOM_EDGE    = 1,
    GEOM_INTERIOR = 2
} bdgb_geom_type_t;

typedef enum {
    DYN_ATTRACTOR     = 0,
    DYN_PRE_ATTRACTOR = 1,
    DYN_TRANSIENT     = 2
} bdgb_dyn_class_t;

typedef struct {
    uint8_t id_bits;
    uint8_t x;
    uint8_t y;
    uint8_t flags;
} bdgb_node_t;

typedef struct {
    uint8_t radio;
    uint8_t densidad;
    uint8_t simetria;
    uint8_t tipo_geom;
    uint8_t clase_dinamica;
    int      pasos_atractor;
    uint8_t  atractor_id;
} bdgb_props_t;

typedef struct {
    uint8_t from_id;
    uint8_t to_id;
} bdgb_edge_t;

typedef struct {
    bdgb_node_t nodes[BDGB_MAX_NEIGHBORS];
    int count;
} bdgb_neighbors_t;

bdgb_error_t bdgb_init(const char *data_path);

bdgb_node_t bdgb_create_node(uint8_t id_bits);

bdgb_error_t bdgb_store_node(const bdgb_node_t *node);
bdgb_error_t bdgb_load_node(uint8_t id, bdgb_node_t *out_node);
bdgb_error_t bdgb_load_all_nodes(bdgb_node_t *out_nodes, int *out_count);

bdgb_neighbors_t bdgb_neighbors_geom(uint8_t id);
bdgb_error_t bdgb_store_edge_geom(const bdgb_edge_t *edge);

uint8_t bdgb_dynamic_rule(uint8_t id);
bdgb_neighbors_t bdgb_neighbors_dyn(uint8_t id);

int  bdgb_count_ones(uint8_t id);
int  bdgb_is_symmetric(uint8_t id);
uint8_t bdgb_compute_radio(uint8_t id);
bdgb_geom_type_t bdgb_geom_type(uint8_t id);
bdgb_dyn_class_t bdgb_dyn_class(uint8_t id);
int  bdgb_steps_to_attractor(uint8_t id);
uint8_t bdgb_find_attractor(uint8_t id);

bdgb_props_t bdgb_compute_props(uint8_t id);

void bdgb_print_node(const bdgb_node_t *node);
void bdgb_print_props(const bdgb_props_t *props);

#endif

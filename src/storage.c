#include "bdgb.h"
#include <stdio.h>
#include <string.h>

#define NODE_FILE "nodes.dat"
#define EDGE_GEOM_FILE "edges_geom.dat"
#define EDGE_DYN_FILE "edges_dyn.dat"
#define INDEX_FILE "index_id.dat"

static char data_path[256] = {0};
static int initialized = 0;

static void make_path(char *buf, size_t bufsz, const char *filename) {
    snprintf(buf, bufsz, "%s/%s", data_path, filename);
}

bdgb_error_t bdgb_init(const char *path) {
    strncpy(data_path, path, sizeof(data_path) - 1);
    initialized = 1;
    return NODE_OK;
}

static FILE *open_file(const char *filename, const char *mode) {
    char path[512];
    make_path(path, sizeof(path), filename);
    return fopen(path, mode);
}

bdgb_error_t bdgb_store_node(const bdgb_node_t *node) {
    if (!initialized) return NODE_ERR_INVALID;

    FILE *f = open_file(NODE_FILE, "r+b");
    if (!f) {
        f = open_file(NODE_FILE, "w+b");
        if (!f) return NODE_ERR_IO;
        for (int i = 0; i < BDGB_GRID_NODES; i++) {
            bdgb_node_t empty = {0, 0, 0, 0};
            fwrite(&empty, sizeof(bdgb_node_t), 1, f);
        }
    }

    fseek(f, node->id_bits * sizeof(bdgb_node_t), SEEK_SET);
    size_t written = fwrite(node, sizeof(bdgb_node_t), 1, f);
    fclose(f);

    return (written == 1) ? NODE_OK : NODE_ERR_IO;
}

bdgb_error_t bdgb_load_node(uint8_t id, bdgb_node_t *out_node) {
    if (!initialized) return NODE_ERR_INVALID;
    if (id >= BDGB_GRID_NODES) return NODE_ERR_NOT_FOUND;

    FILE *f = open_file(NODE_FILE, "rb");
    if (!f) return NODE_ERR_NOT_FOUND;

    fseek(f, id * sizeof(bdgb_node_t), SEEK_SET);
    size_t read = fread(out_node, sizeof(bdgb_node_t), 1, f);
    fclose(f);

    return (read == 1) ? NODE_OK : NODE_ERR_NOT_FOUND;
}

bdgb_error_t bdgb_load_all_nodes(bdgb_node_t *out_nodes, int *out_count) {
    if (!initialized) return NODE_ERR_INVALID;

    FILE *f = open_file(NODE_FILE, "rb");
    if (!f) {
        *out_count = 0;
        return NODE_OK;
    }

    *out_count = fread(out_nodes, sizeof(bdgb_node_t), BDGB_GRID_NODES, f);
    fclose(f);
    return NODE_OK;
}

bdgb_error_t bdgb_store_edge_geom(const bdgb_edge_t *edge) {
    if (!initialized) return NODE_ERR_INVALID;

    FILE *f = open_file(EDGE_GEOM_FILE, "ab");
    if (!f) return NODE_ERR_IO;

    size_t written = fwrite(edge, sizeof(bdgb_edge_t), 1, f);
    fclose(f);

    return (written == 1) ? NODE_OK : NODE_ERR_IO;
}

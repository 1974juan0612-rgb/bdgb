#include "bdgb.h"

bdgb_neighbors_t bdgb_neighbors_geom(uint8_t id) {
    bdgb_neighbors_t result;
    result.count = 0;

    if (id >= BDGB_GRID_NODES) return result;

    uint8_t size = BDGB_GRID_SIZE;
    uint8_t x = id & (size - 1);
    uint8_t y = id >> 2;

    uint8_t candidates[4][2] = {
        {x, y - 1},
        {x, y + 1},
        {x - 1, y},
        {x + 1, y}
    };

    for (int i = 0; i < 4; i++) {
        int8_t cx = candidates[i][0];
        int8_t cy = candidates[i][1];
        if (cx >= 0 && cx < size && cy >= 0 && cy < size) {
            uint8_t nid = (cy << 2) | cx;
            result.nodes[result.count++] = bdgb_create_node(nid);
        }
    }

    return result;
}

#include "bdgb.h"
#include <stdint.h>

bdgb_neighbors_t bdgb_neighbors_geom(uint8_t id) {
    bdgb_neighbors_t result;
    result.count = 0;

    if ((uint16_t)id >= BDGB_GRID_NODES) return result;

    int half = BDGB_GRID_BITS / 2;
    int half_mask = (1 << half) - 1;
    uint8_t size = BDGB_GRID_SIZE;
    uint8_t x = id & half_mask;
    uint8_t y = (id >> half) & half_mask;

    uint8_t candidates[4][2] = {
        {x, (uint8_t)(y - 1)},
        {x, (uint8_t)(y + 1)},
        {(uint8_t)(x - 1), y},
        {(uint8_t)(x + 1), y}
    };

    for (int i = 0; i < 4; i++) {
        int8_t cx = (int8_t)candidates[i][0];
        int8_t cy = (int8_t)candidates[i][1];
        if (cx >= 0 && cx < size && cy >= 0 && cy < size) {
            uint8_t nid = (uint8_t)((cy << half) | cx);
            result.nodes[result.count++] = bdgb_create_node(nid);
        }
    }

    return result;
}

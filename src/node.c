#include "bdgb.h"
#include <stdio.h>
#include <string.h>

#define BIT_MASK(bits) ((1 << (bits)) - 1)

static int count_ones_fallback(uint8_t id) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (id & (1 << i)) count++;
    }
    return count;
}

int bdgb_count_ones(uint8_t id) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(id);
#else
    return count_ones_fallback(id);
#endif
}

int bdgb_is_symmetric(uint8_t id) {
    uint8_t rev = 0;
    for (int i = 0; i < BDGB_GRID_BITS; i++) {
        rev = (rev << 1) | ((id >> i) & 1);
    }
    return (id == (rev >> (8 - BDGB_GRID_BITS))) ? 1 : 0;
}

bdgb_node_t bdgb_create_node(uint8_t id_bits) {
    bdgb_node_t node;
    int half = BDGB_GRID_BITS / 2;
    node.id_bits = id_bits & BIT_MASK(BDGB_GRID_BITS);
    node.x = id_bits & BIT_MASK(half);
    node.y = (id_bits >> half) & BIT_MASK(half);
    node.flags = 0;
    return node;
}

void bdgb_print_node(const bdgb_node_t *node) {
    printf("Node{id=%3u (", node->id_bits);
    for (int i = BDGB_GRID_BITS - 1; i >= 0; i--) {
        printf("%u", (node->id_bits >> i) & 1);
    }
    printf("), x=%u, y=%u", node->x, node->y);
    if (node->flags) {
        printf(", flags=0x%02x", node->flags);
    }
    printf("}");
}

uint8_t bdgb_dynamic_rule(uint8_t id) {
    id &= BIT_MASK(BDGB_GRID_BITS);
    int ones = bdgb_count_ones(id);
    int zeros = BDGB_GRID_BITS - ones;

    uint16_t largest;
    if (ones == 0) {
        largest = 0;
    } else {
        largest = ((1U << ones) - 1U) << zeros;
    }
    uint16_t smallest = (ones == 0) ? 0 : ((1U << ones) - 1U);

    uint8_t result = (largest > smallest) ? (uint8_t)(largest - smallest) : (uint8_t)(smallest - largest);
    return result & BIT_MASK(BDGB_GRID_BITS);
}

bdgb_neighbors_t bdgb_neighbors_dyn(uint8_t id) {
    bdgb_neighbors_t result;
    result.count = 0;

    uint8_t next = bdgb_dynamic_rule(id);
    if (next != id) {
        result.nodes[result.count++] = bdgb_create_node(next);
    }

    for (uint16_t prev = 0; prev < BDGB_GRID_NODES; prev++) {
        if (bdgb_dynamic_rule((uint8_t)prev) == id) {
            bool dup = false;
            for (int i = 0; i < result.count; i++) {
                if (result.nodes[i].id_bits == prev) { dup = true; break; }
            }
            if (!dup) {
                result.nodes[result.count++] = bdgb_create_node((uint8_t)prev);
                if (result.count >= BDGB_MAX_NEIGHBORS) break;
            }
        }
    }
    return result;
}

uint8_t bdgb_compute_radio(uint8_t id) {
    id &= BIT_MASK(BDGB_GRID_BITS);
    int half = BDGB_GRID_BITS / 2;
    int half_mask = BIT_MASK(half);
    int x = id & half_mask;
    int y = (id >> half) & half_mask;
    int cx = 2 * x - (BDGB_GRID_SIZE - 1);
    int cy = 2 * y - (BDGB_GRID_SIZE - 1);
    int dist2 = cx * cx + cy * cy;
    if (dist2 == 0) return 0;
    if (dist2 <= 2 * (BDGB_GRID_SIZE / 4) * (BDGB_GRID_SIZE / 4)) return 1;
    return 2;
}

bdgb_geom_type_t bdgb_geom_type(uint8_t id) {
    id &= BIT_MASK(BDGB_GRID_BITS);
    int half = BDGB_GRID_BITS / 2;
    int half_mask = BIT_MASK(half);
    int x = id & half_mask;
    int y = (id >> half) & half_mask;
    int max = BDGB_GRID_SIZE - 1;
    int is_corner = (x == 0 || x == max) && (y == 0 || y == max);
    int is_edge = (x == 0 || x == max) || (y == 0 || y == max);
    if (is_corner) return GEOM_CORNER;
    if (is_edge)   return GEOM_EDGE;
    return GEOM_INTERIOR;
}

uint8_t bdgb_find_attractor(uint8_t id) {
    uint8_t seen[32];
    int seen_count = 0;
    uint8_t current = id & BIT_MASK(BDGB_GRID_BITS);

    while (1) {
        for (int i = 0; i < seen_count; i++) {
            if (seen[i] == current) return current;
        }
        if (seen_count < 32) seen[seen_count++] = current;
        uint8_t next = bdgb_dynamic_rule(current);
        if (next == current) return current;
        current = next;
    }
}

int bdgb_steps_to_attractor(uint8_t id) {
    uint8_t target = bdgb_find_attractor(id);
    uint8_t current = id & BIT_MASK(BDGB_GRID_BITS);
    int steps = 0;

    while (current != target || steps == 0) {
        if (steps > 0 && current == target) break;
        current = bdgb_dynamic_rule(current);
        steps++;
        if (steps > 256) return -1;
    }
    return steps;
}

bdgb_dyn_class_t bdgb_dyn_class(uint8_t id) {
    id &= BIT_MASK(BDGB_GRID_BITS);
    uint8_t next = bdgb_dynamic_rule(id);
    if (next == id) return DYN_ATTRACTOR;
    uint8_t next2 = bdgb_dynamic_rule(next);
    if (next2 == next) return DYN_PRE_ATTRACTOR;
    return DYN_TRANSIENT;
}

bdgb_props_t bdgb_compute_props(uint8_t id) {
    bdgb_props_t props;
    id &= BIT_MASK(BDGB_GRID_BITS);
    props.radio = bdgb_compute_radio(id);
    props.densidad = (uint8_t)bdgb_count_ones(id);
    props.simetria = (uint8_t)bdgb_is_symmetric(id);
    props.tipo_geom = (uint8_t)bdgb_geom_type(id);
    props.clase_dinamica = (uint8_t)bdgb_dyn_class(id);
    props.pasos_atractor = bdgb_steps_to_attractor(id);
    props.atractor_id = bdgb_find_attractor(id);
    return props;
}

void bdgb_print_props(const bdgb_props_t *props) {
    static const char *geom_names[] = {"CORNER", "EDGE", "INTERIOR"};
    static const char *dyn_names[]  = {"ATTRACTOR", "PRE_ATTRACTOR", "TRANSIENT"};

    printf("props{radio=%u, densidad=%u, simetria=%s, tipo=%s, clase=%s, pasos=%d, atractor=%u}",
           props->radio,
           props->densidad,
           props->simetria ? "SI" : "NO",
           geom_names[props->tipo_geom % 3],
           dyn_names[props->clase_dinamica % 3],
           props->pasos_atractor,
           props->atractor_id);
}

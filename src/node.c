#include "bdgb.h"
#include <stdio.h>
#include <string.h>

static int count_ones_fallback(uint8_t id) {
    int count = 0;
    for (int i = 0; i < BDGB_GRID_BITS; i++) {
        if (id & (1 << i)) count++;
    }
    return count;
}

int bdgb_count_ones(uint8_t id) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(id & 0x0F);
#else
    return count_ones_fallback(id & 0x0F);
#endif
}

bdgb_node_t bdgb_create_node(uint8_t id_bits) {
    bdgb_node_t node;
    node.id_bits = id_bits & 0x0F;
    int half = BDGB_GRID_BITS / 2;
    node.x = id_bits & ((1 << half) - 1);
    node.y = (id_bits >> half) & ((1 << half) - 1);
    node.flags = 0;
    return node;
}

void bdgb_print_node(const bdgb_node_t *node) {
    printf("Node{id=%2u (", node->id_bits);
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
    id &= 0x0F;
    int ones = bdgb_count_ones(id);
    int zeros = BDGB_GRID_BITS - ones;

    uint8_t largest = ((1 << ones) - 1) << zeros;
    uint8_t smallest = (1 << ones) - 1;

    uint8_t result = (largest > smallest) ? (largest - smallest) : (smallest - largest);
    return result & 0x0F;
}

bdgb_neighbors_t bdgb_neighbors_dyn(uint8_t id) {
    bdgb_neighbors_t result;
    result.count = 0;

    uint8_t next = bdgb_dynamic_rule(id);
    if (next != id) {
        result.nodes[result.count++] = bdgb_create_node(next);
    }

    for (uint8_t prev = 0; prev < BDGB_GRID_NODES; prev++) {
        if (bdgb_dynamic_rule(prev) == id) {
            bool dup = false;
            for (int i = 0; i < result.count; i++) {
                if (result.nodes[i].id_bits == prev) { dup = true; break; }
            }
            if (!dup) {
                result.nodes[result.count++] = bdgb_create_node(prev);
                if (result.count >= BDGB_MAX_NEIGHBORS) break;
            }
        }
    }
    return result;
}

int bdgb_is_symmetric(uint8_t id) {
    id &= 0x0F;
    uint8_t rev = 0;
    for (int i = 0; i < BDGB_GRID_BITS; i++) {
        rev = (rev << 1) | ((id >> i) & 1);
    }
    return (id == rev) ? 1 : 0;
}

uint8_t bdgb_compute_radio(uint8_t id) {
    id &= 0x0F;
    int x = id & 3;
    int y = (id >> 2) & 3;
    int dx = 2 * x - 3;
    int dy = 2 * y - 3;
    int dist2 = dx * dx + dy * dy;
    if (dist2 == 0) return 0;
    if (dist2 <= 8) return 1;
    return 2;
}

bdgb_geom_type_t bdgb_geom_type(uint8_t id) {
    id &= 0x0F;
    uint8_t x = id & 3;
    uint8_t y = (id >> 2) & 3;
    int is_corner = (x == 0 || x == 3) && (y == 0 || y == 3);
    int is_edge = (x == 0 || x == 3) || (y == 0 || y == 3);
    if (is_corner) return GEOM_CORNER;
    if (is_edge)   return GEOM_EDGE;
    return GEOM_INTERIOR;
}

uint8_t bdgb_find_attractor(uint8_t id) {
    uint8_t seen[16];
    int seen_count = 0;
    uint8_t current = id & 0x0F;

    while (1) {
        for (int i = 0; i < seen_count; i++) {
            if (seen[i] == current) return current;
        }
        if (seen_count < 16) seen[seen_count++] = current;
        uint8_t next = bdgb_dynamic_rule(current);
        if (next == current) return current;
        current = next;
    }
}

int bdgb_steps_to_attractor(uint8_t id) {
    uint8_t target = bdgb_find_attractor(id);
    uint8_t current = id & 0x0F;
    int steps = 0;

    while (current != target || steps == 0) {
        if (steps > 0 && current == target) break;
        current = bdgb_dynamic_rule(current);
        steps++;
        if (steps > 16) return -1;
    }
    return steps;
}

bdgb_dyn_class_t bdgb_dyn_class(uint8_t id) {
    id &= 0x0F;
    uint8_t next = bdgb_dynamic_rule(id);
    if (next == id) return DYN_ATTRACTOR;
    uint8_t next2 = bdgb_dynamic_rule(next);
    if (next2 == next) return DYN_PRE_ATTRACTOR;
    return DYN_TRANSIENT;
}

bdgb_props_t bdgb_compute_props(uint8_t id) {
    bdgb_props_t props;
    id &= 0x0F;
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

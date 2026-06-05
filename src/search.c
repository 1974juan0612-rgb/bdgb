#include "search.h"
#include "learning.h"
#include <stdio.h>
#include <string.h>

static int node_visited(uint8_t id, uint8_t *visited, int *vcount) {
    for (int i = 0; i < *vcount; i++)
        if (visited[i] == id) return 1;
    if (*vcount < SEARCH_VISITED) visited[(*vcount)++] = id;
    return 0;
}

static int concept_visited(uint16_t cid, uint16_t *visited, int *vcount) {
    for (int i = 0; i < *vcount; i++)
        if (visited[i] == cid) return 1;
    if (*vcount < SEARCH_VISITED) visited[(*vcount)++] = cid;
    return 0;
}

static int result_exists(SearchResult *out, int count, uint8_t node_id, uint16_t concept_id) {
    for (int i = 0; i < count; i++)
        if (out[i].node_id == node_id && out[i].concept_id == concept_id) return 1;
    return 0;
}

uint16_t compute_rank(uint8_t node_id, uint16_t concept_id, uint8_t semantic_weight) {
    bdgb_props_t p = bdgb_compute_props(node_id);
    uint16_t score = 0;

    score += semantic_weight;

    if (p.clase_dinamica == DYN_ATTRACTOR) score += 150;
    else if (p.clase_dinamica == DYN_PRE_ATTRACTOR) score += 80;

    if (p.simetria) score += 100;

    if (p.tipo_geom == GEOM_INTERIOR) score += 60;
    else if (p.tipo_geom == GEOM_EDGE) score += 30;
    else score += 15;

    score += p.densidad * 30;

    if (p.pasos_atractor <= 1) score += 50;

    uint16_t capped = (score > 999) ? 999 : score;
    return apply_learning_to_rank(node_id, capped);
}

int search_by_props(PropPredicate pred, SearchResult *out, int max_out) {
    int count = 0;
    for (uint8_t id = 0; id < BDGB_GRID_NODES && count < max_out; id++) {
        bdgb_props_t p = bdgb_compute_props(id);
        if (pred(&p)) {
            out[count].node_id = id;
            out[count].concept_id = 0;
            out[count].score = compute_rank(id, 0, 200);
            out[count].path_len = 0;
            count++;
        }
    }
    return count;
}

int search_by_concept_and_props(uint16_t concept_id, PropPredicate pred,
                                SearchResult *out, int max_out) {
    SemanticLink links[32];
    int n = find_nodes_by_concept(concept_id, links, 32);
    if (n <= 0) return 0;

    int count = 0;
    for (int i = 0; i < n && count < max_out; i++) {
        bdgb_props_t p = bdgb_compute_props(links[i].node_id);
        if (!pred || pred(&p)) {
            out[count].node_id = links[i].node_id;
            out[count].concept_id = concept_id;
            out[count].score = compute_rank(links[i].node_id, concept_id, links[i].weight);
            out[count].path_len = 1;
            out[count].path[0] = concept_id;
            count++;
        }
    }
    return count;
}

static void search_deep_rec(uint16_t concept_id, int depth,
                            SearchResult *out, int *count, int max_out,
                            uint16_t *con_visited, int *cv_count,
                            uint8_t *nod_visited, int *nv_count,
                            uint16_t *path, int path_len) {
    if (depth <= 0 || *count >= max_out) return;

    ConceptEdge edges[32];
    int e = get_related_concepts(concept_id, edges, 32);

    for (int i = 0; i < e && *count < max_out; i++) {
        uint16_t rel_id = edges[i].to_concept;

        SemanticLink links[16];
        int n = find_nodes_by_concept(rel_id, links, 16);
        for (int j = 0; j < n && *count < max_out; j++) {
            if (node_visited(links[j].node_id, nod_visited, nv_count)) continue;
            if (result_exists(out, *count, links[j].node_id, rel_id)) continue;

            int idx = (*count)++;
            out[idx].node_id = links[j].node_id;
            out[idx].concept_id = rel_id;
            out[idx].score = compute_rank(links[j].node_id, rel_id, edges[i].weight);
            out[idx].path_len = path_len + 1;
            for (int k = 0; k < path_len && k < 7; k++)
                out[idx].path[k] = path[k];
            out[idx].path[path_len] = rel_id;
            if (out[idx].path_len > 8) out[idx].path_len = 8;
        }

        if (!concept_visited(rel_id, con_visited, cv_count)) {
            uint16_t new_path[8];
            for (int k = 0; k < path_len && k < 7; k++) new_path[k] = path[k];
            new_path[path_len] = rel_id;
            search_deep_rec(rel_id, depth - 1, out, count, max_out,
                          con_visited, cv_count,
                          nod_visited, nv_count,
                          new_path, path_len + 1);
        }
    }
}

int search_semantic_deep(uint16_t concept_id, int depth,
                         SearchResult *out, int max_out) {
    uint16_t con_visited[SEARCH_VISITED];
    uint8_t  nod_visited[SEARCH_VISITED];
    int cv_count = 0, nv_count = 0;

    SemanticLink self[16];
    int ns = find_nodes_by_concept(concept_id, self, 16);
    for (int i = 0; i < ns; i++)
        node_visited(self[i].node_id, nod_visited, &nv_count);

    concept_visited(concept_id, con_visited, &cv_count);

    uint16_t path[8];
    path[0] = concept_id;

    int count = 0;
    search_deep_rec(concept_id, depth, out, &count, max_out,
                    con_visited, &cv_count,
                    nod_visited, &nv_count,
                    path, 1);
    return count;
}

int search_near_attractor(uint8_t attractor_id, PropPredicate pred,
                          SearchResult *out, int max_out) {
    int count = 0;
    for (uint8_t id = 0; id < BDGB_GRID_NODES && count < max_out; id++) {
        bdgb_props_t p = bdgb_compute_props(id);
        if (p.atractor_id == attractor_id && (!pred || pred(&p))) {
            out[count].node_id = id;
            out[count].concept_id = 0;
            out[count].score = compute_rank(id, 0, 200);
            out[count].path_len = 0;
            count++;
        }
    }
    return count;
}

int search_by_geom_type(uint8_t geom_type, PropPredicate pred,
                        SearchResult *out, int max_out) {
    int count = 0;
    for (uint8_t id = 0; id < BDGB_GRID_NODES && count < max_out; id++) {
        bdgb_props_t p = bdgb_compute_props(id);
        if (p.tipo_geom == geom_type && (!pred || pred(&p))) {
            out[count].node_id = id;
            out[count].concept_id = 0;
            out[count].score = compute_rank(id, 0, 200);
            out[count].path_len = 0;
            count++;
        }
    }
    return count;
}

int search_hybrid(uint16_t concept_id, PropPredicate pred,
                  uint8_t attractor_id, int has_attractor,
                  SearchResult *out, int max_out) {
    SemanticLink links[32];
    int n = find_nodes_by_concept(concept_id, links, 32);
    if (n <= 0) return 0;

    uint8_t nod_visited[SEARCH_VISITED];
    int nv_count = 0;

    int count = 0;
    for (int i = 0; i < n && count < max_out; i++) {
        uint8_t id = links[i].node_id;
        if (node_visited(id, nod_visited, &nv_count)) continue;

        bdgb_props_t p = bdgb_compute_props(id);
        if (pred && !pred(&p)) continue;
        if (has_attractor && p.atractor_id != attractor_id) continue;

        out[count].node_id = id;
        out[count].concept_id = concept_id;
        out[count].score = compute_rank(id, concept_id, links[i].weight);
        out[count].path_len = 1;
        out[count].path[0] = concept_id;
        count++;
    }
    return count;
}

void reinforce_results(SearchResult *results, int count) {
    for (int i = 0; i < count; i++) {
        reinforce_node(results[i].node_id);
        if (results[i].concept_id)
            reinforce_concept(results[i].concept_id);

        if (results[i].path_len > 1) {
            for (int j = 0; j < results[i].path_len - 1; j++) {
                reinforce_edge(results[i].path[j], results[i].path[j + 1]);
            }
        }
    }
}

void reinforce_trajectory(const SearchResult *r) {
    reinforce_node(r->node_id);
    if (r->concept_id) reinforce_concept(r->concept_id);
    if (r->path_len > 1) {
        for (int j = 0; j < r->path_len - 1; j++)
            reinforce_edge(r->path[j], r->path[j + 1]);
    }
}

void print_search_result(const SearchResult *r) {
    printf("  #%-2u ", r->node_id);
    if (r->concept_id) printf("[C%04u] ", r->concept_id);

    if (r->path_len > 1) {
        printf("(");
        for (int i = 0; i < r->path_len && i < 8; i++) {
            if (i > 0) printf(" -> ");
            printf("%u", r->path[i]);
        }
        printf(") ");
    }

    printf("rank=%3u (uso:%lu)", r->score,
           (unsigned long)get_node_usage(r->node_id));
    bdgb_props_t p = bdgb_compute_props(r->node_id);
    printf(" | ");
    bdgb_print_props(&p);
    printf("\n");
}

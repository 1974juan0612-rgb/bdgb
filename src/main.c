#include "bdgb.h"
#include "semantics.h"
#include "concept_graph.h"
#include "search.h"
#include "learning.h"
#include <stdio.h>
#include <stdlib.h>

static const char *DATA_PATH = "C:/Users/famil/Desktop/glifos/bdgb/data";

static void clear_data(void) {
    char path[512];
    const char *files[] = {"nodes.dat", "edges_geom.dat", "edges_dyn.dat",
                           "semantics.dat", "concepts.idx",
                           "concept_edges.dat", "concept_edges.idx",
                           "usage_nodes.dat", "usage_concepts.dat",
                           "usage_edges.dat", NULL};
    for (int i = 0; files[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", DATA_PATH, files[i]);
        remove(path);
    }
}

static void init_system(void) {
    for (uint8_t id = 0; id < BDGB_GRID_NODES; id++) {
        bdgb_node_t node = bdgb_create_node(id);
        bdgb_store_node(&node);
    }
}

static void init_semantics(void) {
    add_concept(6,  1001, 200, REL_DEFINICION);
    add_concept(9,  1001, 180, REL_METAFORA);
    add_concept(15, 1001, 150, REL_EJEMPLO);
    add_concept(0,  2002, 255, REL_FUNCION);
    add_concept(7,  2002, 220, REL_CAUSA);
    add_concept(9,  2002, 190, REL_DEFINICION);
    add_concept(5,  3003, 210, REL_METAFORA);
    add_concept(10, 3003, 175, REL_FUNCION);
    add_concept(3,  4004, 240, REL_DEFINICION);
    add_concept(12, 4004, 160, REL_EJEMPLO);

    add_concept_edge(1001, 3003, 200, REL_METAFORA);
    add_concept_edge(1001, 4004, 230, REL_DEFINICION);
    add_concept_edge(2002, 4004, 255, REL_CAUSA);
    add_concept_edge(3003, 2002, 180, REL_FUNCION);
    add_concept_edge(4004, 1001, 210, REL_EJEMPLO);
}

static int pred_simetrico(const bdgb_props_t *p) { return p->simetria; }

static int cmp_score(const void *a, const void *b) {
    int sa = ((const SearchResult*)a)->score;
    int sb = ((const SearchResult*)b)->score;
    return (sa < sb) - (sa > sb);
}

static void run_session(int session_num) {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║     SESION DE APRENDIZAJE #%d          ║\n", session_num);
    printf("╚══════════════════════════════════════╝\n\n");

    SearchResult results[MAX_RESULTS];
    int n;

    printf("Busqueda: 'belleza (1001) + simetrico + atractor 9'\n");
    n = search_hybrid(1001, pred_simetrico, 9, 1, results, MAX_RESULTS);
    reinforce_results(results, n);
    qsort(results, n, sizeof(SearchResult), cmp_score);
    for (int i = 0; i < n; i++) print_search_result(&results[i]);

    printf("\nBusqueda: 'resistencia (2002) cerca del atractor 7'\n");
    n = search_hybrid(2002, NULL, 7, 1, results, MAX_RESULTS);
    reinforce_results(results, n);
    qsort(results, n, sizeof(SearchResult), cmp_score);
    for (int i = 0; i < n; i++) print_search_result(&results[i]);

    printf("\nBusqueda profunda desde belleza (profundidad=1):\n");
    n = search_semantic_deep(1001, 1, results, MAX_RESULTS);
    reinforce_results(results, n);
    qsort(results, n, sizeof(SearchResult), cmp_score);
    for (int i = 0; i < n; i++) print_search_result(&results[i]);

    printf("\n>> Nodo mas usado hasta ahora: #%u (%lu usos)\n",
           (unsigned)get_node_usage(9) > get_node_usage(7) ? 9 : 7,
           (unsigned long)(get_node_usage(9) > get_node_usage(7) ?
                           get_node_usage(9) : get_node_usage(7)));
}

static void show_usage_table(void) {
    printf("\n=== TABLA DE USO ACUMULADO ===\n\n");
    printf("Nodo  Usos     Concepto  Usos\n");
    printf("────  ───────  ────────  ───────\n");
    for (uint8_t id = 0; id < BDGB_GRID_NODES; id++) {
        uint32_t nu = get_node_usage(id);
        if (nu > 0) {
            printf(" #%2u  %7lu", id, (unsigned long)nu);
            if (id <= 4) {
                uint16_t cids[] = {1001, 2002, 3003, 4004};
                uint32_t cu = get_concept_usage(cids[id]);
                printf("     C%04u  %7lu", cids[id], (unsigned long)cu);
            }
            printf("\n");
        }
    }
    printf("\n");
}

int main(void) {
    clear_data();
    bdgb_init(DATA_PATH);
    semantic_init(DATA_PATH);
    concept_graph_init(DATA_PATH);
    learning_init(DATA_PATH);

    init_system();
    init_semantics();

    printf("=== BDGB: APRENDIZAJE ADAPTATIVO ===\n\n");

    for (int s = 1; s <= 3; s++) {
        run_session(s);
        if (s < 3) {
            printf("\nPresiona Enter para siguiente sesion...\n");
            getchar();
        }
    }

    printf("\nAplicando decaimiento (factor=0.95)...\n");
    decay_all(0.95f);
    save_usage();

    show_usage_table();

    printf("=== BDGB con aprendizaje adaptativo lista ===\n");
    return 0;
}

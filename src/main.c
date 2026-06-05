#include "bdgb.h"
#include "semantics.h"
#include "concept_graph.h"
#include "search.h"
#include "learning.h"
#include "nlp.h"
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

static int cmp_score(const void *a, const void *b) {
    int sa = ((const SearchResult*)a)->score;
    int sb = ((const SearchResult*)b)->score;
    return (sa < sb) - (sa > sb);
}

static void run_query(const char *label, const char *query) {
    printf(">>> \"%s\"\n", query);
    printf("    %s\n\n", label);

    SearchResult results[MAX_RESULTS];
    int n = nlp_search(query, results, MAX_RESULTS);
    reinforce_results(results, n);
    qsort(results, n, sizeof(SearchResult), cmp_score);

    if (n == 0) {
        printf("    (sin resultados)\n\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        printf("    ");
        print_search_result(&results[i]);
    }
    printf("\n");
}

int main(void) {
    clear_data();
    bdgb_init(DATA_PATH);
    semantic_init(DATA_PATH);
    concept_graph_init(DATA_PATH);
    learning_init(DATA_PATH);
    nlp_init();

    init_system();
    init_semantics();

    printf("=== BDGB: LENGUAJE NATURAL ===\n\n");

    run_query("belleza + simetrico", "belleza simetrico");
    run_query("resistencia + denso", "resistencia densa");
    printf("Presiona Enter...\n"); getchar();

    run_query("belleza + estable (atractor)", "belleza estable");
    run_query("simetrico + cerca atractor 9", "simetrico cerca9");
    printf("Presiona Enter...\n"); getchar();

    run_query("interiores simetricos", "interior simetrico");
    run_query("esquinas densas", "esquina denso");
    run_query("borde cerca atractor 7", "borde cerca7");
    printf("Presiona Enter...\n"); getchar();

    run_query("volumen + interior", "volumen interior");
    run_query("simetria + esquina", "simetria esquina");
    run_query("todos los atractores", "estable");

    save_usage();

    printf("=== BDGB con lenguaje natural lista ===\n");
    return 0;
}

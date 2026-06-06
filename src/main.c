#include "bdgb.h"
#include "semantics.h"
#include "concept_graph.h"
#include "search.h"
#include "learning.h"
#include "nlp.h"
#include "agent.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

static char data_path[512] = {0};

static void resolve_data_path(void) {
    const char *env = getenv("BDGB_DATA");
    if (env) {
        strncpy(data_path, env, sizeof(data_path) - 1);
        return;
    }
    const char *root = getenv("BDGB_ROOT");
    if (!root) root = ".";
    snprintf(data_path, sizeof(data_path), "%s/data", root);
}

static void clear_data(void) {
    char path[512];
    const char *files[] = {"nodes.dat", "edges_geom.dat", "edges_dyn.dat",
                           "semantics.dat", "concepts.idx",
                           "concept_edges.dat", "concept_edges.idx",
                           "usage_nodes.dat", "usage_concepts.dat",
                           "usage_edges.dat", NULL};
    for (int i = 0; files[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", data_path, files[i]);
        remove(path);
    }
}

static void init_system(void) {
    for (uint16_t id = 0; id < BDGB_GRID_NODES; id++) {
        bdgb_node_t node = bdgb_create_node((uint8_t)id);
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

/* --- CLI commands for Python bridge / agent execution --- */

static int cmd_search(int argc, char *argv[]) {
    const char *query = argv[2];
    SearchResult results[MAX_RESULTS];
    int n = nlp_search(query, results, MAX_RESULTS);
    reinforce_results(results, n);
    qsort(results, n, sizeof(SearchResult), cmp_score);

    printf("{\"query\":\"%s\",\"count\":%d,\"results\":[", query, n);
    for (int i = 0; i < n; i++) {
        bdgb_props_t p = bdgb_compute_props(results[i].node_id);
        if (i > 0) printf(",");
        printf("{\"node_id\":%u,\"score\":%u,\"concept_id\":%u,"
               "\"densidad\":%u,\"simetria\":%u,\"tipo_geom\":%u,"
               "\"clase_dinamica\":%u,\"pasos_atractor\":%d,\"atractor_id\":%u}",
               results[i].node_id, results[i].score, results[i].concept_id,
               p.densidad, p.simetria, p.tipo_geom,
               p.clase_dinamica, p.pasos_atractor, p.atractor_id);
    }
    printf("]}\n");
    save_usage();
    return 0;
}

static int cmd_export_nodes(void) {
    printf("{\"nodes\":[");
    for (uint16_t id = 0; id < BDGB_GRID_NODES; id++) {
        bdgb_node_t node;
        if (bdgb_load_node((uint8_t)id, &node) != NODE_OK) continue;
        bdgb_props_t p = bdgb_compute_props((uint8_t)id);
        if (id > 0) printf(",");
        printf("{\"id\":%u,\"x\":%u,\"y\":%u,"
               "\"bits\":%u,\"densidad\":%u,\"simetria\":%u,"
               "\"tipo_geom\":%u,\"radio\":%u,"
               "\"clase_dinamica\":%u,\"pasos_atractor\":%d,\"atractor_id\":%u}",
               id, node.x, node.y, node.id_bits,
               p.densidad, p.simetria, p.tipo_geom, p.radio,
               p.clase_dinamica, p.pasos_atractor, p.atractor_id);
    }
    printf("]}\n");
    return 0;
}

static int cmd_add_concept(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Uso: --add-concept <node_id> <concept_id> <weight> <rel_type>\n");
        return 1;
    }
    uint8_t node_id = (uint8_t)atoi(argv[2]);
    uint16_t concept_id = (uint16_t)atoi(argv[3]);
    uint8_t weight = (uint8_t)atoi(argv[4]);
    uint8_t rel_type = (uint8_t)atoi(argv[5]);
    int r = add_concept(node_id, concept_id, weight, rel_type);
    printf("{\"status\":\"%s\"}\n", r == 0 ? "ok" : "error");
    return 0;
}

static int cmd_agent_run(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: --agent-run <agent_id>\n");
        return 1;
    }
    agent_init();
    return agent_run(argv[2]);
}

static int cmd_interactive(void) {
    printf("=== BDGB v%d ===\n", BDGB_VERSION);
    printf("Data path: %s\n", data_path);
    printf("Grid: %d bits, %d x %d (%d nodes)\n",
           BDGB_GRID_BITS, BDGB_GRID_SIZE, BDGB_GRID_SIZE, BDGB_GRID_NODES);
    printf("\n");

    clear_data();
    bdgb_init(data_path);
    semantic_init(data_path);
    concept_graph_init(data_path);
    learning_init(data_path);
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

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    resolve_data_path();
    if (argc > 2 && strcmp(argv[1], "--data-path") == 0) {
        strncpy(data_path, argv[2], sizeof(data_path) - 1);
        argv += 2;
        argc -= 2;
    }

    bdgb_init(data_path);
    semantic_init(data_path);
    concept_graph_init(data_path);
    learning_init(data_path);
    nlp_init();

    if (argc < 2) return cmd_interactive();

    if (strcmp(argv[1], "--search") == 0 && argc >= 3) return cmd_search(argc, argv);
    if (strcmp(argv[1], "--export-nodes") == 0) return cmd_export_nodes();
    if (strcmp(argv[1], "--add-concept") == 0) return cmd_add_concept(argc, argv);
    if (strcmp(argv[1], "--agent-run") == 0) return cmd_agent_run(argc, argv);
    if (strcmp(argv[1], "--init") == 0) { clear_data(); init_system(); init_semantics(); printf("{\"status\":\"ok\"}\n"); return 0; }
    if (strcmp(argv[1], "--supervisor-tick") == 0) { agent_init(); agent_supervisor_tick(); return 0; }

    fprintf(stderr, "Uso: bdgb [--data-path <path>] <comando>\n");
    fprintf(stderr, "  (sin args)    modo interactivo\n");
    fprintf(stderr, "  --search <q>  busqueda NLP, salida JSON\n");
    fprintf(stderr, "  --export-nodes  dump JSON de todos los nodos\n");
    fprintf(stderr, "  --add-concept <n> <c> <w> <r>\n");
    fprintf(stderr, "  --agent-run <id>  ejecuta pipeline de agente\n");
    fprintf(stderr, "  --init         limpia e inicializa datos\n");
    return 1;
}

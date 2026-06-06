#include "bdgb.h"
#include "bdgb_crypt.h"
#include "semantics.h"
#include "concept_graph.h"
#include "search.h"
#include "learning.h"
#include "nlp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int tests_pass = 0, tests_fail = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_pass++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_fail++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static char TEST_PATH[512] = "tests/test_data";

static void resolve_test_path(void) {
#ifdef BDGB_SOURCE_DIR
    snprintf(TEST_PATH, sizeof(TEST_PATH), BDGB_SOURCE_DIR "/tests/test_data");
#else
    const char *root = getenv("BDGB_ROOT");
    if (root) snprintf(TEST_PATH, sizeof(TEST_PATH), "%s/tests/test_data", root);
#endif
}

static void cleanup(void) {
    const char *files[] = {"nodes.dat", "edges_geom.dat", "edges_dyn.dat",
                           "semantics.dat", "concepts.idx",
                           "concept_edges.dat", "concept_edges.idx",
                           "usage_nodes.dat", "usage_concepts.dat",
                           "usage_edges.dat", NULL};
    char path[512];
    for (int i = 0; files[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", TEST_PATH, files[i]);
        remove(path);
    }
}

static void test_grid_size(void) {
    TEST("grid size and constants");
    ASSERT(BDGB_GRID_BITS == 8, "expected 8 bits");
    ASSERT(BDGB_GRID_SIZE == 16, "expected grid size 16");
    ASSERT(BDGB_GRID_NODES == 256, "expected 256 nodes");
    ASSERT(sizeof(bdgb_node_t) == 4, "expected node struct 4 bytes");
    PASS();
}

static void test_node_creation(void) {
    TEST("create all 256 nodes");
    for (uint16_t i = 0; i < 256; i++) {
        bdgb_node_t n = bdgb_create_node((uint8_t)i);
        ASSERT(n.id_bits == (uint8_t)i, "id mismatch");
        ASSERT(n.x == (i & 0x0F), "x coordinate mismatch");
        ASSERT(n.y == (i >> 4), "y coordinate mismatch");
    }
    PASS();
}

static void test_popcount(void) {
    TEST("popcount");
    ASSERT(bdgb_count_ones(0) == 0, "0 has 0 ones");
    ASSERT(bdgb_count_ones(1) == 1, "1 has 1 one");
    ASSERT(bdgb_count_ones(3) == 2, "3 has 2 ones");
    ASSERT(bdgb_count_ones(0xFF) == 8, "255 has 8 ones");
    ASSERT(bdgb_count_ones(0x0F) == 4, "15 has 4 ones");
    PASS();
}

static void test_symmetry(void) {
    TEST("symmetry detection");
    ASSERT(bdgb_is_symmetric(0) == 1, "0 is symmetric");
    ASSERT(bdgb_is_symmetric(0xFF) == 1, "255 is symmetric");
    ASSERT(bdgb_is_symmetric(0x81) == 1, "129 is symmetric");
    ASSERT(bdgb_is_symmetric(0x1B) == 0, "27 is not symmetric");
    PASS();
}

static void test_geom_types(void) {
    TEST("geometric types for 16x16 grid");
    ASSERT(bdgb_geom_type(0) == GEOM_CORNER, "node 0 is corner");
    ASSERT(bdgb_geom_type(15) == GEOM_CORNER, "node 15 is corner");
    ASSERT(bdgb_geom_type(240) == GEOM_CORNER, "node 240 is corner");
    ASSERT(bdgb_geom_type(255) == GEOM_CORNER, "node 255 is corner");
    ASSERT(bdgb_geom_type(1) == GEOM_EDGE, "node 1 is edge");
    ASSERT(bdgb_geom_type(16) == GEOM_EDGE, "node 16 is edge");
    ASSERT(bdgb_geom_type(17) == GEOM_INTERIOR, "node 17 is interior");
    PASS();
}

static void test_dynamic_rule(void) {
    TEST("Kaprekar binary dynamic rule");
    uint8_t r0 = bdgb_dynamic_rule(0);
    ASSERT(r0 == 0, "0 is fixed point");
    uint8_t r255 = bdgb_dynamic_rule(255);
    ASSERT(r255 == 0, "255 maps to 0 (all ones - all zeros = 0)");
    uint8_t r1 = bdgb_dynamic_rule(1);
    int has_next = (r1 != 1);
    ASSERT(has_next, "1 is not fixed");
    PASS();
}

static void test_attractor_search(void) {
    TEST("attractor detection (all nodes converge)");
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t attr = bdgb_find_attractor((uint8_t)i);
        uint8_t next = bdgb_dynamic_rule(attr);
        ASSERT(next == attr, "attractor must be fixed point");
        int steps = bdgb_steps_to_attractor((uint8_t)i);
        ASSERT(steps >= 0, "attractor must be reachable");
    }
    PASS();
}

static void test_storage_roundtrip(void) {
    TEST("file storage roundtrip");
    bdgb_init(TEST_PATH);

    for (uint16_t i = 0; i < 256; i++) {
        bdgb_node_t n = bdgb_create_node((uint8_t)i);
        bdgb_error_t err = bdgb_store_node(&n);
        ASSERT(err == NODE_OK, "store should succeed");
    }

    bdgb_node_t loaded;
    for (uint16_t i = 0; i < 256; i++) {
        bdgb_error_t err = bdgb_load_node((uint8_t)i, &loaded);
        ASSERT(err == NODE_OK, "load should succeed");
        ASSERT(loaded.id_bits == (uint8_t)i, "loaded id matches");
    }
    PASS();
}

static void test_semantics_hash(void) {
    TEST("semantics with hash index");
    semantic_init(TEST_PATH);

    int r = add_concept(10, 5001, 200, REL_DEFINICION);
    ASSERT(r == 0, "add concept should succeed");
    r = add_concept(20, 5001, 150, REL_EJEMPLO);
    ASSERT(r == 0, "add second concept");

    SemanticLink links[16];
    int n = find_nodes_by_concept(5001, links, 16);
    ASSERT(n == 2, "should find 2 nodes");
    ASSERT(links[0].concept_id == 5001, "concept id matches");
    ASSERT(links[0].weight == 200 || links[1].weight == 200, "weight preserved");
    PASS();
}

static void test_concept_graph_hash(void) {
    TEST("concept graph with hash index");
    concept_graph_init(TEST_PATH);

    int r = add_concept_edge(5001, 6002, 255, REL_CAUSA);
    ASSERT(r == 0, "add edge should succeed");

    ConceptEdge edges[16];
    int n = get_related_concepts(5001, edges, 16);
    ASSERT(n >= 1, "should find related concepts");
    ASSERT(edges[0].to_concept == 6002 || edges[0].from_concept == 5001, "edge matches");

    n = get_related_concepts(6002, edges, 16);
    ASSERT(n >= 1, "reverse lookup works via hash");
    PASS();
}

static void test_learning_reinforce(void) {
    TEST("learning reinforce and decay");
    learning_init(TEST_PATH);

    uint32_t u0 = get_node_usage(5);
    ASSERT(u0 == 0, "usage starts at 0");

    reinforce_node(5);
    uint32_t u1 = get_node_usage(5);
    ASSERT(u1 == 10, "usage increases by 10");

    decay_all(0.5f);
    uint32_t u2 = get_node_usage(5);
    ASSERT(u2 == 5, "decay halves usage");

    PASS();
}

static void test_nlp_parse(void) {
    TEST("NLP parsing");
    nlp_init();

    NLPQueryElement elems[16];
    int n = nlp_parse("belleza simetrico", elems, 16);
    ASSERT(n >= 2, "should parse at least 2 terms");

    int has_concept = 0, has_pred = 0;
    for (int i = 0; i < n; i++) {
        if (elems[i].concept_id == 1001) has_concept = 1;
        if (elems[i].predicate != NULL) has_pred = 1;
    }
    ASSERT(has_concept, "belleza maps to concept 1001");
    ASSERT(has_pred, "simetrico has predicate");
    PASS();
}

static void test_search_hybrid(void) {
    TEST("hybrid search with learning integration");
    semantic_init(TEST_PATH);
    learning_init(TEST_PATH);

    add_concept(7, 2002, 220, REL_CAUSA);

    SearchResult results[16];
    int n = search_hybrid(2002, NULL, 0, 0, results, 16);
    ASSERT(n > 0, "hybrid should find results");

    reinforce_results(results, n);
    uint32_t usage = get_node_usage(results[0].node_id);
    ASSERT(usage >= 10, "results get reinforced");
    PASS();
}

static int props_test_pred(const bdgb_props_t *p) {
    return p->simetria && p->tipo_geom == GEOM_INTERIOR;
}

static void test_search_by_predicate(void) {
    TEST("search by predicate");
    int count = 0;
    for (int id = 0; id < BDGB_GRID_NODES; id++) {
        bdgb_props_t p = bdgb_compute_props((uint8_t)id);
        if (props_test_pred(&p)) count++;
    }
    ASSERT(count > 0, "at least one symmetric interior node");
    PASS();
}

static void test_compute_props(void) {
    TEST("compute properties for all nodes");
    for (uint16_t i = 0; i < 256; i++) {
        bdgb_props_t p = bdgb_compute_props((uint8_t)i);
        ASSERT(p.densidad <= 8, "density max 8 for 8-bit");
        ASSERT(p.tipo_geom <= 2, "geom type in range");
        ASSERT(p.clase_dinamica <= 2, "dyn class in range");
        ASSERT(p.pasos_atractor >= 0, "steps >= 0");
    }
    PASS();
}

static void test_crypt_init(void) {
    TEST("crypt init from password");
    BDGBCryptCtx ctx;
    uint8_t iv[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    int r = bdgb_crypt_init(&ctx, "test-password", iv);
    ASSERT(r == 0, "init should succeed");
    ASSERT(ctx.seed != 0, "seed should be non-zero");
    PASS();
}

static void test_crypt_keystream_differs(void) {
    TEST("crypt keystream different with different passwords/ivs");
    uint8_t iv1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t iv2[8] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01};
    BDGBCryptCtx ctx1, ctx2;
    bdgb_crypt_init(&ctx1, "alpha", iv1);
    bdgb_crypt_init(&ctx2, "omega", iv2);
    uint8_t ks1[32], ks2[32];
    bdgb_crypt_keystream(&ctx1, ks1, 32);
    bdgb_crypt_keystream(&ctx2, ks2, 32);
    ASSERT(memcmp(ks1, ks2, 32) != 0, "keystreams with different keys should differ");
    PASS();
}

static void test_crypt_encrypt_decrypt(void) {
    TEST("crypt round-trip encrypt/decrypt");
    uint8_t plain[] = "HELLO BDGB CRYPT! THIS IS A TEST MESSAGE 12345";
    size_t len = strlen((char*)plain) + 1;
    uint8_t *buf = (uint8_t*)malloc(len);
    memcpy(buf, plain, len);

    BDGBCryptCtx ctx;
    uint8_t iv[8] = {0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88};
    bdgb_crypt_init(&ctx, "test-key", iv);
    bdgb_crypt_buffer(&ctx, buf, len);

    ASSERT(memcmp(buf, plain, len) != 0, "ciphertext differs from plaintext");

    BDGBCryptCtx ctx2;
    bdgb_crypt_init(&ctx2, "test-key", iv);
    bdgb_crypt_buffer(&ctx2, buf, len);

    ASSERT(memcmp(buf, plain, len) == 0, "decrypted matches original");
    free(buf);
    PASS();
}

static void test_crypt_file_roundtrip(void) {
    TEST("crypt file round-trip");
    const char *orig = "bdgb_crypt_test_orig.txt";
    const char *enc  = "bdgb_crypt_test_enc.bdgb";
    const char *dec  = "bdgb_crypt_test_dec.txt";

    FILE *f = fopen(orig, "wb");
    ASSERT(f != NULL, "create orig file");
    fprintf(f, "BDGB CRYPT FILE TEST - masa madre encryptada!");
    fclose(f);

    int r = bdgb_encrypt_file(orig, enc, "clave-secreta");
    ASSERT(r == 0, "encrypt should succeed");

    r = bdgb_decrypt_file(enc, dec, "clave-secreta");
    ASSERT(r == 0, "decrypt should succeed");

    FILE *fa = fopen(orig, "rb");
    FILE *fb = fopen(dec, "rb");
    ASSERT(fa && fb, "both files exist");
    int eq = 1;
    int ca, cb;
    do {
        ca = fgetc(fa); cb = fgetc(fb);
        if (ca != cb) { eq = 0; break; }
    } while (ca != EOF);
    fclose(fa); fclose(fb);
    ASSERT(eq, "decrypted file matches original");

    remove(orig); remove(enc); remove(dec);
    PASS();
}

static void run_all_tests(void) {
    printf("=== BDGB TESTS ===\n\n");

    test_grid_size();
    test_node_creation();
    test_popcount();
    test_symmetry();
    test_geom_types();
    test_dynamic_rule();
    test_attractor_search();
    test_storage_roundtrip();
    test_semantics_hash();
    test_concept_graph_hash();
    test_learning_reinforce();
    test_nlp_parse();
    test_search_hybrid();
    test_search_by_predicate();
    test_compute_props();
    test_crypt_init();
    test_crypt_keystream_differs();
    test_crypt_encrypt_decrypt();
    test_crypt_file_roundtrip();

    printf("\n=== RESULTADOS: %d/%d PASS, %d FAIL ===\n",
           tests_pass, tests_pass + tests_fail, tests_fail);
}

int main(void) {
    resolve_test_path();
    cleanup();
    bdgb_init(TEST_PATH);
    run_all_tests();
    cleanup();
    return tests_fail > 0 ? 1 : 0;
}

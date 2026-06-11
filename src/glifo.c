#include "glifo.h"
#include "bdgb.h"
#include "nlp.h"
#include "util.h"
#include "json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define TRENDS_RSS_URL "https://trends.google.com/trending/rss?geo=US"
#define MAX_PATH_LEN 512

/* ============================================================
 *   REGISTRY
 * ============================================================ */

static GlifoDef glifos[MAX_GLIFOS];
static int glifo_count = 0;

int glifo_register(const char *id, const char *nombre,
                   int (*run)(const char *args)) {
    if (glifo_count >= MAX_GLIFOS) return -1;
    GlifoDef *g = &glifos[glifo_count++];
    strncpy(g->id, id, sizeof(g->id) - 1);
    strncpy(g->nombre, nombre, sizeof(g->nombre) - 1);
    g->run = run;
    g->sistema[0] = 0;
    g->ejecuciones = 0;
    g->exitosas = 0;
    g->fallidas = 0;
    return 0;
}

/* ---- Helpers JSON para leer registry.json (legacy, sin json.c) ---- */

static int json_strval(const char *json, const char *key, char *out, size_t outsz) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '"') return -1;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < outsz - 1) out[i++] = *p++;
    out[i] = 0;
    return 0;
}

static int is_active_system(const char *block) {
    char estado[16] = {0};
    if (json_strval(block, "estado", estado, sizeof(estado)) != 0) return 0;
    return (strcmp(estado, "activo") == 0);
}

static int parse_glifo_array(const char *block, char out[][64], int max) {
    const char *p = strstr(block, "\"glifos\"");
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    int count = 0;
    while (*p && *p != ']' && count < max) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
        if (*p == '"') {
            p++;
            int i = 0;
            while (*p && *p != '"' && i < 63) out[count][i++] = *p++;
            out[count][i] = 0;
            if (i > 0) count++;
            if (*p) p++;
        } else break;
    }
    return count;
}

int glifo_load_systems(void) {
    const char *root = getenv("BDGB_ROOT");
    if (!root) root = ".";
    char path[512];
    snprintf(path, sizeof(path), "%s/glifos/registry.json", root);

    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc((size_t)sz + 1);
    if (!json) { fclose(f); return 0; }
    fread(json, 1, (size_t)sz, f);
    json[sz] = 0;
    fclose(f);

    int loaded = 0;
    char *p = json;
    while ((p = strstr(p, "\"panal\"")) && loaded < MAX_GLIFOS) {
        char *bs = p;
        while (bs > json && *bs != '{') bs--;
        if (*bs != '{') { p = p + 1; continue; }
        char *be = strchr(p, '}');
        if (!be) break;
        int blen = (int)(be - bs + 1);
        char block[4096];
        if (blen >= 4096) { p = be + 1; continue; }
        strncpy(block, bs, blen);
        block[blen] = 0;

        if (!is_active_system(block)) { p = be + 1; continue; }

        char sn[64] = {0};
        json_strval(block, "panal", sn, sizeof(sn));
        if (sn[0] == 0) { p = be + 1; continue; }

        char sg[16][64];
        int ng = parse_glifo_array(block, sg, 16);
        for (int gi = 0; gi < ng; gi++) {
            for (int ri = 0; ri < glifo_count; ri++) {
                if (strcmp(glifos[ri].id, sg[gi]) == 0) {
                    strncpy(glifos[ri].sistema, sn, sizeof(glifos[ri].sistema) - 1);
                    loaded++;
                    break;
                }
            }
        }
        p = be + 1;
    }
    free(json);
    return loaded;
}

int glifo_run(const char *id, const char *args) {
    for (int i = 0; i < glifo_count; i++) {
        if (strcmp(glifos[i].id, id) == 0) {
            if (glifos[i].sistema[0] == 0) {
                printf("[GLIFO] Error: '%s' no pertenece a ningun sistema activo\n", id);
                return GLIFO_ERR;
            }
            glifos[i].ejecuciones++;
            int r = glifos[i].run(args ? args : "");
            if (r == 0) glifos[i].exitosas++;
            else glifos[i].fallidas++;
            return r;
        }
    }
    printf("[GLIFO] Error: '%s' no encontrado\n", id);
    return GLIFO_ERR;
}

int glifo_list(GlifoDef *out, int max) {
    int n = 0;
    for (int i = 0; i < glifo_count && n < max; i++) {
        if (glifos[i].sistema[0]) out[n++] = glifos[i];
    }
    return n;
}

void glifo_mark_success(const char *id) {
    for (int i = 0; i < glifo_count; i++)
        if (strcmp(glifos[i].id, id) == 0) { glifos[i].exitosas++; break; }
}

void glifo_mark_fail(const char *id) {
    for (int i = 0; i < glifo_count; i++)
        if (strcmp(glifos[i].id, id) == 0) { glifos[i].fallidas++; break; }
}

/* ============================================================
 *   ORQUESTADOR GENERICO — lee semilla.json, ejecuta pipeline
 * ============================================================ */

static int run_command(const char *cmd) {
    printf("[PIPELINE] %s\n", cmd);
    char buf[4096] = {0};
    int r = run_captured(cmd, buf, sizeof(buf));
    if (buf[0]) printf("%s", buf);
    return r;
}

/* Busca un glifo por id en el array "glifos" de semilla.json.
   Devuelve el entry como string (no debe ser freed). */
static const char *find_entry(JsonValue *semilla, const char *glifo_id) {
    JsonValue *glifos_arr = json_get(semilla, "glifos");
    if (!glifos_arr) return NULL;
    int n = json_len(glifos_arr);
    for (int i = 0; i < n; i++) {
        JsonValue *g = json_idx(glifos_arr, i);
        if (!g) continue;
        JsonValue *id_v = json_get(g, "id");
        if (!id_v) continue;
        if (strcmp(json_string(id_v), glifo_id) != 0) continue;
        JsonValue *entry_v = json_get(g, "entry");
        if (!entry_v) continue;
        return json_string(entry_v);
    }
    return NULL;
}

/* Lee el root path desde BDGB_ROOT o pwd */
static const char *get_root(void) {
    const char *r = getenv("BDGB_ROOT");
    return r ? r : ".";
}

/* Construye la ruta completa: <root>/glifos/<panal>/semilla.json */
static void build_semilla_path(char *out, size_t outsz, const char *panal) {
    snprintf(out, outsz, "%s/glifos/%s/semilla.json", get_root(), panal);
}

/* Ejecuta un paso del pipeline.
   Returns: 0 = OK, -1 = ERROR (stop pipeline), -2 = SKIP (continue) */
static int ejecutar_paso(JsonValue *semilla, const char *glifo_id,
                         const char *args_extra, int warn_on_fail) {
    const char *entry = find_entry(semilla, glifo_id);
    if (!entry) {
        printf("[PIPELINE] ERROR: glifo '%s' no tiene entry en semilla.json\n", glifo_id);
        return warn_on_fail ? GLIFO_SALTAR : GLIFO_ERR;
    }

    char cmd[1024];
    if (args_extra && args_extra[0])
        snprintf(cmd, sizeof(cmd), "%s %s", entry, args_extra);
    else
        snprintf(cmd, sizeof(cmd), "%s", entry);

    int r = run_command(cmd);
    if (r != 0) {
        if (warn_on_fail) {
            printf("[PIPELINE] WARN: '%s' fallo (codigo %d), continuando\n", glifo_id, r);
            return GLIFO_SALTAR;
        }
        printf("[PIPELINE] ERROR: '%s' fallo (codigo %d), pipeline detenido\n", glifo_id, r);
        return GLIFO_ERR;
    }
    return GLIFO_OK;
}

/* Orquestador generico.
   Args: nombre del panal (e.g. "generacion-contenido", "vigilancia-tendencias").
   Lee semilla.json del panal y ejecuta pipeline segun config.pipeline
   (array simple) o pipeline.orden (array detallado). */
int glifo_pipeline_run(const char *args) {
    const char *panal = args;
    if (!panal || !panal[0]) panal = "generacion-contenido";

    printf("[PIPELINE] ===== Panal: %s =====\n", panal);

    char semilla_path[MAX_PATH_LEN];
    build_semilla_path(semilla_path, sizeof(semilla_path), panal);

    JsonValue *semilla = json_parse_file(semilla_path);
    if (!semilla) {
        printf("[PIPELINE] ERROR: no se pudo leer %s\n", semilla_path);
        return GLIFO_ERR;
    }

    /* Determinar orden del pipeline */
    int step_count = 0;
    char step_ids[32][64];
    char step_args[32][128];
    int step_warn[32];

    /* Priority 1: config.pipeline (array simple de strings) */
    JsonValue *cfg = json_get(semilla, "config");
    if (cfg) {
        JsonValue *arr = json_get(cfg, "pipeline");
        if (arr) {
            int n = json_len(arr);
            for (int i = 0; i < n && step_count < 32; i++) {
                JsonValue *id_v = json_idx(arr, i);
                if (!id_v || json_string(id_v) == NULL) continue;
                strncpy(step_ids[step_count], json_string(id_v), 63);
                step_ids[step_count][63] = 0;
                step_args[step_count][0] = 0;
                step_warn[step_count] = 0;
                step_count++;
            }
        }
    }

    /* Priority 2: pipeline.orden (array detallado de objetos) */
    if (step_count == 0) {
        JsonValue *orden = json_path(semilla, "pipeline.orden");
        if (orden) {
            int n = json_len(orden);
            for (int i = 0; i < n && step_count < 32; i++) {
                JsonValue *step = json_idx(orden, i);
                if (!step) continue;
                JsonValue *gv = json_get(step, "glifo");
                if (!gv || !json_string(gv)) continue;
                strncpy(step_ids[step_count], json_string(gv), 63);
                step_ids[step_count][63] = 0;

                /* args opcionales */
                JsonValue *av = json_get(step, "args");
                if (av) {
                    char buf[128] = {0};
                    int alen = json_len(av);
                    for (int a = 0; a < alen; a++) {
                        JsonValue *av_i = json_idx(av, a);
                        if (av_i && json_string(av_i)) {
                            if (buf[0]) strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
                            strncat(buf, json_string(av_i), sizeof(buf) - strlen(buf) - 1);
                        }
                    }
                    strncpy(step_args[step_count], buf, 127);
                    step_args[step_count][127] = 0;
                } else {
                    step_args[step_count][0] = 0;
                }

                /* warn_on_fail si glifo_id contiene "cleanup" o accion contiene "clean" */
                if (strstr(step_ids[step_count], "cleanup")) {
                    step_warn[step_count] = 1;
                } else {
                    JsonValue *ev = json_get(step, "accion");
                    step_warn[step_count] = (ev && json_string(ev) &&
                        (strstr(json_string(ev), "cleanup") ||
                         strcmp(json_string(ev), "clean") == 0)) ? 1 : 0;
                }

                step_count++;
            }
            /* ultimo paso siempre warn */
            if (step_count > 0) step_warn[step_count - 1] = 1;
        }
    }

    /* Priority 3: si no hay pipeline, ejecutar todos los glifos en orden declarado */
    if (step_count == 0) {
        JsonValue *glifos_arr = json_get(semilla, "glifos");
        int n = json_len(glifos_arr);
        for (int i = 0; i < n && step_count < 32; i++) {
            JsonValue *g = json_idx(glifos_arr, i);
            JsonValue *id_v = json_get(g, "id");
            if (!id_v || !json_string(id_v)) continue;
            strncpy(step_ids[step_count], json_string(id_v), 63);
            step_ids[step_count][63] = 0;
            step_args[step_count][0] = 0;
            step_warn[step_count] = (strstr(step_ids[step_count], "cleanup") != NULL) ? 1 : 0;
            step_count++;
        }
        if (step_count > 1) step_warn[step_count - 1] = 1;
    }

    if (step_count == 0) {
        printf("[PIPELINE] ERROR: no se encontraron pasos en semilla.json\n");
        json_free(semilla);
        return GLIFO_ERR;
    }

    /* Ejecutar pipeline */
    int total_ok = 0, total_warn = 0, total_err = 0;
    for (int i = 0; i < step_count; i++) {
        printf("[PIPELINE] Paso %d/%d: %s\n", i + 1, step_count, step_ids[i]);
        int r = ejecutar_paso(semilla, step_ids[i], step_args[i], step_warn[i]);
        if (r == GLIFO_OK) {
            total_ok++;
        } else if (r == GLIFO_SALTAR) {
            total_warn++;
        } else {
            total_err++;
            json_free(semilla);
            printf("[PIPELINE] ===== Pipeline fallido en paso %d (%s) =====\n",
                   i + 1, step_ids[i]);
            return GLIFO_ERR;
        }
    }

    json_free(semilla);
    printf("[PIPELINE] ===== Pipeline completado: %d OK, %d WARN, %d ERR =====\n",
           total_ok, total_warn, total_err);
    return total_err > 0 ? GLIFO_ERR : GLIFO_OK;
}

/* ============================================================
 *   GLIFO GENERACION-CONTENIDO — wrapper del orquestador
 * ============================================================ */

int glifo_generacion_contenido_run(const char *args) {
    (void)args;
    return glifo_pipeline_run("generacion-contenido");
}

/* ============================================================
 *   GLIFO VIGILANCIA — wrapper del orquestador
 * ============================================================ */

int glifo_vigilancia_run(const char *args) {
    (void)args;
    return glifo_pipeline_run("vigilancia-tendencias");
}

/* ============================================================
 *   GLIFO PRIMO — Trend Tracker nativo en C
 * ============================================================ */

#define MOCK_COUNT 10

static const char *mock_topics[MOCK_COUNT] = {
    "inteligencia artificial pixel art",
    "video juegos retro 2026",
    "musica lo fi para estudiar",
    "tutorial blender 3d",
    "cocina vegana rapida",
    "diseno de personajes pixel",
    "animacion 2d principiantes",
    "chip tunes musica",
    "game development sin codigo",
    "ia generativa arte"
};

static const char *mock_categories[MOCK_COUNT] = {
    "tecnologia", "entretenimiento", "musica", "educacion", "cocina",
    "arte", "educacion", "musica", "tecnologia", "tecnologia"
};

static int mock_scores[MOCK_COUNT] = {
    88, 79, 73, 67, 61, 82, 71, 64, 76, 91
};

typedef struct {
    char topic[64];
    int score;
    char category[32];
} TrendItem;

static int fetch_trends(TrendItem *out, int max) {
    if (max <= 0) return 0;
    char buf[8192] = {0};
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s --max-time 5 '" TRENDS_RSS_URL "' 2>/dev/null || echo FALLBACK");
    run_captured(cmd, buf, sizeof(buf));

    int count = 0;
    if (strstr(buf, "title") && !strstr(buf, "FALLBACK")) {
        char *p = buf;
        while ((p = strstr(p, "<title>")) && count < max) {
            p += 7;
            char *end = strstr(p, "</title>");
            if (!end) break;
            int len = (int)(end - p);
            if (len >= 63) len = 63;
            strncpy(out[count].topic, p, (size_t)len);
            out[count].topic[len] = 0;
            out[count].score = 50 + (rand() % 50);
            snprintf(out[count].category, 32, "general");
            count++;
            p = end + 1;
        }
        if (count > 0) return count;
    }

    for (int i = 0; i < MOCK_COUNT && count < max; i++) {
        strncpy(out[count].topic, mock_topics[i], 63);
        out[count].score = mock_scores[i] + (rand() % 10 - 5);
        snprintf(out[count].category, 32, "%s", mock_categories[i]);
        count++;
    }
    return count;
}

static void inject_into_bdgb(TrendItem *trends, int n) {
    for (int i = 0; i < n && i < 5; i++) {
        uint16_t cid = (uint16_t)(6000 + i);
        uint8_t node_id = (uint8_t)((i + 10) % BDGB_GRID_NODES);
        add_concept(node_id, cid, (uint8_t)(trends[i].score > 255 ? 255 : trends[i].score), 0);
    }
    char combined[1024] = {0};
    for (int i = 0; i < n && i < 5; i++) {
        if (combined[0]) strncat(combined, ". ", sizeof(combined) - strlen(combined) - 1);
        strncat(combined, trends[i].topic, sizeof(combined) - strlen(combined) - 1);
    }
    if (combined[0]) {
        int learned = nlp_learn_from_text(combined, 20);
        printf("[GLIFO-PRIMO] %d nuevos terminos NLP aprendidos\n", learned);
    }
}

static int save_report(TrendItem *trends, int n) {
    const char *root = getenv("BDGB_ROOT");
    if (!root) root = ".";
    char dir[512], weekly_dir[512];
    snprintf(dir, sizeof(dir), "%s/glifos/vigilancia-tendencias/glifos/trend-tracker/daily", root);
    snprintf(weekly_dir, sizeof(weekly_dir), "%s/glifos/vigilancia-tendencias/glifos/trend-tracker/weekly", root);
    ensure_dir(dir);
    ensure_dir(weekly_dir);

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date[16];
    strftime(date, sizeof(date), "%Y-%m-%d", tm);

    char path[512];
    snprintf(path, sizeof(path), "%s/%s.txt", dir, date);
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "=========================================\n");
    fprintf(f, "  BDGB GLIFO PRIMO — REPORTE DIARIO\n");
    fprintf(f, "  Fecha: %s\n", date);
    fprintf(f, "=========================================\n\n");
    fprintf(f, "Tendencias: %d\n\n", n);
    for (int i = 0; i < n; i++)
        fprintf(f, "  %2d. %-40s %3d  [%s]\n", i + 1,
                trends[i].topic, trends[i].score, trends[i].category);
    fclose(f);
    printf("[GLIFO-PRIMO] Reporte: %s\n", path);
    return 0;
}

int glifo_primo_run(const char *args) {
    (void)args;
    printf("[GLIFO-PRIMO] Glifo Primo — Trend Tracker\n");

    TrendItem trends[32];
    int n = fetch_trends(trends, 32);
    if (n <= 0) return -1;

    printf("[GLIFO-PRIMO] %d tendencias:\n", n);
    for (int i = 0; i < n && i < 5; i++)
        printf("  %2d. %-40s %3d  [%s]\n", i + 1,
               trends[i].topic, trends[i].score, trends[i].category);

    save_report(trends, n);
    inject_into_bdgb(trends, n);
    return 0;
}

/* ============================================================
 *   INIT
 * ============================================================ */

int glifo_init(void) {
    glifo_count = 0;
    glifo_register("primo", "Glifo Primo - Trend Tracker", glifo_primo_run);
    glifo_register("generacion-contenido", "Orquestador Generacion de Contenido",
                   glifo_generacion_contenido_run);
    glifo_register("vigilancia-tendencias", "Orquestador Vigilancia de Tendencias",
                   glifo_vigilancia_run);
    glifo_load_systems();
    return glifo_count;
}
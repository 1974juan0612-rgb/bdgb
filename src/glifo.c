#include "glifo.h"
#include "bdgb.h"
#include "nlp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

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
    g->ejecuciones = 0;
    g->exitosas = 0;
    g->fallidas = 0;
    return 0;
}

int glifo_run(const char *id, const char *args) {
    for (int i = 0; i < glifo_count; i++) {
        if (strcmp(glifos[i].id, id) == 0) {
            glifos[i].ejecuciones++;
            int r = glifos[i].run(args ? args : "");
            if (r == 0) glifos[i].exitosas++;
            else glifos[i].fallidas++;
            return r;
        }
    }
    printf("[GLIFO] Error: '%s' no encontrado\n", id);
    return -1;
}

int glifo_list(GlifoDef *out, int max) {
    int n = (glifo_count < max) ? glifo_count : max;
    for (int i = 0; i < n; i++) out[i] = glifos[i];
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

/* Ejecuta comando y captura stdout */
static int run_captured(const char *cmd, char *out, size_t outsz) {
#ifdef _WIN32
    FILE *fp = _popen(cmd, "r");
#else
    FILE *fp = popen(cmd, "r");
#endif
    if (!fp) return -1;
    size_t pos = 0;
    while (pos < outsz - 1) {
        int c = fgetc(fp);
        if (c == EOF) break;
        out[pos++] = (char)c;
    }
    out[pos] = 0;
#ifdef _WIN32
    return _pclose(fp);
#else
    return pclose(fp);
#endif
}

static void ensure_dir(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

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
        "curl -s --max-time 5 'https://trends.google.com/trending/rss?geo=US' 2>/dev/null || echo FALLBACK");
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
    snprintf(dir, sizeof(dir), "%s/glifos/trend-tracker/daily", root);
    snprintf(weekly_dir, sizeof(weekly_dir), "%s/glifos/trend-tracker/weekly", root);
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
    return glifo_count;
}

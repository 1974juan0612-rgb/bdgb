#include "agent.h"
#include "bdgb.h"
#include "search.h"
#include "nlp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static char registry_path[512] = {0};
static char agents_dir[512] = {0};

static AgentDef agents[MAX_AGENTS];
static int agent_count = 0;
static int initialized = 0;

int agent_init(void) {
    const char *root = getenv("BDGB_ROOT");
    if (!root) root = ".";
    snprintf(registry_path, sizeof(registry_path), "%s/agents/registry.json", root);
    snprintf(agents_dir, sizeof(agents_dir), "%s/agents", root);
    agent_count = 0;
    memset(agents, 0, sizeof(agents));
    initialized = 1;
    return agent_load_all();
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) s[--len] = 0;
}

/* --- JSON helpers --- */

static int json_get_string(const char *json, const char *key, char *out, size_t outsz) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '"') {
        p++;
        int i = 0;
        while (*p && *p != '"' && i < (int)outsz - 1) out[i++] = *p++;
        out[i] = 0;
        return 0;
    }
    return -1;
}

static int json_get_int(const char *json, const char *key) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return atoi(p);
}

static void json_escape(const char *in, char *out, size_t outsz) {
    int i = 0;
    while (*in && i < (int)outsz - 2) {
        if (*in == '"' || *in == '\\') { if (i < (int)outsz - 3) out[i++] = '\\'; }
        out[i++] = *in++;
    }
    out[i] = 0;
}

/* --- Registry I/O --- */

int agent_load_all(void) {
    FILE *f = fopen(registry_path, "rb");
    if (!f) { agent_count = 0; return 0; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc(size + 1);
    if (!json) { fclose(f); return -1; }
    size_t read = fread(json, 1, size, f);
    (void)read;
    json[size] = 0;
    fclose(f);

    agent_count = 0;
    char *p = json;
    while ((p = strstr(p, "{")) && agent_count < MAX_AGENTS) {
        char *end = strstr(p, "}");
        if (!end) break;
        int len = (int)(end - p + 1);
        char block[2048];
        if (len >= 2048) { p = end + 1; continue; }
        strncpy(block, p, len);
        block[len] = 0;
        if (!strstr(block, "\"id\"")) { p = end + 1; continue; }

        AgentDef *a = &agents[agent_count];
        memset(a, 0, sizeof(AgentDef));
        json_get_string(block, "id", a->id, sizeof(a->id));
        json_get_string(block, "nombre", a->nombre, sizeof(a->nombre));
        json_get_string(block, "estado", a->estado, sizeof(a->estado));
        a->activo = (strcmp(a->estado, "activo") == 0) ? 1 : 0;
        a->ejecuciones = json_get_int(block, "ejecuciones");
        a->exitosas = json_get_int(block, "exitosas");
        a->fallidas = json_get_int(block, "fallidas");
        agent_count++;
        p = end + 1;
    }
    free(json);
    return agent_count;
}

int agent_get(const char *id, AgentDef *out) {
    for (int i = 0; i < agent_count; i++) {
        if (strcmp(agents[i].id, id) == 0) { *out = agents[i]; return 0; }
    }
    return -1;
}

int agent_list(AgentDef *out, int max_out) {
    int n = (agent_count < max_out) ? agent_count : max_out;
    for (int i = 0; i < n; i++) out[i] = agents[i];
    return n;
}

int agent_register(const char *id, const char *nombre, const char *schedule) {
    FILE *f = fopen(registry_path, "rb");
    if (f) fclose(f);
    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s/%s/config.json", agents_dir, id);
    f = fopen(dir, "rb");
    if (f) { fclose(f); return -1; }
    return 0;
}

/* ============================================================
 *   TOOL SYSTEM — real external tool execution
 * ============================================================ */

/* Run external command, capture stdout into buffer. Returns 0 on success. */
static int run_captured(const char *cmd, char *out, size_t outsz) {
    FILE *fp;
#ifdef _WIN32
    fp = _popen(cmd, "r");
#else
    fp = popen(cmd, "r");
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

/* Tool: web-fetch — uses curl or wget */
static int tool_web_fetch(const char *args, char *out, size_t outsz) {
    char url[256];
    if (strncmp(args, "url=", 4) == 0)
        strncpy(url, args + 4, sizeof(url) - 1);
    else
        strncpy(url, args, sizeof(url) - 1);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "curl -s --max-time 10 \"%s\" 2>/dev/null || "
        "wget -q -O - --timeout=10 \"%s\" 2>/dev/null || "
        "echo '{\"error\":\"no-http-client\"}'", url, url);
    return run_captured(cmd, out, outsz);
}

/* Tool: bdgb-search — search via NLP engine */
static int tool_bdgb_search(const char *args, char *out, size_t outsz) {
    (void)outsz;
    char query[128];
    if (strncmp(args, "query=", 6) == 0)
        strncpy(query, args + 6, sizeof(query) - 1);
    else
        strncpy(query, args, sizeof(query) - 1);

    SearchResult results[MAX_RESULTS];
    int n = nlp_search(query, results, MAX_RESULTS);

    char buf[4096];
    int pos = snprintf(buf, sizeof(buf), "{\"tool\":\"search\",\"query\":");
    char eq[256]; json_escape(query, eq, sizeof(eq));
    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "\"%s\",\"count\":%d,\"results\":[", eq, n);

    for (int i = 0; i < n && i < 10; i++) {
        bdgb_props_t p = bdgb_compute_props(results[i].node_id);
        if (i > 0) buf[pos++] = ',';
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
            "{\"node_id\":%u,\"score\":%u,\"concept_id\":%u,"
            "\"densidad\":%u,\"simetria\":%u,\"tipo_geom\":%u,"
            "\"pasos_atractor\":%d}",
            results[i].node_id, results[i].score, results[i].concept_id,
            p.densidad, p.simetria, p.tipo_geom, p.pasos_atractor);
    }
    snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]}");
    strncpy(out, buf, outsz - 1);
    out[outsz - 1] = 0;
    return 0;
}

/* Tool: scraper-trends — run the Python scraper */
static int tool_scraper_trends(const char *args, char *out, size_t outsz) {
    const char *root = getenv("BDGB_ROOT");
    if (!root) root = ".";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "cd \"%s\" && \"%s/venv/Scripts/python\" \"%s/scripts/scraper_trends.py\" %s 2>&1",
        root, root, root, args ? args : "");
    return run_captured(cmd, out, outsz);
}

/* Tool: file-read */
static int tool_file_read(const char *args, char *out, size_t outsz) {
    char path[256];
    if (strncmp(args, "path=", 5) == 0)
        strncpy(path, args + 5, sizeof(path) - 1);
    else
        strncpy(path, args, sizeof(path) - 1);

    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(out, outsz, "{\"error\":\"cannot read %s\"}", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz > (long)outsz - 128) sz = (long)outsz - 128;
    size_t n = fread(out, 1, (size_t)sz, f);
    out[n] = 0;
    fclose(f);
    return 0;
}

/* Tool: system — controlled external command */
static int tool_system(const char *args, char *out, size_t outsz) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s 2>&1", args);
    return run_captured(cmd, out, outsz);
}

/* Tool: learn-from-text — extract and learn new NLP terms from raw text */
static int tool_learn_text(const char *args, char *out, size_t outsz) {
    if (!args || args[0] == 0) {
        snprintf(out, outsz, "{\"error\":\"no text provided\"}");
        return -1;
    }
    int learned = nlp_learn_from_text(args, 50);
    int total = nlp_term_count();
    snprintf(out, outsz, "{\"learned\":%d,\"total_terms\":%d}", learned, total);
    return 0;
}

typedef struct {
    const char *name;
    int (*handler)(const char *args, char *out, size_t outsz);
} ToolDef;

static ToolDef tools[] = {
    {"web-fetch",       tool_web_fetch},
    {"bdgb-search",     tool_bdgb_search},
    {"scraper-trends",  tool_scraper_trends},
    {"file-read",       tool_file_read},
    {"learn-from-text", tool_learn_text},
    {"system",          tool_system},
    {NULL, NULL}
};

static int dispatch_tool(const char *tool_name, const char *args,
                         char *out, size_t outsz) {
    for (int i = 0; tools[i].name; i++) {
        if (strcmp(tools[i].name, tool_name) == 0)
            return tools[i].handler(args ? args : "", out, outsz);
    }
    snprintf(out, outsz, "{\"error\":\"unknown tool: %s\"}", tool_name);
    return -1;
}

/* ============================================================
 *   STEP EXECUTION — read config.json, dispatch each step
 * ============================================================ */

static int execute_step(const char *agent_id, const char *step_name) {
    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s/config.json", agents_dir, agent_id);

    FILE *f = fopen(cfg_path, "rb");
    if (!f) {
        printf("[AGENT] Error: no se pudo leer %s\n", cfg_path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc((size_t)size + 1);
    if (!json) { fclose(f); return -1; }
    fread(json, 1, (size_t)size, f);
    json[size] = 0;
    fclose(f);

    char tool_name[64] = {0};
    char tool_args[256] = {0};
    {
        char search_key[128];
        snprintf(search_key, sizeof(search_key), "\"%s\"", step_name);
        char *step_block = strstr(json, search_key);
        if (step_block) {
            char *tp = strstr(step_block, "\"tool\"");
            if (tp) {
                tp = strchr(tp, ':');
                if (tp) {
                    tp++;
                    while (*tp == ' ' || *tp == '\t' || *tp == '\n' || *tp == '\r') tp++;
                    if (*tp == '"') {
                        tp++;
                        int ti = 0;
                        while (*tp && *tp != '"' && ti < 63) tool_name[ti++] = *tp++;
                        tool_name[ti] = 0;
                    }
                }
            }
            char *ap = strstr(step_block, "\"args\"");
            if (ap) {
                ap = strchr(ap, ':');
                if (ap) {
                    ap++;
                    while (*ap == ' ' || *ap == '\t' || *ap == '\n' || *ap == '\r') ap++;
                    if (*ap == '"') {
                        ap++;
                        int ai = 0;
                        while (*ap && *ap != '"' && ai < 255) tool_args[ai++] = *ap++;
                        tool_args[ai] = 0;
                    }
                }
            }
        }
    }

    if (tool_name[0] == 0) {
        printf("[AGENT] Step '%s': no tool; skipping\n", step_name);
        free(json);
        return 0;
    }

    printf("[AGENT] Step '%s' -> tool: %s", step_name, tool_name);
    if (tool_args[0]) printf(", args: %s", tool_args);
    printf("\n");
    fflush(stdout);

    char result[4096];
    int ret = dispatch_tool(tool_name, tool_args, result, sizeof(result));

    printf("[AGENT]   result (%d): %s\n", ret, result);
    fflush(stdout);
    free(json);
    return ret;
}

/* ============================================================
 *   PUBLIC API
 * ============================================================ */

int agent_run(const char *id) {
    if (!initialized) return -1;
    printf("[AGENT] Ejecutando agente: %s\n", id);
    return agent_run_pipeline(id, 0);
}

int agent_run_pipeline(const char *id, int step_index) {
    AgentDef a;
    if (agent_get(id, &a) != 0) {
        printf("[AGENT] Error: agente '%s' no encontrado\n", id);
        return -1;
    }

    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s/config.json", agents_dir, id);

    FILE *f = fopen(cfg_path, "rb");
    if (!f) {
        for (int i = step_index; i < a.pipeline_len; i++)
            printf("[AGENT]   -> %s\n", a.pipeline[i]);
        printf("[AGENT] Pipeline completado (simulado - sin config)\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc((size_t)size + 1);
    if (!json) { fclose(f); return -1; }
    fread(json, 1, (size_t)size, f);
    json[size] = 0;
    fclose(f);

    int step_count = 0;
    char step_names[MAX_PIPELINE][64];
    char *pb = strstr(json, "\"pipeline\"");
    if (pb) {
        char *brace = strchr(pb, '{');
        if (brace) {
            char *end = strchr(brace, '}');
            if (!end) end = brace + strlen(brace);
            char *scan = brace + 1;
            while (scan < end && step_count < MAX_PIPELINE) {
                while (*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r') scan++;
                if (*scan == '"') {
                    scan++;
                    int si = 0;
                    while (*scan && *scan != '"' && si < 63) step_names[step_count][si++] = *scan++;
                    step_names[step_count][si] = 0;
                    if (si > 0) step_count++;
                    if (*scan) scan++;
                } else break;
            }
        }
    }
    free(json);

    if (step_count == 0) {
        printf("[AGENT] No se encontraron steps en pipeline\n");
        return 0;
    }
    printf("[AGENT] %d steps encontrados\n", step_count);

    int all_ok = 0;
    for (int i = step_index; i < step_count; i++) {
        printf("\n[AGENT] === Step %d/%d: %s ===\n", i + 1, step_count, step_names[i]);
        if (execute_step(id, step_names[i]) != 0) {
            printf("[AGENT] Step %s fallo\n", step_names[i]);
            all_ok = -1;
            break;
        }
    }

    if (all_ok == 0) {
        printf("\n[AGENT] Pipeline '%s' completado\n", id);
        agent_mark_success(id);
    } else {
        agent_mark_fail(id);
    }
    return all_ok;
}

void agent_mark_success(const char *id) {
    for (int i = 0; i < agent_count; i++) {
        if (strcmp(agents[i].id, id) == 0) {
            agents[i].ejecuciones++;
            agents[i].exitosas++;
            break;
        }
    }
}

void agent_mark_fail(const char *id) {
    for (int i = 0; i < agent_count; i++) {
        if (strcmp(agents[i].id, id) == 0) {
            agents[i].ejecuciones++;
            agents[i].fallidas++;
            break;
        }
    }
}

void agent_supervisor_tick(void) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    printf("[SUPERVISOR] Tick - %02d:%02d %s\n",
           tm_now->tm_hour, tm_now->tm_min,
           tm_now->tm_wday == 1 ? "LUNES" : "otro dia");
    for (int i = 0; i < agent_count; i++) {
        if (!agents[i].activo) continue;
        if (strcmp(agents[i].schedule_tipo, "semanal") == 0) {
            int es_lunes = (tm_now->tm_wday == 1);
            int es_hora = (tm_now->tm_hour == 9 && tm_now->tm_min == 0);
            if (es_lunes && es_hora) {
                printf("[SUPERVISOR] Lanzando agente: %s\n", agents[i].id);
                agent_run(agents[i].id);
            }
        }
    }
}

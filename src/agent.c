#include "agent.h"
#include "bdgb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static char registry_path[512] = {0};
static char agents_dir[512] = {0};
static char config_path[512] = {0};

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

/* Simple JSON string value extractor */
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

int agent_load_all(void) {
    FILE *f = fopen(registry_path, "r");
    if (!f) {
        agent_count = 0;
        return 0;
    }

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

        int len = end - p + 1;
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
        if (strcmp(agents[i].id, id) == 0) {
            *out = agents[i];
            return 0;
        }
    }
    return -1;
}

int agent_list(AgentDef *out, int max_out) {
    int n = (agent_count < max_out) ? agent_count : max_out;
    for (int i = 0; i < n; i++) out[i] = agents[i];
    return n;
}

int agent_register(const char *id, const char *nombre, const char *schedule) {
    FILE *f = fopen(registry_path, "r");
    if (f) fclose(f);

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s/%s/config.json", agents_dir, id);
    f = fopen(dir, "r");
    if (f) { fclose(f); return -1; }

    return 0;
}

/*
 * Ejecuta un comando del pipeline llamando a system().
 * Lee la config del agente para obtener ruta y args de la herramienta.
 */
static int execute_step(const char *agent_id, const char *step_name) {
    /* Construir path a config.json del agente */
    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s/config.json", agents_dir, agent_id);

    FILE *f = fopen(cfg_path, "r");
    if (!f) {
        printf("[AGENT] Error: no se pudo leer %s\n", cfg_path);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc(size + 1);
    if (!json) { fclose(f); return -1; }
    fread(json, 1, size, f);
    json[size] = 0;
    fclose(f);

    /* Buscar la herramienta configurada para este step */
    char tool_name[64] = {0};
    {
        char search_key[128];
        snprintf(search_key, sizeof(search_key), "\"%s\"", step_name);
        char *step_block = strstr(json, search_key);
        if (step_block) {
            char *tool_p = strstr(step_block, "\"tool\"");
            if (tool_p) {
                tool_p = strchr(tool_p, ':');
                if (tool_p) {
                    tool_p++;
                    while (*tool_p == ' ' || *tool_p == '\t' || *tool_p == '\n' || *tool_p == '\r') tool_p++;
                    if (*tool_p == '"') {
                        tool_p++;
                        int ti = 0;
                        while (*tool_p && *tool_p != '"' && ti < 63) tool_name[ti++] = *tool_p++;
                        tool_name[ti] = 0;
                    }
                }
            }
        }
    }

    if (tool_name[0] == 0) {
        printf("[AGENT] Step '%s': no tool config found, skipping\n", step_name);
        free(json);
        return 0;
    }

    /* Buscar ruta de la herramienta en config */
    char tool_path[256] = {0};
    {
        char search_key[128];
        snprintf(search_key, sizeof(search_key), "\"%s\"", tool_name);
        char *tool_block = strstr(json, search_key);
        if (tool_block) {
            char *ruta_p = strstr(tool_block, "\"ruta\"");
            if (ruta_p) {
                ruta_p = strchr(ruta_p, ':');
                if (ruta_p) {
                    ruta_p++;
                    while (*ruta_p == ' ' || *ruta_p == '\t' || *ruta_p == '\n' || *ruta_p == '\r') ruta_p++;
                    if (*ruta_p == '"') {
                        ruta_p++;
                        int ri = 0;
                        while (*ruta_p && *ruta_p != '"' && ri < 255) tool_path[ri++] = *ruta_p++;
                        tool_path[ri] = 0;
                    }
                }
            }
        }
    }

    if (tool_path[0] == 0) {
        printf("[AGENT] Tool '%s': no ruta found, using tool name as command\n", tool_name);
        strncpy(tool_path, tool_name, sizeof(tool_path) - 1);
    }

    /* Ejecutar */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s", tool_path);

    printf("[AGENT] Ejecutando: %s\n", cmd);
    fflush(stdout);

    int ret = system(cmd);
    if (ret == 0) {
        printf("[AGENT] Step '%s' completado exitosamente\n", step_name);
    } else {
        printf("[AGENT] Step '%s' fallo con codigo %d\n", step_name, ret);
    }

    free(json);
    return ret;
}

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

    printf("[AGENT] Pipeline: %s (paso %d+)\n", id, step_index);

    /* Leer pipeline steps del config */
    char cfg_path[512];
    snprintf(cfg_path, sizeof(cfg_path), "%s/%s/config.json", agents_dir, id);

    FILE *f = fopen(cfg_path, "r");
    if (!f) {
        /* Fallback: usar pipeline del registry */
        for (int i = step_index; i < a.pipeline_len; i++) {
            printf("[AGENT]   -> %s\n", a.pipeline[i]);
        }
        printf("[AGENT] Pipeline completado (simulado - sin config)\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *json = (char*)malloc(size + 1);
    if (!json) { fclose(f); return -1; }
    fread(json, 1, size, f);
    json[size] = 0;
    fclose(f);

    /* Extraer nombres de los steps del pipeline */
    char *pipeline_block = strstr(json, "\"pipeline\"");
    if (!pipeline_block) {
        printf("[AGENT] No pipeline config found\n");
        free(json);
        return -1;
    }

    /* Extraer keys del objeto pipeline */
    int step_count = 0;
    char step_names[MAX_PIPELINE][64];

    char *brace = strchr(pipeline_block, '{');
    if (brace) {
        char *end = strchr(brace, '}');
        if (!end) end = brace + strlen(brace);
        /* Scan for quoted strings followed by : { */
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

    if (step_count == 0) {
        printf("[AGENT] No se encontraron steps en pipeline\n");
        free(json);
        return 0;
    }

    printf("[AGENT] %d steps encontrados en config\n", step_count);

    /* Ejecutar cada step desde step_index */
    int all_ok = 0;
    for (int i = step_index; i < step_count; i++) {
        printf("\n[AGENT] === Step %d/%d: %s ===\n", i + 1, step_count, step_names[i]);
        int ret = execute_step(id, step_names[i]);
        if (ret != 0) {
            printf("[AGENT] Step %s fallo\n", step_names[i]);
            all_ok = -1;
            break;
        }
    }

    if (all_ok == 0) {
        printf("\n[AGENT] Pipeline '%s' completado exitosamente\n", id);
        agent_mark_success(id);
    } else {
        agent_mark_fail(id);
    }

    free(json);
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

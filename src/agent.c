#include "agent.h"
#include "bdgb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define REGISTRY_PATH "C:/Users/famil/Desktop/glifos/bdgb/agents/registry.json"
#define AGENTS_DIR    "C:/Users/famil/Desktop/glifos/bdgb/agents"

static AgentDef agents[MAX_AGENTS];
static int agent_count = 0;
static int initialized = 0;

int agent_init(void) {
    agent_count = 0;
    memset(agents, 0, sizeof(agents));
    initialized = 1;
    agent_load_all();
    return 0;
}

static void trim_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r')) s[--len] = 0;
}

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
    FILE *f = fopen(REGISTRY_PATH, "r");
    if (!f) {
        agent_count = 0;
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

    agent_count = 0;
    char *p = json;
    int in_array = 0;

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
    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s/%s", AGENTS_DIR, id);

    FILE *f = fopen(REGISTRY_PATH, "r");
    int exists = (f != NULL);
    if (f) fclose(f);

    snprintf(dir, sizeof(dir), "%s/%s/config.json", AGENTS_DIR, id);
    f = fopen(dir, "r");
    if (f) { fclose(f); return -1; }

    return 0;
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

    printf("[AGENT] Pipeline paso %d/%d\n", step_index + 1, a.pipeline_len);

    for (int i = step_index; i < a.pipeline_len; i++) {
        printf("[AGENT]   -> %s\n", a.pipeline[i]);
    }

    printf("[AGENT] Pipeline completado (simulado)\n");
    return 0;
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

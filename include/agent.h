#ifndef BDGB_AGENT_H
#define BDGB_AGENT_H

#include <stdint.h>

#define MAX_AGENTS      32
#define MAX_PIPELINE    16
#define MAX_TOOLS       16
#define AGENT_PATH_MAX  512

typedef struct {
    char id[64];
    char nombre[128];
    char estado[16];
    char schedule_tipo[16];
    char schedule_dia[16];
    char schedule_hora[16];
    char pipeline[MAX_PIPELINE][64];
    int  pipeline_len;
    char herramientas[MAX_TOOLS][64];
    int  herramientas_len;
    uint32_t ejecuciones;
    uint32_t exitosas;
    uint32_t fallidas;
    int  activo;
} AgentDef;

int  agent_init(void);
int  agent_register(const char *id, const char *nombre, const char *schedule);
int  agent_load_all(void);
int  agent_get(const char *id, AgentDef *out);
int  agent_list(AgentDef *out, int max_out);
int  agent_run(const char *id);
int  agent_run_pipeline(const char *id, int step_index);
void agent_mark_success(const char *id);
void agent_mark_fail(const char *id);
void agent_supervisor_tick(void);

#endif

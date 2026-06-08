#ifndef BDGB_GLIFO_H
#define BDGB_GLIFO_H

#include <stdint.h>

#define MAX_GLIFOS         32
#define GLIFO_ID_MAX       64
#define GLIFO_NOMBRE_MAX   128
#define GLIFO_SISTEMA_MAX  64

/* Estado de retorno de un glifo */
#define GLIFO_OK       0
#define GLIFO_ERR      -1
#define GLIFO_SALTAR   -2   /* glifo no ejecutado (condicion no cumple) */

typedef struct {
    char id[GLIFO_ID_MAX];
    char nombre[GLIFO_NOMBRE_MAX];
    int (*run)(const char *args);
    char sistema[GLIFO_SISTEMA_MAX];   /* sistema al que pertenece */
    uint32_t ejecuciones;
    uint32_t exitosas;
    uint32_t fallidas;
} GlifoDef;

int  glifo_init(void);
int  glifo_register(const char *id, const char *nombre,
                    int (*run)(const char *args));
int  glifo_run(const char *id, const char *args);
int  glifo_list(GlifoDef *out, int max);
void glifo_mark_success(const char *id);
void glifo_mark_fail(const char *id);

/* Carga sistemas desde glifos/registry.json + glifosenilla.json.
   Marca cada glifo registrado con el sistema al que pertenece.
   Solo los glifos en sistemas activos pueden listarse y ejecutarse. */
int glifo_load_systems(void);

/* Glifos incorporados (built-in) */
int glifo_primo_run(const char *args);

#endif

/*
 * mmia.c — MMIA 2.0 en C puro
 * Subconsciente numerico (B_num ortonormal) + Sistema Orbital
 * Un solo ejecutable, cero dependencias externas.
 *
 * Compilar: gcc -o mmia.exe mmia.c -lm
 * Usar:     mmia.exe
 *           mmia.exe "abre el bloc de notas"
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "mmia.h"

/* ================================================================
 * CONFIGURACION
 * ================================================================ */

#define DIM          32   /* dimension del espacio de estado */
#define K             4   /* 4 leyes: neutro, inverso, conmutativo, distributivo */
#define THETA      0.12f  /* umbral de incompletitud */
#define DT         0.10f  /* paso temporal */
#define MAX_MODS    1000  /* max modulos orbitales */
#define MAX_NAME      64  /* tamano nombre modulo */
#define MAX_HISTORY  100  /* profundidad historial */

/* tipos de modulo */
#define TIPO_INNATE     0
#define TIPO_OBJETO     1
#define TIPO_CUALIDAD   2
#define TIPO_ACCION     3
#define TIPO_CONOCIMIENTO  4
#define MAX_ASOC        8
#define CTX_SIZE       16       /* profundidad del buffer de contexto */
#define MAX_TOOLS     128       /* max herramientas registradas */
#define MAX_TASKS 64
#define MAX_TASK_DESC 256
#define MAX_PROJECT_TYPE 64
#define MAX_PROJECT_NAME 256

/* asociacion entre modulos */
typedef struct {
    int dest_id;
    int tipo_asoc;   /* 0=usa, 1=compone, 2=dispara */
    float peso;
} Asociacion;

/* ================================================================
 * VECTORES
 * ================================================================ */

typedef struct { int n; float *d; } Vec;

Vec vec_new(int n) {
    Vec v; v.n = n;
    v.d = (float*)calloc(n, sizeof(float));
    if (!v.d) { fprintf(stderr, "error: alloc %d floats\n", n); exit(1); }
    return v;
}

void vec_free(Vec *v) { free(v->d); v->d = NULL; v->n = 0; }

void vec_set(Vec *v, int i, float val) { if (i < v->n) v->d[i] = val; }

float vec_get(Vec v, int i) { return (i < v.n) ? v.d[i] : 0.0f; }

void vec_copy(Vec *dst, Vec src) {
    int n = dst->n < src.n ? dst->n : src.n;
    memcpy(dst->d, src.d, n * sizeof(float));
}

Vec vec_clone(Vec v) {
    Vec c = vec_new(v.n);
    memcpy(c.d, v.d, v.n * sizeof(float));
    return c;
}

float vec_dot(Vec a, Vec b) {
    int n = a.n < b.n ? a.n : b.n;
    float s = 0; for (int i = 0; i < n; i++) s += a.d[i] * b.d[i];
    return s;
}

float vec_norm(Vec v) { return sqrtf(vec_dot(v, v)); }

void vec_normalize(Vec *v) {
    float n = vec_norm(*v);
    if (n > 1e-9f) { for (int i = 0; i < v->n; i++) v->d[i] /= n; }
}

void vec_add(Vec *a, Vec b) {
    int n = a->n < b.n ? a->n : b.n;
    for (int i = 0; i < n; i++) a->d[i] += b.d[i];
}

void vec_sub(Vec *a, Vec b) {
    int n = a->n < b.n ? a->n : b.n;
    for (int i = 0; i < n; i++) a->d[i] -= b.d[i];
}

void vec_scale(Vec *v, float s) { for (int i = 0; i < v->n; i++) v->d[i] *= s; }

Vec vec_sub_new(Vec a, Vec b) {
    int n = a.n < b.n ? a.n : b.n;
    Vec r = vec_new(n);
    for (int i = 0; i < n; i++) r.d[i] = a.d[i] - b.d[i];
    return r;
}

void vec_fill_rand(Vec *v) {
    for (int i = 0; i < v->n; i++) v->d[i] = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

/* ================================================================
 * MATRICES (almacenadas column-major para facilidad con vectores)
 * ================================================================ */

typedef struct { int rows, cols; float *d; } Mat;

Mat mat_new(int r, int c) {
    Mat m; m.rows = r; m.cols = c;
    m.d = (float*)calloc(r * c, sizeof(float));
    if (!m.d) { fprintf(stderr, "error: alloc %dx%d floats\n", r, c); exit(1); }
    return m;
}

void mat_free(Mat *m) { free(m->d); m->d = NULL; m->rows = m->cols = 0; }

float mat_get(Mat m, int r, int c) { return m.d[c * m.rows + r]; }
void  mat_set(Mat *m, int r, int c, float v) { m->d[c * m->rows + r] = v; }

Vec mat_col(Mat m, int c) {
    Vec v = vec_new(m.rows);
    for (int i = 0; i < m.rows; i++) v.d[i] = mat_get(m, i, c);
    return v;
}

void mat_set_col(Mat *m, int c, Vec v) {
    for (int i = 0; i < m->rows && i < v.n; i++) mat_set(m, i, c, v.d[i]);
}

Vec mat_mul_vec(Mat m, Vec v) {
    Vec r = vec_new(m.rows);
    for (int i = 0; i < m.rows; i++) {
        float s = 0;
        for (int j = 0; j < m.cols && j < v.n; j++) s += mat_get(m, i, j) * v.d[j];
        r.d[i] = s;
    }
    return r;
}

Mat mat_mul(Mat a, Mat b) {
    Mat r = mat_new(a.rows, b.cols);
    for (int i = 0; i < a.rows; i++)
        for (int j = 0; j < b.cols; j++) {
            float s = 0;
            for (int k = 0; k < a.cols; k++) s += mat_get(a, i, k) * mat_get(b, k, j);
            mat_set(&r, i, j, s);
        }
    return r;
}

Mat mat_transpose(Mat m) {
    Mat r = mat_new(m.cols, m.rows);
    for (int i = 0; i < m.rows; i++)
        for (int j = 0; j < m.cols; j++)
            mat_set(&r, j, i, mat_get(m, i, j));
    return r;
}

/* Gram-Schmidt: factoriza QR in-place (columnas de m se ortonormalizan) */
void gram_schmidt(Mat *m) {
    for (int j = 0; j < m->cols; j++) {
        Vec v = mat_col(*m, j);
        for (int i = 0; i < j; i++) {
            Vec ui = mat_col(*m, i);
            float proj = vec_dot(v, ui);
            for (int k = 0; k < v.n; k++) v.d[k] -= proj * ui.d[k];
            vec_free(&ui);
        }
        vec_normalize(&v);
        mat_set_col(m, j, v);
        vec_free(&v);
    }
}

/* ================================================================
 * SUBCONSCIENTE NUMERICO (B_num)
 * ================================================================ */

typedef struct {
    int d, k;
    Mat B_num;   /* d x k, ortonormal: B_num^T * B_num = I_k */
    Vec props;   /* firma binaria [1,1,1,1] */
    Mat op;      /* k x k, identidad inicial */
} Subconscious;

void subc_init(Subconscious *sc, int d, int k) {
    sc->d = d; sc->k = k;
    sc->B_num = mat_new(d, k);
    /* llenar con aleatorios y ortonormalizar via Gram-Schmidt */
    for (int j = 0; j < k; j++) {
        Vec v = vec_new(d);
        vec_fill_rand(&v);
        mat_set_col(&sc->B_num, j, v);
        vec_free(&v);
    }
    gram_schmidt(&sc->B_num);

    sc->props = vec_new(k);
    for (int i = 0; i < k; i++) sc->props.d[i] = 1.0f;

    sc->op = mat_new(k, k);
    for (int i = 0; i < k; i++) mat_set(&sc->op, i, i, 1.0f);
}

void subc_free(Subconscious *sc) {
    mat_free(&sc->B_num);
    vec_free(&sc->props);
    mat_free(&sc->op);
}

Vec subc_project(Subconscious *sc, Vec x) {
    /* P = B_num * B_num^T * x */
    Mat Bt = mat_transpose(sc->B_num);   /* k x d */
    Vec tmp = mat_mul_vec(Bt, x);        /* k */
    Vec r = mat_mul_vec(sc->B_num, tmp); /* d */
    mat_free(&Bt); vec_free(&tmp);
    return r;
}

float subc_evaluate(Subconscious *sc, Vec x, Vec *residual) {
    Vec xp = subc_project(sc, x);
    Vec res = vec_sub_new(x, xp);
    float LI = vec_norm(res) / (vec_norm(x) + 1e-9f);
    if (residual) *residual = res; else vec_free(&res);
    vec_free(&xp);
    return LI;
}

/* ================================================================
 * MODULO ORBITAL
 * ================================================================ */

typedef struct {
    char name[MAX_NAME];
    Vec properties;     /* firma [0,1]^p */
    float orbit_radius, omega, lambda_ret, alpha, tau;
    int source;         /* 0=innate, 1=learned */
    int tipo;           /* TIPO_INNATE, _OBJETO, _CUALIDAD, _ACCION, _CONOCIMIENTO */
    float position, angle, activation;
    int usage_count, is_active;
    Asociacion asociaciones[MAX_ASOC];  /* vinculos con otros modulos */
    int num_asociaciones;
} OrbitalModule;

/* forward declarations (struct OrbitalSystem se define mas abajo) */
struct OrbitalSystem;
Vec orbital_encode_text(struct OrbitalSystem *sys, const char *text);
void orbital_add_module(struct OrbitalSystem *sys, const char *name,
                        float *props, int source, float tau);
void orbital_analyze_tokens(struct OrbitalSystem *sys, const char *text);
void orbital_visualize(struct OrbitalSystem *sys);
void orbital_save(struct OrbitalSystem *sys, const char *path);
int orbital_load(struct OrbitalSystem *sys, const char *path);
void orbital_push_context(struct OrbitalSystem *sys, const char *input);
int orbital_find_objeto(struct OrbitalSystem *sys, const char *name);
void ejecutar_comando_raw(const char *cmd, int show_output);
void ejecutar_comando(const char *cmd);
void orbital_load_tools(struct OrbitalSystem *sys, const char *path);
void orbital_list_tasks(struct OrbitalSystem *sys);
void orbital_add_task(struct OrbitalSystem *sys, const char *desc);
int orbital_next_task(struct OrbitalSystem *sys);
void orbital_clear_tasks(struct OrbitalSystem *sys);
void orbital_list_directory(const char *path);
int crear_proyecto(struct OrbitalSystem *sys, const char *tipo, const char *nombre);

float module_resonance(OrbitalModule *m, Vec context) {
    int p = m->properties.n < context.n ? m->properties.n : context.n;
    float num = 0, den = 0;
    for (int i = 0; i < p; i++) {
        float a = m->properties.d[i], b = context.d[i];
        num += (a < b ? a : b);
        den += (a > b ? a : b);
    }
    return num / (den + 1e-9f);
}

void module_step(OrbitalModule *m, Vec context, float P_pos, float dt) {
    float rho = module_resonance(m, context);
    float k = 5.0f;
    float sig = 1.0f / (1.0f + expf(-k * (rho - m->tau)));

    float orbital = m->omega * sinf(m->angle);
    float retorno = -m->lambda_ret * (m->position - m->orbit_radius);
    float atraccion = m->alpha * sig * (P_pos - m->position);

    m->position += dt * (orbital + retorno + atraccion);
    if (m->position < 0) m->position = 0;
    if (m->position > 2) m->position = 2;

    m->angle += dt * m->omega * 2 * (float)M_PI;
    m->activation = sig;
    m->is_active = m->position < 0.4f;
    if (m->is_active) m->usage_count++;
}

/* ================================================================
 * SISTEMA ORBITAL
 * ================================================================ */

typedef struct {
    char cualidad[MAX_NAME];
    char comando[512];
} ToolReg;

typedef struct {
    char desc[MAX_TASK_DESC];
    int done;
} TaskItem;

/* plantillas de proyecto */
static const char *manga_folders[]  = {"guion","personajes","paginas","tramas","portada","assets",NULL};
static const char *anime_folders[]  = {"storyboard","escenas","audio","render","assets","guion",NULL};
static const char *videojuego_folders[] = {"assets","scripts","niveles","ui","audio","personajes","escenarios",NULL};
static const char *novela_folders[] = {"capitulos","personajes","escenarios","portada",NULL};
static const char *historieta_folders[] = {"guion","paginas","personajes","portada",NULL};
static const char *musica_folders[] = {"canciones","instrumentos","mezcla",NULL};

typedef struct {
    const char *type;
    const char **folders;
} ProjectTemplate;

static const ProjectTemplate project_types[] = {
    {"manga", manga_folders},
    {"anime", anime_folders},
    {"videojuego", videojuego_folders},
    {"novela", novela_folders},
    {"historieta", historieta_folders},
    {"musica", musica_folders},
    {NULL, NULL}
};

typedef struct OrbitalSystem {
    int p, t, n_modules;
    float dt, theta, L_factor, P_position;
    Subconscious sc;
    OrbitalModule modules[MAX_MODS];
    Vec last_residual;
    float last_LI;
    char context[CTX_SIZE][1024];  /* buffer circular de contexto */
    int ctx_idx, ctx_count;
    ToolReg tools[MAX_TOOLS];
    int n_tools;
    TaskItem task_queue[MAX_TASKS];
    int n_tasks;
    char current_project[MAX_PROJECT_NAME];
    char project_type[MAX_PROJECT_TYPE];
} OrbitalSystem;

void orbital_init(OrbitalSystem *sys, int p, float theta, float dt) {
    sys->p = p; sys->t = 0; sys->n_modules = 0;
    sys->dt = dt; sys->theta = theta;
    sys->L_factor = 1.0f; sys->P_position = 0.0f;
    sys->last_LI = 0;
    sys->last_residual = vec_new(p);

    subc_init(&sys->sc, p, K);

    /* 4 modulos innatos */
    const char *innate_names[] = {"aritmetica", "logica", "lenguaje", "memoria_corta"};
    float innate_tau[]  = {0.15f, 0.15f, 0.15f, 0.10f};
    float innate_lam[]  = {0.7f, 0.7f, 0.6f, 0.8f};
    float innate_omg[]  = {0.12f, 0.11f, 0.09f, 0.08f};
    float innate_rad[]  = {0.5f, 0.5f, 0.5f, 0.5f};

    for (int i = 0; i < 4 && i < MAX_MODS; i++) {
        OrbitalModule *m = &sys->modules[sys->n_modules++];
        strncpy(m->name, innate_names[i], MAX_NAME-1);
        /* properties aleatorias con semilla fija */
        m->properties = vec_new(p);
        srand(42 + i);
        for (int j = 0; j < p; j++) m->properties.d[j] = (float)rand()/(float)RAND_MAX;
        vec_normalize(&m->properties);

        m->orbit_radius = innate_rad[i];
        m->omega = innate_omg[i];
        m->lambda_ret = innate_lam[i];
        m->alpha = 1.5f;
        m->tau = innate_tau[i];
        m->source = 0; /* innate */
        m->position = 0.5f;
        m->angle = (float)(rand() % 1000) / 1000.0f * 2 * (float)M_PI;
        m->activation = 0;
        m->usage_count = 0;
        m->is_active = 0;
        m->tipo = TIPO_INNATE;
        m->num_asociaciones = 0;
    }

    /* buffer de contexto */
    sys->ctx_idx = 0;
    sys->ctx_count = 0;
    for (int i = 0; i < CTX_SIZE; i++) sys->context[i][0] = 0;

    /* modulos de vocabulario semilla */
    const char *seed_names[] = {
        /* OBJETOS — 12 originales */
        "yo","sistema","usuario","memoria","modulo",
        "archivo","comando","texto","entrada","salida",
        "ahora","estado",
        /* OBJETOS — nuevos (~30) */
        "carpeta","proyecto","recurso","version",
        "imagen","capa","color","forma","lienzo","pincel","paleta",
        "pagina","vineta","bocadillo","escena","plano",
        "personaje","sprite","modelo","nivel","escenario",
        "video","audio","fotograma","animacion","clip",
        "idea","concepto","prototipo","diseno","referencia",
        "documento","informe","nota","trama","portada",
        /* CUALIDADES — 10 originales */
        "crear","guardar","cargar","mostrar","listar",
        "decir","ejecutar","hacer","saber","aprender",
        /* CUALIDADES — nuevos (~20) */
        "abrir","cerrar","nuevo",
        "importar","exportar","convertir",
        "editar","recortar","rotar","escalar",
        "pintar","dibujar","colorear","rellenar",
        "buscar","seleccionar","inspeccionar",
        "renderizar","previsualizar","probar",
        "commit","push","pull"
    };
    int seed_tipos[] = {
        /* objetos originales */
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,
        /* objetos nuevos */
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,TIPO_OBJETO,
        /* cualidades originales */
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        /* cualidades nuevos */
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD,
        TIPO_CUALIDAD,TIPO_CUALIDAD,TIPO_CUALIDAD
    };
    int n_seeds = sizeof(seed_names) / sizeof(seed_names[0]);

    for (int i = 0; i < n_seeds; i++) {
        Vec sv = orbital_encode_text(sys, seed_names[i]);
        float *props = (float*)malloc(sys->p * sizeof(float));
        for (int j = 0; j < sys->p; j++) props[j] = sv.d[j];
        orbital_add_module(sys, seed_names[i], props, 0, 0.15f);
        sys->modules[sys->n_modules-1].tipo = seed_tipos[i];
        free(props);
        vec_free(&sv);
    }
    /* cola de tareas y proyecto */
    sys->n_tasks = 0;
    sys->current_project[0] = 0;
    sys->project_type[0] = 0;

    orbital_load_tools(sys, "mmia_tools.json");
}

void orbital_free(OrbitalSystem *sys) {
    subc_free(&sys->sc);
    vec_free(&sys->last_residual);
    for (int i = 0; i < sys->n_modules; i++)
        vec_free(&sys->modules[i].properties);
}

void orbital_add_module(OrbitalSystem *sys, const char *name,
                        float *props, int source,
                        float tau) {
    if (sys->n_modules >= MAX_MODS) return;
    OrbitalModule *m = &sys->modules[sys->n_modules++];
    strncpy(m->name, name, MAX_NAME-1);
    m->properties = vec_new(sys->p);
    memcpy(m->properties.d, props, sys->p * sizeof(float));
    vec_normalize(&m->properties);

    m->orbit_radius = (source == 0) ? 0.5f : 1.2f;
    m->omega = 0.08f + (float)(rand() % 100) / 1000.0f;
    m->lambda_ret = 0.5f;
    m->alpha = 1.0f;
    m->tau = tau;
    m->source = source;
    m->tipo = TIPO_CONOCIMIENTO;
    m->position = m->orbit_radius;
    m->angle = (float)(rand() % 1000) / 1000.0f * 2 * (float)M_PI;
    m->activation = 0;
    m->usage_count = 0;
    m->is_active = 0;
    m->num_asociaciones = 0;
}

void orbital_expand_from_residual(OrbitalSystem *sys, Vec residual) {
    char name[MAX_NAME];
    snprintf(name, MAX_NAME, "mod_%03d", sys->n_modules + 1);
    float *props = (float*)malloc(sys->p * sizeof(float));
    float norm = vec_norm(residual);
    int p = residual.n < sys->p ? residual.n : sys->p;
    for (int i = 0; i < p; i++) props[i] = fabsf(residual.d[i]) / (norm + 1e-9f);
    for (int i = p; i < sys->p; i++) props[i] = 0;
    orbital_add_module(sys, name, props, 1, 0.30f);
    free(props);
}

Vec orbital_encode_text(OrbitalSystem *sys, const char *text) {
    Vec v = vec_new(sys->p);
    int len = (int)strlen(text);
    for (int d = 0; d < sys->p; d++) {
        unsigned int h = 2166136261u ^ (unsigned int)d;
        for (int i = 0; i < len; i++)
            h = (h ^ (unsigned char)text[i]) * 16777619u;
        v.d[d] = (h % 32768) / 32768.0f;
    }
    vec_normalize(&v);
    return v;
}

void orbital_process(OrbitalSystem *sys, const char *text) {
    sys->t++;
    Vec x = orbital_encode_text(sys, text);

    /* 1. Subconsciente evalua */
    Vec residual;
    float LI = subc_evaluate(&sys->sc, x, &residual);
    vec_copy(&sys->last_residual, residual);
    sys->last_LI = LI;

    /* 2. Decision */
    if (LI > sys->theta) {
        /* Ver si ya aprendimos algo similar */
        Vec ctx = vec_clone(residual);
        for (int i = 0; i < ctx.n; i++) ctx.d[i] = fabsf(ctx.d[i]);
        vec_normalize(&ctx);

        float max_rho = 0;
        int best_idx = -1;
        for (int i = 4; i < sys->n_modules; i++) { /* solo learned */
            float rho = module_resonance(&sys->modules[i], ctx);
            if (rho > max_rho) { max_rho = rho; best_idx = i; }
        }

        if (best_idx >= 0 && max_rho > sys->modules[best_idx].tau) {
            sys->L_factor = 0.9f;
            printf("[MMIA] LI=%.4f > THETA  resuena con '%s' (rho=%.3f)"
                   "  -> CONTRACCION (L=%.2f)\n",
                   LI, sys->modules[best_idx].name, max_rho, sys->L_factor);
        } else {
            sys->L_factor = 2.3947f;
            orbital_expand_from_residual(sys, residual);
            printf("[MMIA] LI=%.4f > THETA  nuevo modulo  -> EXPANSION (L=%.2f)\n",
                   LI, sys->L_factor);
        }
        vec_free(&ctx);
    } else {
        sys->L_factor = 0.9f;
        Vec xp = subc_project(&sys->sc, x);
        vec_copy(&x, xp);
        vec_free(&xp);
        printf("[MMIA] LI=%.4f <= THETA  -> CONTRACCION (L=%.2f)\n",
               LI, sys->L_factor);
    }

    /* 3. Contraccion/expansion de Banach */
    if (sys->L_factor < 1.0f) {
        vec_scale(&x, 1.0f - sys->L_factor * 0.1f);
    } else {
        vec_scale(&x, sys->L_factor);
    }

    /* 4. Modulos orbitales */
    Vec C = x; /* estado = contexto */
    for (int i = 0; i < sys->n_modules; i++) {
        module_step(&sys->modules[i], C, sys->P_position, sys->dt);
    }

    /* Actualizar P_position */
    int has_active = 0;
    for (int i = 0; i < sys->n_modules; i++)
        if (sys->modules[i].is_active) { has_active = 1; break; }
    if (has_active)
        sys->P_position = sys->P_position > 0.05f ? sys->P_position - 0.05f : 0;
    else
        sys->P_position = sys->P_position < 0.2f ? sys->P_position + 0.02f : 0.2f;

    /* 5. Guardar contexto y analizar tokens */
    if (strlen(text) > 0 && text[0] != '!') {
        orbital_push_context(sys, text);
        orbital_analyze_tokens(sys, text);
    }

    vec_free(&x);
    vec_free(&residual);
}

/* ================================================================
 * EJECUTOR DE ACCIONES
 * ================================================================ */

void ejecutar_accion(OrbitalSystem *sys, const char *cualidad, const char *objeto) {
    char cmd[4096];
    if (strcmp(cualidad, "mostrar") == 0) {
        if (strcmp(objeto, "estado") == 0) orbital_visualize(sys);
        else printf("[ACCION] mostrando '%s'\n", objeto);
    } else if (strcmp(cualidad, "guardar") == 0) {
        if (strcmp(objeto, "memoria") == 0 || strcmp(objeto, "todo") == 0)
            orbital_save(sys, "mmia_memory.json");
        else printf("[ACCION] guardando '%s'\n", objeto);
    } else if (strcmp(cualidad, "cargar") == 0) {
        orbital_load(sys, "mmia_memory.json");
    } else if (strcmp(cualidad, "ejecutar") == 0 || strcmp(cualidad, "hacer") == 0) {
        ejecutar_comando(objeto);
    } else if (strcmp(cualidad, "decir") == 0) {
        printf("%s\n", objeto);
    } else if (strcmp(cualidad, "listar") == 0) {
        for (int i = 0; i < sys->n_modules; i++)
            printf("  %s (%s)\n", sys->modules[i].name,
                   sys->modules[i].tipo == TIPO_OBJETO ? "objeto" :
                   sys->modules[i].tipo == TIPO_CUALIDAD ? "cualidad" :
                   sys->modules[i].tipo == TIPO_ACCION ? "accion" : "conocimiento");
    } else if (strcmp(cualidad, "saber") == 0) {
        printf("[ACCION] LI=%.4f  modulos=%d\n", sys->last_LI, sys->n_modules);
    } else if (strcmp(cualidad, "crear") == 0) {
        if (strncmp(objeto, "proyecto ", 9) == 0) {
            char ptype[MAX_PROJECT_TYPE], pname[MAX_PROJECT_NAME];
            char resto[256]; resto[0] = 0;
            int n = sscanf(objeto + 9, "%s %s %255c", ptype, pname, resto);
            if (n >= 2) {
                /* limpiar resto (posible trailing whitespace) */
                for (int i = (int)strlen(resto)-1; i >= 0 && resto[i] == ' '; i--) resto[i] = 0;
                if (strlen(resto) > 0) {
                    strncat(pname, " ", sizeof(pname)-strlen(pname)-1);
                    strncat(pname, resto, sizeof(pname)-strlen(pname)-1);
                }
                crear_proyecto(sys, ptype, pname);
            } else {
                printf("[ACCION] usar: crear proyecto <tipo> <nombre>\n");
                printf("  tipos: manga, anime, videojuego, novela, historieta, musica\n");
            }
        } else {
            printf("[ACCION] creando '%s'...\n", objeto);
            snprintf(cmd, sizeof(cmd), "if (-not (Test-Path \"%s\")) { New-Item -ItemType Directory -Path \"%s\" -Force }", objeto, objeto);
            ejecutar_comando_raw(cmd, 0);
        }
    } else if (strcmp(cualidad, "abrir") == 0) {
        printf("[ACCION] abriendo '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "cerrar") == 0) {
        printf("[ACCION] cerrando '%s' (simulado)\n", objeto);
    } else if (strcmp(cualidad, "nuevo") == 0) {
        printf("[ACCION] nuevo '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "New-Item -ItemType File -Path \"%s\" -Force", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "importar") == 0) {
        printf("[ACCION] importando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "Copy-Item -Path \"%s\" -Destination . -Recurse -Force -ErrorAction SilentlyContinue", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "exportar") == 0 || strcmp(cualidad, "convertir") == 0) {
        printf("[ACCION] exportando/convirtiendo '%s'\n", objeto);
    } else if (strcmp(cualidad, "editar") == 0) {
        printf("[ACCION] editando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "notepad.exe \"%s\"", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "buscar") == 0) {
        printf("[ACCION] buscando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "Get-ChildItem -Recurse -Filter \"*%s*\" | Select-Object FullName", objeto);
        ejecutar_comando_raw(cmd, 1);
    } else if (strcmp(cualidad, "pintar") == 0 || strcmp(cualidad, "dibujar") == 0 ||
               strcmp(cualidad, "colorear") == 0 || strcmp(cualidad, "rellenar") == 0) {
        printf("[ACCION] %s sobre '%s' (abriendo editor...)\n", cualidad, objeto);
        snprintf(cmd, sizeof(cmd), "start mspaint.exe \"%s\"", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "renderizar") == 0) {
        printf("[ACCION] renderizando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "Write-Output \"[RENDER] %s - iniciado a las $(Get-Date -Format 'HH:mm:ss')\"", objeto);
        ejecutar_comando_raw(cmd, 1);
    } else if (strcmp(cualidad, "previsualizar") == 0) {
        printf("[ACCION] previsualizando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", objeto);
        ejecutar_comando_raw(cmd, 0);
    } else if (strcmp(cualidad, "commit") == 0) {
        printf("[ACCION] git commit: '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "git add -A; git commit -m \"%s\"", objeto);
        ejecutar_comando_raw(cmd, 1);
    } else if (strcmp(cualidad, "push") == 0) {
        printf("[ACCION] git push\n");
        ejecutar_comando_raw("git push", 1);
    } else if (strcmp(cualidad, "pull") == 0) {
        printf("[ACCION] git pull\n");
        ejecutar_comando_raw("git pull", 1);
    } else if (strcmp(cualidad, "recortar") == 0 || strcmp(cualidad, "rotar") == 0 ||
               strcmp(cualidad, "escalar") == 0) {
        printf("[ACCION] transformando '%s' (%s)\n", objeto, cualidad);
    } else if (strcmp(cualidad, "seleccionar") == 0) {
        printf("[ACCION] seleccionado '%s'\n", objeto);
    } else if (strcmp(cualidad, "inspeccionar") == 0) {
        printf("[ACCION] inspeccionando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "Get-Item \"%s\" | Select-Object *", objeto);
        ejecutar_comando_raw(cmd, 1);
    } else if (strcmp(cualidad, "probar") == 0) {
        printf("[ACCION] probando '%s'\n", objeto);
        snprintf(cmd, sizeof(cmd), "if (Test-Path \"%s\") { Write-Output \"OK: %s existe\" } else { Write-Output \"NO: %s no encontrado\" }", objeto, objeto, objeto);
        ejecutar_comando_raw(cmd, 1);
    } else {
        int found = 0;
        for (int i = 0; i < sys->n_tools; i++) {
            if (strcmp(sys->tools[i].cualidad, cualidad) == 0) {
                char cmd[4096];
                snprintf(cmd, sizeof(cmd), sys->tools[i].comando, objeto);
                printf("[TOOL] ejecutando herramienta registrada: %s\n", cualidad);
                ejecutar_comando_raw(cmd, 1);
                found = 1;
                break;
            }
        }
        if (!found)
            printf("[ACCION] %s + %s  (aprendido, %d modulos)\n",
                   cualidad, objeto, sys->n_modules);
    }
}

/* ================================================================
 * CONTEXTO Y AYUDA
 * ================================================================ */

void orbital_push_context(OrbitalSystem *sys, const char *input) {
    strncpy(sys->context[sys->ctx_idx], input, 1023);
    sys->context[sys->ctx_idx][1023] = 0;
    sys->ctx_idx = (sys->ctx_idx + 1) % CTX_SIZE;
    if (sys->ctx_count < CTX_SIZE) sys->ctx_count++;
}

int orbital_find_objeto(OrbitalSystem *sys, const char *name) {
    for (int i = 4; i < sys->n_modules; i++)
        if (sys->modules[i].tipo == TIPO_OBJETO &&
            strcmp(sys->modules[i].name, name) == 0)
            return i;
    return -1;
}

/* buscar el último objeto mencionado en el contexto */
int orbital_last_objeto(OrbitalSystem *sys) {
    /* recorrer contexto desde el más reciente hacia atrás */
    int idx = sys->ctx_idx;
    for (int c = 0; c < sys->ctx_count; c++) {
        idx = (idx - 1 + CTX_SIZE) % CTX_SIZE;
        char buf[1024];
        strncpy(buf, sys->context[idx], 1023);
        buf[1023] = 0;
        char *tokens[64];
        int nt = 0;
        char *p = strtok(buf, " ");
        while (p && nt < 64) { tokens[nt++] = p; p = strtok(NULL, " "); }
        for (int t = nt-1; t >= 0; t--) {
            int oid = orbital_find_objeto(sys, tokens[t]);
            if (oid >= 0) return oid;
        }
    }
    return -1;
}

/* ================================================================
 * ANALISIS DE TOKENS (objeto + cualidad → accion)
 * ================================================================ */

void orbital_analyze_tokens(OrbitalSystem *sys, const char *text) {
    char buf[1024];
    strncpy(buf, text, sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    char *tokens[64];
    int ntokens = 0;
    char *p = strtok(buf, " ");
    while (p && ntokens < 64) { tokens[ntokens++] = p; p = strtok(NULL, " "); }

    if (ntokens < 1) return;

    /* busqueda multi‑token: probar secuencias de 1..4 tokens contra nombres de modulos */
    int cualidad_id = -1;
    int cualidad_token_idx = -1;
    int objeto_id = -1;
    int token_consumed[64] = {0};
    int is_tool = 0;
    char tool_name[MAX_NAME] = "";

    for (int start = 0; start < ntokens && !(cualidad_id >= 0 && objeto_id >= 0); start++) {
        if (token_consumed[start]) continue;
        char phrase[256] = {0};
        for (int len = 1; len <= 4 && start + len <= ntokens; len++) {
            if (len > 1) { strcat(phrase, " "); strcat(phrase, tokens[start + len - 1]); }
            else strcpy(phrase, tokens[start]);
            for (int i = 4; i < sys->n_modules; i++) {
                if (strcmp(sys->modules[i].name, phrase) != 0) continue;
                int tipo = sys->modules[i].tipo;
                if (tipo == TIPO_OBJETO && cualidad_id >= 0 && objeto_id < 0) {
                    for (int c = start; c < start + len; c++) token_consumed[c] = 1;
                    objeto_id = i;
                    break;
                }
                if (tipo == TIPO_CUALIDAD && cualidad_id < 0) {
                    for (int c = start; c < start + len; c++) token_consumed[c] = 1;
                    cualidad_id = i;
                    cualidad_token_idx = start;
                    break;
                }
                if (tipo == TIPO_ACCION && cualidad_id < 0 && objeto_id < 0) {
                    for (int c = start; c < start + len; c++) token_consumed[c] = 1;
                    cualidad_id = i;
                    cualidad_token_idx = start;
                    break;
                }
            }
            /* si no se encontro en modulos, buscar en herramientas registradas */
            if (cualidad_id < 0 && sys->n_tools > 0) {
                for (int t = 0; t < sys->n_tools; t++) {
                    if (strcmp(sys->tools[t].cualidad, phrase) == 0) {
                        for (int c = start; c < start + len; c++) token_consumed[c] = 1;
                        strncpy(tool_name, phrase, sizeof(tool_name)-1);
                        is_tool = 1;
                        cualidad_token_idx = start;
                        break;
                    }
                }
            }
            if (objeto_id >= 0 || cualidad_id >= 0 || is_tool) break;
        }
    }

    /* si se encontro una herramienta registrada, tratarla como cualidad */
    if (is_tool && cualidad_id < 0) {
        /* buscar un token que sea objeto */
        for (int start = 0; start < ntokens && objeto_id < 0; start++) {
            if (token_consumed[start]) continue;
            char phrase[256] = {0};
            for (int len = 1; len <= 4 && start + len <= ntokens; len++) {
                if (len > 1) { strcat(phrase, " "); strcat(phrase, tokens[start + len - 1]); }
                else strcpy(phrase, tokens[start]);
                for (int i = 4; i < sys->n_modules; i++) {
                    if (strcmp(sys->modules[i].name, phrase) == 0 &&
                        sys->modules[i].tipo == TIPO_OBJETO) {
                        for (int c = start; c < start + len; c++) token_consumed[c] = 1;
                        objeto_id = i;
                        break;
                    }
                }
                if (objeto_id >= 0) break;
            }
        }
    }

    /* construir obj_name: objeto + tokens no consumidos */
    char obj_name[MAX_NAME * 4];
    obj_name[0] = 0;
    if (cualidad_id >= 0 || is_tool) {
        /* si hay objeto explicito, empezar con su nombre */
        if (objeto_id >= 0) {
            strncpy(obj_name, sys->modules[objeto_id].name, sizeof(obj_name)-1);
        }
        /* anadir tokens no consumidos */
        for (int t = 0; t < ntokens; t++) {
            if (token_consumed[t]) continue;
            if (strlen(obj_name) > 0)
                strncat(obj_name, " ", sizeof(obj_name)-strlen(obj_name)-1);
            strncat(obj_name, tokens[t], sizeof(obj_name)-strlen(obj_name)-1);
        }
        /* FALLBACK: usar ultimo objeto del contexto */
        if (strlen(obj_name) == 0 && sys->ctx_count > 0) {
            int last_oid = orbital_last_objeto(sys);
            if (last_oid >= 0) {
                objeto_id = last_oid;
                strncpy(obj_name, sys->modules[objeto_id].name, sizeof(obj_name)-1);
                printf("[CONTEXTO] usando objeto anterior: '%s'\n", obj_name);
            }
        }
    }

    if (cualidad_id < 0 && !is_tool) return;

    char cual_name[MAX_NAME];
    if (is_tool)
        strncpy(cual_name, tool_name, MAX_NAME-1);
    else
        strncpy(cual_name, sys->modules[cualidad_id].name, MAX_NAME-1);

    char action_name[MAX_NAME];
    if (objeto_id >= 0) {
        snprintf(action_name, MAX_NAME, "%s_%s", cual_name,
                 sys->modules[objeto_id].name);
    } else if (strlen(obj_name) > 0) {
        snprintf(action_name, MAX_NAME, "%s_%s", cual_name, obj_name);
        for (char *c = action_name; *c; c++)
            if (!isalnum(*c) && *c != '_') *c = '_';
    } else {
        strncpy(action_name, cual_name, MAX_NAME-1);
    }

    /* buscar o crear la accion */
    int action_id = -1;
    if (strlen(action_name) > 0) {
        for (int i = 0; i < sys->n_modules; i++) {
            if (sys->modules[i].tipo == TIPO_ACCION &&
                strcmp(sys->modules[i].name, action_name) == 0) {
                action_id = i;
                break;
            }
        }
    }

    if (action_id >= 0) {
        sys->modules[action_id].usage_count++;
        printf("[TOKEN] accion reconocida: '%s' (%d usos)\n",
               action_name, sys->modules[action_id].usage_count);
    } else if (strlen(action_name) > 0 && !is_tool) {
        Vec qv = sys->modules[cualidad_id].properties;
        float *props = (float*)malloc(sys->p * sizeof(float));
        if (objeto_id >= 0) {
            Vec ov = sys->modules[objeto_id].properties;
            for (int i = 0; i < sys->p; i++)
                props[i] = (qv.d[i] + ov.d[i]) * 0.5f;
        } else {
            Vec ov = orbital_encode_text(sys, obj_name);
            for (int i = 0; i < sys->p; i++)
                props[i] = (qv.d[i] + ov.d[i]) * 0.5f;
            vec_free(&ov);
        }
        orbital_add_module(sys, action_name, props, 1, 0.20f);
        sys->modules[sys->n_modules-1].tipo = TIPO_ACCION;
        int mid = sys->n_modules - 1;
        sys->modules[mid].asociaciones[0].dest_id = cualidad_id;
        sys->modules[mid].asociaciones[0].tipo_asoc = 0;
        sys->modules[mid].asociaciones[0].peso = 1.0f;
        if (objeto_id >= 0) {
            sys->modules[mid].asociaciones[1].dest_id = objeto_id;
            sys->modules[mid].asociaciones[1].tipo_asoc = 1;
            sys->modules[mid].asociaciones[1].peso = 1.0f;
            sys->modules[mid].num_asociaciones = 2;
        } else {
            sys->modules[mid].num_asociaciones = 1;
        }
        printf("[TOKEN] nueva accion: '%s'\n", action_name);
        free(props);
    }

    if (strlen(obj_name) > 0)
        ejecutar_accion(sys, cual_name, obj_name);
    else if (strlen(cual_name) > 0)
        ejecutar_accion(sys, cual_name, "");
}

/* ================================================================
 * VISUALIZACION
 * ================================================================ */

void orbital_visualize(OrbitalSystem *sys) {
    printf("\n=======================================================\n");
    printf("  SISTEMA ORBITAL  t=%d  modulos=%d\n", sys->t, sys->n_modules);
    printf("-------------------------------------------------------\n");
    const char *phase = sys->L_factor > 1.0f ? "EXPANSION" : "CONTRACCION";
    printf("  SUBCONSCIENTE: dim=%d  B_num OK  |  L=%.2f  [%s]\n",
           sys->sc.d, sys->L_factor, phase);
    printf("-------------------------------------------------------\n");

    for (int i = 0; i < sys->n_modules; i++) {
        OrbitalModule *m = &sys->modules[i];
        int bar_len = (int)((1.0f - m->position / 2.0f) * 20);
        char bar[25];
        for (int j = 0; j < bar_len && j < 20; j++) bar[j] = '#';
        for (int j = bar_len; j < 20; j++) bar[j] = '.';
        bar[20] = 0;
        char tag = (m->source == 0) ? '*' : ' ';
        const char *tipo_str = "";
        if (m->tipo == TIPO_OBJETO) tipo_str = "obj";
        else if (m->tipo == TIPO_CUALIDAD) tipo_str = "cual";
        else if (m->tipo == TIPO_ACCION) tipo_str = "acc";
        else if (m->tipo == TIPO_CONOCIMIENTO) tipo_str = "con";
        else tipo_str = "inn";
        char status[16];
        snprintf(status, 16, "%-4s%s", tipo_str, m->is_active ? "[A]" : "   ");
        printf("  %c%s [%s] %-20s rho=%.2f\n",
               tag, status, bar, m->name, m->activation);
        if (m->num_asociaciones > 0) {
            printf("  %*s -> ", (int)(sizeof("  *       [")-1), "");
            for (int a = 0; a < m->num_asociaciones; a++) {
                int aid = m->asociaciones[a].dest_id;
                if (aid >= 0 && aid < sys->n_modules)
                    printf("%s ", sys->modules[aid].name);
            }
            printf("\n");
        }
    }
    printf("=======================================================\n");
}

/* ================================================================
 * REGISTRO DINAMICO DE HERRAMIENTAS
 * ================================================================ */

void orbital_load_tools(OrbitalSystem *sys, const char *path) {
    sys->n_tools = 0;
    FILE *f = fopen(path, "r");
    if (!f) return;
    char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = 0;
    fclose(f);
    char *p = buf;
    while ((p = strstr(p, "\"cualidad\"")) && sys->n_tools < MAX_TOOLS) {
        p += 10;
        while (*p && *p != '"') p++;
        if (*p != '"') continue;
        p++;
        int ni = 0;
        while (*p && *p != '"' && ni < MAX_NAME - 1)
            sys->tools[sys->n_tools].cualidad[ni++] = *p++;
        sys->tools[sys->n_tools].cualidad[ni] = 0;
        p = strstr(p, "\"comando\"");
        if (!p) continue;
        p += 9;
        while (*p && *p != '"') p++;
        if (*p != '"') continue;
        p++;
        ni = 0;
        while (*p && *p != '"' && ni < 511)
            sys->tools[sys->n_tools].comando[ni++] = *p++;
        sys->tools[sys->n_tools].comando[ni] = 0;
        sys->n_tools++;
    }
    if (sys->n_tools > 0)
        printf("[TOOLS] %d herramientas registradas desde %s\n", sys->n_tools, path);
}

/* ================================================================
 * PERSISTENCIA (JSON minimo)
 * ================================================================ */

void orbital_save(OrbitalSystem *sys, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) { fprintf(stderr, "error: no se pudo escribir %s\n", path); return; }

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": \"2.0_c\",\n");
    fprintf(f, "  \"t\": %d,\n", sys->t);
    fprintf(f, "  \"p\": %d,\n", sys->p);
    fprintf(f, "  \"n_modules\": %d,\n", sys->n_modules);
    if (strlen(sys->current_project) > 0)
        fprintf(f, "  \"current_project\": \"%s\",\n", sys->current_project);
    if (strlen(sys->project_type) > 0)
        fprintf(f, "  \"project_type\": \"%s\",\n", sys->project_type);
    fprintf(f, "  \"modules\": [\n");
    for (int i = 4; i < sys->n_modules; i++) {
        OrbitalModule *m = &sys->modules[i];
        if (m->source == 0 && m->tipo != TIPO_ACCION) continue;
        fprintf(f, "    {\"name\":\"%s\",\"source\":%d,\"tipo\":%d,\"tau\":%.4f,\"position\":%.4f,\"angle\":%.4f,\"usage\":%d,",
                m->name, m->source, m->tipo, m->tau, m->position, m->angle, m->usage_count);
        fprintf(f, "\"asoc\":[");
        for (int a = 0; a < m->num_asociaciones && a < MAX_ASOC; a++) {
            if (a > 0) fprintf(f, ",");
            fprintf(f, "{%d,%d,%.2f}", m->asociaciones[a].dest_id, m->asociaciones[a].tipo_asoc, m->asociaciones[a].peso);
        }
        fprintf(f, "],");
        fprintf(f, "\"properties\":[");
        for (int j = 0; j < m->properties.n && j < sys->p; j++) {
            if (j > 0) fprintf(f, ",");
            fprintf(f, "%.6f", m->properties.d[j]);
        }
        fprintf(f, "]},\n");
    }
    fprintf(f, "}\n");
    fclose(f);
    printf("  -> estado guardado en %s\n", path);
}

int orbital_load(OrbitalSystem *sys, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    /* parsing minimal: buscar "properties":[ ... ] y extraer learned modules */
    char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf)-1, f);
    buf[n] = 0;
    fclose(f);

    /* contar cuantos learned modules hay en el JSON */
    int count = 0;
    char *p = buf;
    while ((p = strstr(p, "\"source\":1")) != NULL) { count++; p++; }

    if (count == 0) return 1;

    /* restaurar learned modules */
    p = buf;
    for (int i = 0; i < count; i++) {
        /* buscar bloque de modulo */
        p = strstr(p, "\"name\":\"");
        if (!p) break;
        p += 8;
        char name[MAX_NAME]; int ni = 0;
        while (*p && *p != '\"' && ni < MAX_NAME-1) name[ni++] = *p++;
        name[ni] = 0;

        float tau = 0.3f, pos = 1.0f, angle = 0;
        int usage = 0, tipo = TIPO_CONOCIMIENTO;
        /* extraer campos */
        char *t = strstr(p, "\"tipo\":");
        if (t) sscanf(t+7, "%d", &tipo);
        t = strstr(p, "\"tau\":");
        if (t) sscanf(t+6, "%f", &tau);
        t = strstr(p, "\"position\":");
        if (t) sscanf(t+11, "%f", &pos);
        t = strstr(p, "\"angle\":");
        if (t) sscanf(t+7, "%f", &angle);
        t = strstr(p, "\"usage\":");
        if (t) sscanf(t+7, "%d", &usage);

        /* extraer asociaciones */
        t = strstr(p, "\"asoc\":[");
        int asoc_ids[MAX_ASOC] = {0};
        int asoc_tipos[MAX_ASOC] = {0};
        float asoc_pesos[MAX_ASOC] = {0};
        int nasoc = 0;
        if (t) {
            t += 8;
            while (*t && *t != ']' && nasoc < MAX_ASOC) {
                while (*t && *t != '{') t++;
                if (*t != '{') break;
                t++;
                asoc_ids[nasoc] = atoi(t);
                while (*t && *t != ',') t++;
                if (*t == ',') t++;
                asoc_tipos[nasoc] = atoi(t);
                while (*t && *t != ',') t++;
                if (*t == ',') t++;
                asoc_pesos[nasoc] = (float)atof(t);
                while (*t && *t != '}') t++;
                nasoc++;
            }
        }

        /* extraer properties */
        t = strstr(p, "\"properties\":[");
        if (!t) { p++; continue; }
        t += 14;
        float props[256];
        int np = 0;
        while (*t && *t != ']' && np < 256) {
            while (*t && (*t == ' ' || *t == '\n' || *t == '\r')) t++;
            if (*t == ']') break;
            props[np++] = (float)atof(t);
            while (*t && *t != ',' && *t != ']') t++;
            if (*t == ',') t++;
        }

        /* anadir modulo */
        if (np > 0) {
            int dup = 0;
            for (int j = 4; j < sys->n_modules; j++)
                if (strcmp(sys->modules[j].name, name) == 0) { dup = 1; break; }
            if (!dup) {
                int source = (tipo == TIPO_OBJETO || tipo == TIPO_CUALIDAD) ? 0 : 1;
                orbital_add_module(sys, name, props, source, tau);
                int mid = sys->n_modules - 1;
                sys->modules[mid].tipo = tipo;
                sys->modules[mid].position = pos;
                sys->modules[mid].angle = angle;
                sys->modules[mid].usage_count = usage;
                sys->modules[mid].num_asociaciones = nasoc;
                for (int a = 0; a < nasoc; a++) {
                    sys->modules[mid].asociaciones[a].dest_id = asoc_ids[a];
                    sys->modules[mid].asociaciones[a].tipo_asoc = asoc_tipos[a];
                    sys->modules[mid].asociaciones[a].peso = asoc_pesos[a];
                }
            }
        }
        p = t;
    }
    printf("  -> restaurados %d modulos aprendidos\n", count);

    /* restaurar contexto de proyecto */
    p = strstr(buf, "\"current_project\": \"");
    if (p) {
        p += 20;
        int ni = 0;
        while (*p && *p != '\"' && ni < MAX_PROJECT_NAME-1)
            sys->current_project[ni++] = *p++;
        sys->current_project[ni] = 0;
        p = strstr(p, "\"project_type\": \"");
        if (p) {
            p += 17;
            ni = 0;
            while (*p && *p != '\"' && ni < MAX_PROJECT_TYPE-1)
                sys->project_type[ni++] = *p++;
            sys->project_type[ni] = 0;
        }
        printf("  -> proyecto restaurado: %s (%s)\n",
               sys->current_project, sys->project_type);
    }
    return 1;
}

/* ================================================================
 * EJECUTOR DE ACCIONES (Windows)
 * ================================================================ */

void ejecutar_comando_raw(const char *cmd, int show_output) {
#ifdef _WIN32
    char buf[4096];
    snprintf(buf, sizeof(buf), "powershell -Command \"%s\"", cmd);
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (show_output) {
        printf("  -> ejecutando: %s\n", cmd);
        si.dwFlags = 0;
    } else {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    if (CreateProcessA(NULL, buf, NULL, NULL, FALSE,
                       show_output ? 0 : CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, show_output ? 10000 : 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (!show_output) printf("  -> comando ejecutado\n");
    } else {
        printf("  -> error al ejecutar (%lu)\n", GetLastError());
    }
#else
    printf("  -> (simulado) %s\n", cmd);
    (void)show_output;
#endif
}

void ejecutar_comando(const char *cmd) {
    ejecutar_comando_raw(cmd, 1);
}

/* ================================================================
 * SISTEMA DE PROYECTOS
 * ================================================================ */

int crear_proyecto(OrbitalSystem *sys, const char *tipo, const char *nombre) {
    /* buscar tipo en plantillas */
    const ProjectTemplate *pt = NULL;
    for (int i = 0; project_types[i].type != NULL; i++) {
        if (strcmp(project_types[i].type, tipo) == 0) { pt = &project_types[i]; break; }
    }
    if (!pt) {
        printf("[PROY] error: tipo '%s' no reconocido\n", tipo);
        return 0;
    }
    /* crear directorio raiz */
    char root[512];
    snprintf(root, sizeof(root), "%s", nombre);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "if (-not (Test-Path \"%s\")) { New-Item -ItemType Directory -Path \"%s\" -Force }", root, root);
    ejecutar_comando_raw(cmd, 0);
    /* crear subdirectorios de la plantilla */
    for (int i = 0; pt->folders[i] != NULL; i++) {
        char sub[512];
        snprintf(sub, sizeof(sub), "%s\\%s", root, pt->folders[i]);
        snprintf(cmd, sizeof(cmd), "New-Item -ItemType Directory -Path \"%s\" -Force", sub);
        ejecutar_comando_raw(cmd, 0);
    }
    /* registrar en el sistema */
    strncpy(sys->current_project, nombre, MAX_PROJECT_NAME-1);
    strncpy(sys->project_type, tipo, MAX_PROJECT_TYPE-1);
    int nf = 0;
    while (pt->folders[nf] != NULL) nf++;
    printf("[PROY] proyecto '%s' (%s) creado con %d carpetas\n", nombre, tipo, nf);
    /* crear modulo de proyecto en el sistema */
    Vec pv = orbital_encode_text(sys, nombre);
    float *props = (float*)malloc(sys->p * sizeof(float));
    for (int j = 0; j < sys->p; j++) props[j] = pv.d[j];
    orbital_add_module(sys, nombre, props, 1, 0.15f);
    sys->modules[sys->n_modules-1].tipo = TIPO_OBJETO;
    free(props);
    vec_free(&pv);
    return 1;
}

/* ================================================================
 * COLA DE TAREAS
 * ================================================================ */

void orbital_list_tasks(OrbitalSystem *sys) {
    if (sys->n_tasks == 0) { printf("[TAREAS] cola vacia\n"); return; }
    printf("[TAREAS] %d pendientes:\n", sys->n_tasks);
    for (int i = 0; i < sys->n_tasks; i++) {
        printf("  %d. [%s] %s\n", i+1,
               sys->task_queue[i].done ? "HECHA" : "PEND",
               sys->task_queue[i].desc);
    }
}

void orbital_add_task(OrbitalSystem *sys, const char *desc) {
    if (sys->n_tasks >= MAX_TASKS) { printf("[TAREAS] cola llena\n"); return; }
    strncpy(sys->task_queue[sys->n_tasks].desc, desc, MAX_TASK_DESC-1);
    sys->task_queue[sys->n_tasks].done = 0;
    sys->n_tasks++;
    printf("[TAREAS] tarea %d anadida: %s\n", sys->n_tasks, desc);
}

int orbital_next_task(OrbitalSystem *sys) {
    for (int i = 0; i < sys->n_tasks; i++) {
        if (!sys->task_queue[i].done) {
            sys->task_queue[i].done = 1;
            printf("[TAREAS] ejecutando: %s\n", sys->task_queue[i].desc);
            return 1;
        }
    }
    printf("[TAREAS] no hay tareas pendientes\n");
    return 0;
}

void orbital_clear_tasks(OrbitalSystem *sys) {
    sys->n_tasks = 0;
    printf("[TAREAS] cola vaciada\n");
}

/* ================================================================
 * AGENTE DE SISTEMA DE ARCHIVOS
 * ================================================================ */

void orbital_list_directory(const char *path) {
    char cmd[4096];
    if (path && strlen(path) > 0) {
        snprintf(cmd, sizeof(cmd), "Get-ChildItem -LiteralPath \"%s\" | Select-Object Mode,Length,Name", path);
    } else {
        snprintf(cmd, sizeof(cmd), "Get-ChildItem | Select-Object Mode,Length,Name");
    }
    ejecutar_comando_raw(cmd, 1);
}

/* ================================================================
 * MAIN
 * ================================================================ */

int mmia_run_interactive(void) {
    srand((unsigned int)time(NULL));
    const char *save_path = "mmia_memory.json";

    printf("\n=======================================================\n");
    printf("  MMIA 2.0 — Subconsciente Numerico + Orbital\n");
    printf("  Version C  |  dim=%d  theta=%.2f\n", DIM, THETA);
    printf("=======================================================\n\n");

    OrbitalSystem sys;
    orbital_init(&sys, DIM, THETA, DT);
    orbital_load(&sys, save_path);

    printf("Escribe una tarea, o 'salir' para terminar.\n");
    printf("Comandos: !cmd <comando>  !save  !load  !status\n");
    printf("         !queue <tarea>  !tasks  !run  !clear\n");
    printf("         !ls [path]  !cd <dir>  !pwd  !proyecto\n\n");

    char input[1024];
    while (1) {
        printf("\n» ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        /* Pipeline: separar por ; y procesar cada segmento */
        int n_segments = 0;
        char *segments[16];
        char *save_p = NULL;
        char *seg = strtok_r(input, ";", &save_p);
        while (seg && n_segments < 16) {
            while (*seg == ' ') seg++;
            char *end = seg + strlen(seg) - 1;
            while (end > seg && *end == ' ') *end-- = 0;
            segments[n_segments++] = seg;
            seg = strtok_r(NULL, ";", &save_p);
        }
        if (n_segments == 0) continue;

        int should_exit = 0;
        for (int s = 0; s < n_segments && !should_exit; s++) {
            char *cmd = segments[s];

        if (strcmp(cmd, "salir") == 0 || strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) { should_exit = 1; break; }

        if (strncmp(cmd, "!cmd ", 5) == 0) {
            ejecutar_comando(cmd + 5);
            continue;
        }
        if (strcmp(cmd, "!save") == 0) {
            orbital_save(&sys, save_path);
            continue;
        }
        if (strcmp(cmd, "!load") == 0) {
            orbital_load(&sys, save_path);
            continue;
        }
        if (strcmp(cmd, "!status") == 0) {
            orbital_visualize(&sys);
            continue;
        }
        if (strcmp(cmd, "!tasks") == 0 || strcmp(cmd, "!tareas") == 0) {
            orbital_list_tasks(&sys);
            continue;
        }
        if (strncmp(cmd, "!queue ", 7) == 0 || strncmp(cmd, "!tarea ", 7) == 0) {
            orbital_add_task(&sys, cmd + 7);
            continue;
        }
        if (strcmp(cmd, "!run") == 0 || strcmp(cmd, "!next") == 0) {
            orbital_next_task(&sys);
            continue;
        }
        if (strcmp(cmd, "!clear") == 0) {
            orbital_clear_tasks(&sys);
            continue;
        }
        if (strncmp(cmd, "!ls", 3) == 0) {
            char *p = cmd + 3;
            while (*p == ' ') p++;
            orbital_list_directory(strlen(p) > 0 ? p : NULL);
            continue;
        }
        if (strncmp(cmd, "!cd ", 4) == 0) {
#ifdef _WIN32
            SetCurrentDirectoryA(cmd + 4);
#endif
            printf("[FS] directorio cambiado a: ");
            ejecutar_comando_raw("Get-Location", 1);
            continue;
        }
        if (strcmp(cmd, "!pwd") == 0) {
            ejecutar_comando_raw("Get-Location", 1);
            continue;
        }
        if (strcmp(cmd, "!proyecto") == 0 || strcmp(cmd, "!project") == 0) {
            if (strlen(sys.current_project) > 0) {
                printf("[PROY] proyecto activo: %s (tipo: %s)\n",
                       sys.current_project, sys.project_type);
            } else {
                printf("[PROY] no hay proyecto activo\n");
                printf("  usar: crear proyecto <tipo> <nombre>\n");
            }
            continue;
        }

        orbital_process(&sys, cmd);
        if (s == n_segments - 1) orbital_visualize(&sys);
        else printf("  -> segmento %d/%d completado\n", s + 1, n_segments);
    }
    if (should_exit) break;
    }

    printf("\nGuardando estado...\n");
    orbital_save(&sys, save_path);
    orbital_free(&sys);
    printf("Hecho.\n");
    return 0;
}

int mmia_process_direct(const char *input_text) {
    srand((unsigned int)time(NULL));
    const char *save_path = "mmia_memory.json";

    OrbitalSystem sys;
    orbital_init(&sys, DIM, THETA, DT);
    orbital_load(&sys, save_path);

    printf("TAREA: %s\n\n", input_text);
    orbital_process(&sys, input_text);
    orbital_visualize(&sys);
    orbital_save(&sys, save_path);
    orbital_free(&sys);
    return 0;
}

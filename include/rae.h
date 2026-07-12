#ifndef BDGB_RAE_H
#define BDGB_RAE_H

#include <stdint.h>

/* --- Categorias gramaticales RAE (como concept IDs) --- */
#define CAT_SUSTANTIVO  5000
#define CAT_VERBO       5001
#define CAT_ADJETIVO    5002
#define CAT_ADVERBIO    5003
#define CAT_ARTICULO    5004
#define CAT_PREPOSICION 5005
#define CAT_PRONOMBRE   5006

/* --- Conjuntos semanticos (como concept IDs) --- */
#define SET_PERSONAJE       6000
#define SET_ESCENA          6001
#define SET_ACCION_DIBUJO   6002
#define SET_ACCION_ARCHIVO  6003
#define SET_HERRAMIENTA     6004
#define SET_LUGAR           6005
#define SET_TIEMPO          6006
#define SET_CUALIDAD        6007
#define SET_PROYECTO        6008
#define SET_OBJETO          6009
#define SET_ACCION_GIT      6010
#define SET_ACCION_PROYECTO 6011
#define SET_VOCABLO_BASE    6012
#define SET_FAMILIA         6013
#define SET_ANIMAL          6014
#define SET_NATURALEZA      6015
#define SET_COMIDA          6016
#define SET_CUERPO          6017
#define SET_HOGAR           6018
#define SET_ROPA            6019
#define SET_CIUDAD          6020
#define SET_EMOCION         6021
#define SET_CONJUNCION      6022
#define SET_INTERROGATIVO   6023
#define SET_NUMERAL         6024
#define SET_DESCONOCIDO     6025

/* --- Estructura de frase analizada --- */
typedef struct {
    int  actante_id;      /* concept_id del sujeto */
    int  accion_id;       /* concept_id del verbo */
    int  paciente_id;     /* concept_id del objeto */
    int  contexto_id;     /* concept_id del contexto/lugar */
    char actante[64];
    char accion[64];
    char paciente[64];
    char contexto[64];
    char intencion[32];
    char pregunta[32];
    char raw[512];
    int  confianza;       /* 0-255 */
} FraseAnalizada;

/* --- Inicializacion del sistema RAE --- */
int  rae_init(const char *data_path);

/* --- Analisis semantico de texto --- */
int  rae_analizar(const char *texto, FraseAnalizada *out);

/* --- Aprendizaje de vocabulario --- */
int  rae_aprender_palabra(const char *palabra, uint16_t concepto_id, uint16_t categoria);
int  rae_aprender_de_texto(const char *texto);
int  rae_categorizar(const char *palabra, uint16_t *concepto_id, uint16_t *categoria);
int  rae_categorizar_multi(const char *palabra, uint16_t *conceptos,
                           uint16_t *categorias, int max_out);

/* --- Teoria de conjuntos --- */
int  rae_pertenece_a_set(uint16_t concepto_id, uint16_t set_id);
int  rae_agregar_a_set(uint16_t concepto_id, uint16_t set_id);
int  rae_obtener_miembros(uint16_t set_id, uint16_t *out, int max_out);
int  rae_listar_sets(uint16_t concepto_id, uint16_t *out, int max_out);

/* --- Utilidad --- */
void rae_imprimir_frase(const FraseAnalizada *f);
int  rae_cargar_diccionario(const char *archivo);
int  rae_salvar_diccionario(void);

#endif

#ifndef BDGB_NLP_H
#define BDGB_NLP_H

#include <stdint.h>
#include "search.h"

#define MAX_TOKENS 64
#define MAX_TERMS  2048

#define NLP_TERMS_FILE "nlp_terms.dat"

typedef struct {
    uint16_t concept_id;
    int (*predicate)(const bdgb_props_t *);
    int dynamic_filter;
    int geom_type_filter;
} NLPQueryElement;

int  nlp_init(void);
int  nlp_parse(const char *text, NLPQueryElement *out, int max_out);
int  nlp_search(const char *text, SearchResult *out, int max_out);
void nlp_add_term(const char *word, uint16_t concept_id,
                  int (*pred)(const bdgb_props_t *),
                  int dyn_filter, int geom_filter);
void nlp_set_data_path(const char *path);
int  nlp_lookup_all(const char *word, uint16_t *out_ids, int max_out);
int  nlp_save(void);
int  nlp_load(void);
int  nlp_learn_from_text(const char *text, int max_terms);
int  nlp_term_count(void);
void nlp_list_terms(void);

#endif

#ifndef BDGB_NLP_H
#define BDGB_NLP_H

#include <stdint.h>
#include "search.h"

#define MAX_TOKENS 16
#define MAX_TERMS  32

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

#endif

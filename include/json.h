#ifndef BDGB_JSON_H
#define BDGB_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsonValue JsonValue;
typedef struct JsonPair {
    char *key;
    JsonValue *value;
} JsonPair;

struct JsonValue {
    int type;           /* 0=string 1=number 2=object 3=array 4=true 5=false 6=null */
    char *string;       /* type 0 */
    double number;      /* type 1 */
    JsonPair *pairs;    /* type 2 */
    JsonValue **items;  /* type 3 */
    int count;          /* pairs (object) or items (array) count */
};

/* Parse JSON string into tree. Returns NULL on parse error. */
JsonValue *json_parse(const char *input);

/* Parse JSON file contents into tree. Reads entire file. */
JsonValue *json_parse_file(const char *path);

/* Free entire tree recursively. */
void json_free(JsonValue *v);

/* Accessors — return NULL / 0 on type mismatch or missing. */
JsonValue *json_get(JsonValue *obj, const char *key);
const char *json_string(JsonValue *v);
double json_number(JsonValue *v);
int json_len(JsonValue *arr);         /* array length, 0 if not array */
JsonValue *json_idx(JsonValue *arr, int i); /* array index, NULL if OOB */

/* Dot-path lookup: "config.pipeline" returns pipeline array. */
JsonValue *json_path(JsonValue *root, const char *path);

#ifdef __cplusplus
}
#endif

#endif
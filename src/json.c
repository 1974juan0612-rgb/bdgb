#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* ---- internal helpers ---- */

static void skip_ws(const char *s, int *pos) {
    while (s[*pos] == ' ' || s[*pos] == '\t' || s[*pos] == '\n' || s[*pos] == '\r')
        (*pos)++;
}

static JsonValue *alloc_val(void) {
    JsonValue *v = (JsonValue*)malloc(sizeof(JsonValue));
    if (v) memset(v, 0, sizeof(JsonValue));
    return v;
}

static char *dup_str(const char *s, int len) {
    char *d = (char*)malloc((size_t)len + 1);
    if (!d) return NULL;
    memcpy(d, s, (size_t)len);
    d[len] = 0;
    return d;
}

/* forward */
static JsonValue *parse_value(const char *s, int *pos);

/* ---- string parser with full escape support ---- */

static char *parse_string(const char *s, int *pos) {
    if (s[*pos] != '"') return NULL;
    (*pos)++; /* skip opening quote */

    int start = *pos;
    int len = 0;
    int escapes = 0;

    /* first pass: measure */
    while (s[*pos] && s[*pos] != '"') {
        if (s[*pos] == '\\') {
            (*pos)++;
            escapes++;
            if (!s[*pos]) return NULL;
        }
        (*pos)++;
        len++;
    }
    if (!s[*pos]) return NULL;
    (*pos)++; /* skip closing quote */

    /* allocate */
    char *result = (char*)malloc((size_t)len - escapes + 1);
    if (!result) return NULL;

    int src = start;
    int dst = 0;
    for (int i = 0; i < len; i++) {
        if (s[src + i] == '\\') {
            i++;
            switch (s[src + i]) {
                case '"':  result[dst++] = '"';  break;
                case '\\': result[dst++] = '\\'; break;
                case '/':  result[dst++] = '/';  break;
                case 'b':  result[dst++] = '\b'; break;
                case 'f':  result[dst++] = '\f'; break;
                case 'n':  result[dst++] = '\n'; break;
                case 'r':  result[dst++] = '\r'; break;
                case 't':  result[dst++] = '\t'; break;
                case 'u': {
                    /* \uXXXX — skip 4 hex, store replacement char */
                    i += 4;
                    result[dst++] = '?';
                    break;
                }
                default:   result[dst++] = s[src + i]; break;
            }
        } else {
            result[dst++] = s[src + i];
        }
    }
    result[dst] = 0;
    return result;
}

/* ---- number parser ---- */

static JsonValue *parse_number(const char *s, int *pos) {
    int start = *pos;
    if (s[*pos] == '-') (*pos)++;
    while (isdigit((unsigned char)s[*pos])) (*pos)++;
    int is_float = 0;
    if (s[*pos] == '.') { is_float = 1; (*pos)++;
        while (isdigit((unsigned char)s[*pos])) (*pos)++; }
    if (s[*pos] == 'e' || s[*pos] == 'E') { is_float = 1; (*pos)++;
        if (s[*pos] == '+' || s[*pos] == '-') (*pos)++;
        while (isdigit((unsigned char)s[*pos])) (*pos)++; }

    int len = *pos - start;
    char buf[64];
    if (len >= 64) len = 63;
    strncpy(buf, s + start, (size_t)len);
    buf[len] = 0;

    JsonValue *v = alloc_val();
    if (!v) return NULL;
    v->type = 1;
    v->number = is_float ? atof(buf) : (double)atoll(buf);
    return v;
}

/* ---- value dispatcher ---- */

static JsonValue *parse_value(const char *s, int *pos) {
    skip_ws(s, pos);
    if (!s[*pos]) return NULL;

    switch (s[*pos]) {
        case '"': {
            char *str = parse_string(s, pos);
            if (!str) return NULL;
            JsonValue *v = alloc_val();
            if (!v) { free(str); return NULL; }
            v->type = 0;
            v->string = str;
            return v;
        }
        case '{': {
            (*pos)++; /* skip { */
            JsonValue *v = alloc_val();
            if (!v) return NULL;
            v->type = 2;

            int cap = 8;
            v->pairs = (JsonPair*)malloc((size_t)cap * sizeof(JsonPair));
            if (!v->pairs) { free(v); return NULL; }
            v->count = 0;

            skip_ws(s, pos);
            if (s[*pos] == '}') { (*pos)++; return v; }

            while (1) {
                skip_ws(s, pos);
                if (s[*pos] != '"') break;
                char *key = parse_string(s, pos);
                if (!key) break;
                skip_ws(s, pos);
                if (s[*pos] != ':') { free(key); break; }
                (*pos)++;
                JsonValue *val = parse_value(s, pos);
                if (!val) { free(key); break; }

                if (v->count >= cap) {
                    cap *= 2;
                    JsonPair *np = (JsonPair*)realloc(v->pairs, (size_t)cap * sizeof(JsonPair));
                    if (!np) { free(key); json_free(val); break; }
                    v->pairs = np;
                }
                v->pairs[v->count].key = key;
                v->pairs[v->count].value = val;
                v->count++;

                skip_ws(s, pos);
                if (s[*pos] == '}') { (*pos)++; return v; }
                if (s[*pos] != ',') break;
                (*pos)++;
            }

            /* parse error — free partial and return NULL */
            json_free(v);
            return NULL;
        }
        case '[': {
            (*pos)++; /* skip [ */
            JsonValue *v = alloc_val();
            if (!v) return NULL;
            v->type = 3;

            int cap = 8;
            v->items = (JsonValue**)malloc((size_t)cap * sizeof(JsonValue*));
            if (!v->items) { free(v); return NULL; }
            v->count = 0;

            skip_ws(s, pos);
            if (s[*pos] == ']') { (*pos)++; return v; }

            while (1) {
                JsonValue *item = parse_value(s, pos);
                if (!item) break;

                if (v->count >= cap) {
                    cap *= 2;
                    JsonValue **ni = (JsonValue**)realloc(v->items, (size_t)cap * sizeof(JsonValue*));
                    if (!ni) { json_free(item); break; }
                    v->items = ni;
                }
                v->items[v->count++] = item;

                skip_ws(s, pos);
                if (s[*pos] == ']') { (*pos)++; return v; }
                if (s[*pos] != ',') break;
                (*pos)++;
            }

            json_free(v);
            return NULL;
        }
        case 't':
            if (strncmp(s + *pos, "true", 4) == 0) {
                *pos += 4;
                JsonValue *v = alloc_val();
                if (v) { v->type = 4; v->number = 1.0; }
                return v;
            }
            return NULL;
        case 'f':
            if (strncmp(s + *pos, "false", 5) == 0) {
                *pos += 5;
                JsonValue *v = alloc_val();
                if (v) { v->type = 5; v->number = 0.0; }
                return v;
            }
            return NULL;
        case 'n':
            if (strncmp(s + *pos, "null", 4) == 0) {
                *pos += 4;
                JsonValue *v = alloc_val();
                if (v) v->type = 6;
                return v;
            }
            return NULL;
        default:
            if (s[*pos] == '-' || isdigit((unsigned char)s[*pos]))
                return parse_number(s, pos);
            return NULL;
    }
}

/* ---- public API ---- */

JsonValue *json_parse(const char *input) {
    if (!input) return NULL;
    int pos = 0;
    JsonValue *v = parse_value(input, &pos);
    if (v) {
        skip_ws(input, &pos);
        /* trailing non-whitespace means incomplete parse */
        if (input[pos]) { json_free(v); return NULL; }
    }
    return v;
}

JsonValue *json_parse_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = 0;
    JsonValue *v = json_parse(buf);
    free(buf);
    return v;
}

void json_free(JsonValue *v) {
    if (!v) return;
    free(v->string);
    if (v->type == 2) {
        for (int i = 0; i < v->count; i++) {
            free(v->pairs[i].key);
            json_free(v->pairs[i].value);
        }
        free(v->pairs);
    }
    if (v->type == 3) {
        for (int i = 0; i < v->count; i++)
            json_free(v->items[i]);
        free(v->items);
    }
    free(v);
}

JsonValue *json_get(JsonValue *v, const char *key) {
    if (!v || v->type != 2) return NULL;
    for (int i = 0; i < v->count; i++)
        if (strcmp(v->pairs[i].key, key) == 0)
            return v->pairs[i].value;
    return NULL;
}

const char *json_string(JsonValue *v) {
    if (!v || v->type != 0) return NULL;
    return v->string;
}

double json_number(JsonValue *v) {
    if (!v || v->type != 1) return 0.0;
    return v->number;
}

int json_len(JsonValue *arr) {
    if (!arr || arr->type != 3) return 0;
    return arr->count;
}

JsonValue *json_idx(JsonValue *arr, int i) {
    if (!arr || arr->type != 3 || i < 0 || i >= arr->count) return NULL;
    return arr->items[i];
}

JsonValue *json_path(JsonValue *root, const char *path) {
    if (!root || !path || !*path) return root;

    char buf[256];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    JsonValue *cur = root;
    char *tok = buf;
    char *dot;

    while ((dot = strchr(tok, '.')) != NULL) {
        *dot = 0;
        cur = json_get(cur, tok);
        if (!cur) return NULL;
        tok = dot + 1;
    }

    return json_get(cur, tok);
}
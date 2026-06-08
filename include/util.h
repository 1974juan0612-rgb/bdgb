#ifndef BDGB_UTIL_H
#define BDGB_UTIL_H

#include <stddef.h>

/* Ejecuta comando, captura stdout. Returns 0 on success. */
int run_captured(const char *cmd, char *out, size_t outsz);

/* Crea directorio si no existe (portatil Windows/POSIX) */
void ensure_dir(const char *path);

#endif

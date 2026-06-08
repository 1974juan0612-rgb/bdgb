#include "util.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

int run_captured(const char *cmd, char *out, size_t outsz) {
    FILE *fp;
#ifdef _WIN32
    fp = _popen(cmd, "r");
#else
    fp = popen(cmd, "r");
#endif
    if (!fp) return -1;
    size_t pos = 0;
    while (pos < outsz - 1) {
        int c = fgetc(fp);
        if (c == EOF) break;
        out[pos++] = (char)c;
    }
    out[pos] = 0;
#ifdef _WIN32
    return _pclose(fp);
#else
    return pclose(fp);
#endif
}

void ensure_dir(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

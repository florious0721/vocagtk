#ifndef _VOCAGTK_HELPER_H
#define _VOCAGTK_HELPER_H
#include <curl/curl.h>
#include <sqlite3.h>
#include <stdlib.h>

typedef struct {
    CURL *handle;
    sqlite3 *db;
} AppState;

#define DEBUG(...) vocagtk_log("DEBUG", G_LOG_LEVEL_WARNING, __VA_ARGS__)
#define sqlite3_column_str(...) \
    ((char const *) sqlite3_column_text(__VA_ARGS__))

static inline void free_signal_wrapper(void *_, void *data) {
    free(data);
}
#endif

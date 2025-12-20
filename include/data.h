#include <curl/curl.h>
#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>
#include <yyjson.h>

typedef enum {
    VOCAGTK_OK = 0,
    VOCAGTK_DB_ERR,
    VOCAGTK_DB_NOT_FOUND,
    VOCAGTK_NETWORK_ERR,
} VocagtkErr;

typedef struct {
    CURL *handle;
    char const *cache_path;
} VocagtkDownloader;

typedef struct {
    char const *query;
    int entry_type; // -1 means anything, or follow VocagtkEntryTypeLabel
} VocagtkSearchQuery;

typedef struct {
    yyjson_doc *doc;
    yyjson_arr_iter iter;
} VocagtkSearchResultIterator;

static inline size_t curl_g_byte_array_writer(
    void *src, size_t size, size_t n,
    GByteArray *dest
) {
    size_t old_len = dest->len;
    g_byte_array_append(dest, src, size * n);
    return dest->len - old_len;
}

void vocagtk_downloader_search(
    VocagtkDownloader *dl,
    VocagtkSearchQuery *query,
    VocagtkSearchResultIterator *iter // out
);

void vocagtk_get_search_entries(
    VocagtkSearchResultIterator *iter,
    GListStore *list, sqlite3 *db, int *sql_err
);

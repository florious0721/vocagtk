#ifndef _VOCAGTK_DL_H
#define _VOCAGTK_DL_H

#include <curl/curl.h>
#include <gio/gio.h>
#include <glib.h>
#include <sqlite3.h>
#include <yyjson.h>

#define VOCAGTK_DOWNLOADER_PAGE_SIZE (0x20)

typedef struct {
    CURL *handle;
    char const *cache_path;
} VocagtkDownloader;

typedef struct {
    char const *query;
    char const *entry_type; // -1 means anything, or follow VocagtkEntryTypeLabel
} VocagtkSearchQuery;

typedef struct {
    yyjson_doc *doc;
    yyjson_arr_iter iter;
    GString *url;
    size_t start; // begin with 0
    size_t page_size;
    size_t start_offset_in_url;
    time_t last_update_at;
} VocagtkResultIterator;

void vocagtk_downloader_search(
    VocagtkSearchQuery const *query, // in
    VocagtkResultIterator *iter // out
);

void vocagtk_downloader_update(
    int artist_id, time_t last_update_at,
    VocagtkResultIterator *iter // out
);

// Fetch a single entry JSON document from VocaDB.
// Returns NULL on network/parse error.
yyjson_doc *vocagtk_downloader_album(VocagtkDownloader *dl, int id);
yyjson_doc *vocagtk_downloader_artist(VocagtkDownloader *dl, int id);
yyjson_doc *vocagtk_downloader_song(VocagtkDownloader *dl, int id);

yyjson_val *vocagtk_result_iterator_next(
    VocagtkResultIterator *iter,
    VocagtkDownloader *dl, CURLcode *err
);

// Download image from URL and save to specified path.
// Returns CURLE_OK on success.
CURLcode vocagtk_downloader_image(
    VocagtkDownloader *dl,
    char const *url,
    char const *out_path
);

#endif

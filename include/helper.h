#ifndef _VOCAGTK_HELPER_H
#define _VOCAGTK_HELPER_H
#include <curl/curl.h>
#include <gio/gio.h>
#include <glib-2.0/glib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdlib.h>

#include "dl.h"

typedef struct {
    VocagtkDownloader dl; // before app activates
    sqlite3 *db; // before app activates
    struct {
        GtkEntry *field;
        GtkDropDown *type_selector;
        GListStore *list;
    } search_widgets; // on build search
    GListStore *rss_artist; // on build rss
    GListStore *rss_song; // on build rss
    GtkStringList *songlists; // before app activates
    GtkDropDown *songlist_select; // on build songlist ?
    GListStore *current_songlist; // on build songlist
    int current_songlist_id; // on songlist change?
} AppState;

#define DEBUG(...) vocagtk_log("DEBUG", G_LOG_LEVEL_WARNING, __VA_ARGS__)
#define sqlite3_column_str(...) \
    ((char const *) sqlite3_column_text(__VA_ARGS__))

static inline void free_signal_wrapper(void *_, void *data) {
    free(data);
}

static inline size_t curl_g_byte_array_writer(
    void *src, size_t size, size_t n,
    GByteArray *dest
) {
    size_t old_len = dest->len;
    g_byte_array_append(dest, src, size * n);
    return dest->len - old_len;
}
#endif

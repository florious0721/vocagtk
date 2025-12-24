#ifndef _VOCAGTK_HELPER_H
#define _VOCAGTK_HELPER_H
#include <curl/curl.h>
#include <gio/gio.h>
#include <glib-2.0/glib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>

#include "exterr.h"
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

// Parse ISO 8601 datetime string (e.g., "2025-12-23T12:49:23.070Z") to time_t
static inline time_t parse_iso8601_datetime(char const *datetime_str) {
    if (!datetime_str) return -1;

    struct tm tm = {0};
    // Parse format: YYYY-MM-DDTHH:MM:SS (ignore milliseconds and timezone)
    int parsed = sscanf(datetime_str, "%d-%d-%dT%d:%d:%d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

    if (parsed < 6) {
        DEBUG("Failed to parse datetime: %s", datetime_str);
        return -1;
    }

    // Adjust tm structure
    tm.tm_year -= 1900;  // tm_year is years since 1900
    tm.tm_mon -= 1;      // tm_mon is 0-11
    tm.tm_isdst = -1;    // Let mktime determine DST

    // Convert to time_t (assumes UTC, as the string has 'Z' suffix)
    time_t result = timegm(&tm);
    if (result == -1) {
        DEBUG("timegm failed for datetime: %s", datetime_str);
    }

    return result;
}

#endif

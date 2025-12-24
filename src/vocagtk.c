#include <curl/curl.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <yyjson.h>

#include "db.h"
#include "entrybox.h"
#include "exterr.h"
#include "ui.h"

/*
 * SELECT s.*
 * FROM song s
 * JOIN song_in_playlist sip ON s.id = sip.song_id
 * WHERE sip.playlist_id = ?
 * ORDER BY s.title ASC;
 */

typedef struct {
    AppState *app;
    GtkEntry *field;
    GtkDropDown *type_selector;
    GListStore *results;
} SearchState;

static void activate(GtkApplication *app, AppState *ctx) {
    GtkBuilder *builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/main.ui"
    );

    GObject *window = gtk_builder_get_object(builder, "main_window");

    build_home(builder, ctx);
    build_search(builder, ctx);
    build_rss_artist_box(builder, ctx);
    build_rss_song_box(builder, ctx);
    build_playlist(builder, ctx);

    gtk_window_set_application(GTK_WINDOW(window), app);
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);

    g_object_unref(builder);
}

/*!
 * @brief
 *   Initialize the classes defined by us,
 *   call it before the application constructed
 */
static void init_classes(void) {
    VOCAGTK_TYPE_ALBUM;
    VOCAGTK_TYPE_ARTIST;
    VOCAGTK_TYPE_SONG;
    VOCAGTK_TYPE_ENTRY;
    VOCAGTK_TYPE_ENTRY_BOX;
}

static sqlite3 *init_database(void) {
    // TODO: database version control and migration support
    sqlite3 *r = NULL;
    int err = sqlite3_open("voca.db", &r);
    if (err != SQLITE_OK) return NULL;

    char const *sqls[] = {
        "PRAGMA foreign_keys = ON;",

        "CREATE TABLE IF NOT EXISTS album("
        "id INTEGER PRIMARY KEY,"
        "title TEXT, artist TEXT, cover_url TEXT,"
        "publish_date INTEGER"
        ");",

        "CREATE TABLE IF NOT EXISTS song("
        "id INTEGER PRIMARY KEY,"
        "title TEXT, artist TEXT, image_url TEXT,"
        "publish_date INTEGER"
        ");",

        "CREATE TABLE IF NOT EXISTS artist("
        "id INTEGER PRIMARY KEY,"
        "name TEXT, avatar_url TEXT, update_at INTEGER DEFAULT 0"
        ");",

        "CREATE TABLE IF NOT EXISTS song_in_album("
        "song_id INTEGER, album_id INTEGER,"
        "PRIMARY KEY(song_id, album_id),"
        "FOREIGN KEY (song_id) REFERENCES song(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (album_id) REFERENCES album(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"
        ");",

        "CREATE TABLE IF NOT EXISTS artist_for_song("
        "song_id INTEGER, artist_id INTEGER,"
        "PRIMARY KEY(song_id, artist_id),"
        "FOREIGN KEY (song_id) REFERENCES song(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (artist_id) REFERENCES artist(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"
        ");",

        /*"CREATE TABLE IF NOT EXISTS artist_for_album("
           "id INTEGER PRIMARY KEY,"
           "album_id INTEGER, artist_id INTEGER,"
           "FOREIGN KEY (album_id) REFERENCES album(id)"
           "ON UPDATE CASCADE ON DELETE RESTRICT,"
           "FOREIGN KEY (artist_id) REFERENCES artist(id)"
           "ON UPDATE CASCADE ON DELETE RESTRICT"
           ");",*/

        "CREATE TABLE IF NOT EXISTS rss("
        "artist_id INTEGER PRIMARY KEY,"
        "FOREIGN KEY (artist_id) REFERENCES artist(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"
        /*"FOREIGN KEY (artist_name) REFERENCES artist(name)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"*/
        ");",

        "CREATE TABLE IF NOT EXISTS playlist("
        "name TEXT PRIMARY KEY"
        ");",

        "CREATE TABLE IF NOT EXISTS song_in_playlist("
        "song_id INTEGER, playlist_name TEXT,"
        "PRIMARY KEY(song_id, playlist_name),"
        "FOREIGN KEY (song_id) REFERENCES song(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (playlist_name) REFERENCES playlist(name)"
        "ON UPDATE CASCADE ON DELETE CASCADE"
        ");",

        //"INSERT OR IGNORE INTO playlist(name) VALUES('Default');",
    };

    char *errmsg;
    for (int i = 0; i < 9; ++i) {
        if (sqlite3_exec(r, sqls[i], NULL, NULL, &errmsg) != SQLITE_OK) {
            vocagtk_warn_sql("%s", errmsg);
        }
    }

    return r;
}

int main(int argc, char **argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    init_classes();

    int status = 0;

    AppState state = {0};
    GtkApplication *app = NULL;
    struct curl_slist *headers = NULL;

    g_log_set_handler(
        NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL,
        g_log_default_handler, NULL
    );

    state.db = init_database();
    if (!state.db) {
        status = 1;
        goto clean;
    }

    state.dl.cache_path = "./cache/";
    state.dl.handle = curl_easy_init();
    if (!state.dl.handle) {
        status = 1;
        goto clean;
    }

    state.playlists = gtk_string_list_new(NULL);

    // Load playlist names from database into playlists
    sqlite3_stmt *stmt;
    if (db_playlist_get_all(state.db, &stmt) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            char const *name = sqlite3_column_str(stmt, 0);
            gtk_string_list_append(state.playlists, name);
        }
        sqlite3_finalize(stmt);
    }


    /*headers = curl_slist_append(
        headers,
        "Cookie: cf_clearance=2KUVcv8AGzrgktzUK28bbHmv_t.l1gvI5GjnYOu9bng-1766191196-1.2.1.1-F68vdmbpw4uxsowg5vkWvVhWEy16UlfjnbkS3.9Wkzc6VoeKnaRiit5OdFjSnflzBV4PKvkvQCoZV3QfOzd7CX48v3rdOHYVIY2wUahyxcDHtq6JdIY.WlF.OZc4oCfblKcOB5.bDAjq1khyfIp4DdYcSbY5aS1sJk8w3s0X97qE03nBChH_XYkVHgi4cwRyBGCDL8x3B2aJt8GmzVixM3XFwyWPAW8NQ41NE8958vU"
    );
    headers = curl_slist_append(
        headers,
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:144.0) Gecko/20100101 Firefox/144.0"
    );
    curl_easy_setopt(state.dl.handle, CURLOPT_HTTPHEADER, headers);*/

    app = gtk_application_new(
        "moe.florious0721.vocagtk",
        G_APPLICATION_DEFAULT_FLAGS
    );
    if (!app) {
        status = 1;
        goto clean;
    }

    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);
    status = g_application_run(G_APPLICATION(app), argc, argv);

clean:
    if (app) g_object_unref(app);
    if (state.dl.handle) curl_easy_cleanup(state.dl.handle);
    if (state.db) sqlite3_close(state.db);
    if (state.playlists) g_object_unref(state.playlists);
    //if (headers) curl_slist_free_all(headers);

    return status;
}

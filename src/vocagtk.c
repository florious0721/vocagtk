#include <curl/curl.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sqlite3.h>
#include <yyjson.h>

#include "album.h"
#include "artist.h"
#include "data.h"
#include "entry.h"
#include "entrybox.h"
#include "song.h"

#include "exterr.h"

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

static void entry_factory_setup(
    GtkListItemFactory *factory,
    GtkListItem *item, AppState *ctx
) {
    VocagtkEntryBox *box = vocagtk_entry_box_new(ctx, NULL);

    gtk_list_item_set_child(item, GTK_WIDGET(box));
}

static void entry_factory_bind(
    GtkListItemFactory *factory,
    GtkListItem *item, AppState *ctx
) {
    VocagtkEntry *entry = gtk_list_item_get_item(item);
    VocagtkEntryBox *box = VOCAGTK_ENTRY_BOX(gtk_list_item_get_child(item));
    vocagtk_entry_box_bind(box, entry);
}

static void entry_factory_teardown(
    GtkListItemFactory *factory,
    GtkListItem *item, AppState *ctx
) {}

static void entry_factory(GObject *factory, AppState *ctx) {
    g_signal_connect(
        factory, "setup",
        G_CALLBACK(entry_factory_setup), ctx
    );
    g_signal_connect(
        factory, "bind",
        G_CALLBACK(entry_factory_bind), ctx
    );
    g_signal_connect(
        factory, "teardown",
        G_CALLBACK(entry_factory_teardown), ctx
    );
}

static void search(GtkButton *_, SearchState *ctx) {
    // TODO
    VocagtkDownloader dl;
    dl.cache_path = "/tmp/vocagtk/cache/";
    dl.handle = ctx->app->handle;

    VocagtkSearchQuery query;
    query.query = gtk_editable_get_text(GTK_EDITABLE(ctx->field));
    query.entry_type = -1;

    VocagtkSearchResultIterator it;
    vocagtk_downloader_search(&dl, &query, &it);

    vocagtk_get_search_entries(&it, ctx->results, ctx->app->db, NULL);

    yyjson_doc_free(it.doc);
}

static GtkWidget *build_search_box(AppState *ctx) {
    GtkBuilder *builder =
        gtk_builder_new_from_resource("/moe/florious0721/vocagtk/ui/search.ui");

    GObject *button = gtk_builder_get_object(builder, "searchButton");

    SearchState *data = malloc(sizeof(*data));
    data->app = ctx;
    data->field = GTK_ENTRY(
        gtk_builder_get_object(builder, "searchField")
    );
    data->type_selector = GTK_DROP_DOWN(
        gtk_builder_get_object(builder, "typeSelector")
    );
    data->results = G_LIST_STORE(
        gtk_builder_get_object(builder, "resultList")
    );

    g_signal_connect(button, "clicked", G_CALLBACK(search), data);
    g_signal_connect(button, "destroy", G_CALLBACK(free_signal_wrapper), data);

    GObject *factory = gtk_builder_get_object(builder, "entryFactory");
    entry_factory(factory, ctx);

    void *r = gtk_builder_get_object(builder, "searchPanel");
    // g_object_unref(builder);
    return r;
}

static GtkWidget *build_entry_box(AppState *ctx) {
    GtkBuilder *builder =
        gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui");

    // TODO: make factory shared among lists
    GObject *factory = gtk_builder_get_object(builder, "factory");
    entry_factory(factory, ctx);

    GListStore *model = G_LIST_STORE(
        gtk_builder_get_object(builder, "model")
    );

    g_list_store_append(
        model,
        vocagtk_entry_new_album(
            vocagtk_album_new(51119, ctx->db, ctx->handle)
        )
    );

    void *r = gtk_builder_get_object(builder, "entryList");
    // g_object_unref(builder);
    return r;
}

static void activate(GtkApplication *app, AppState *ctx) {
    GtkBuilder *builder =
        gtk_builder_new_from_resource("/moe/florious0721/vocagtk/ui/main.ui");

    GObject *window = gtk_builder_get_object(builder, "mainWindow");
    GObject *search_page = gtk_builder_get_object(builder, "searchPage");
    GObject *home_page = gtk_builder_get_object(builder, "homePage");

    GtkWidget *search_box = build_search_box(ctx);
    GtkWidget *entry_box = build_entry_box(ctx);

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(search_page),
        GTK_WIDGET(search_box)
    );
    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(home_page),
        GTK_WIDGET(entry_box)
    );

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
        "release_date INTEGER"
        ");",

        "CREATE TABLE IF NOT EXISTS song("
        "id INTEGER PRIMARY KEY,"
        "title TEXT, artist TEXT, image_url TEXT,"
        "release_date INTEGER"
        ");",

        "CREATE TABLE IF NOT EXISTS artist("
        "id INTEGER PRIMARY KEY,"
        "name TEXT, avatar_url TEXT"
        ");",

        "CREATE TABLE IF NOT EXISTS song_in_album("
        "song_id INTEGER, album_id INTEGER,"
        "PRIMARY KEY(song_id, album_id),"
        "FOREIGN KEY (song_id) PREFERENCES song(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (album_id) PREFERENCES album(id)"
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
        "artist_name TEXT, update_at INTEGER,"
        "FOREIGN KEY (artist_id) PREFERENCES artist(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (artist_name) PREFERENCES artist(name)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"
        ");",

        "CREATE TABLE IF NOT EXISTS playlist("
        "id INTEGER PRIMARY KEY,"
        "name TEXT"
        ");",

        "CREATE TABLE IF NOT EXISTS song_in_playlist("
        "id INTEGER PRIMARY KEY,"
        "song_id INTEGER, playlist_id INTEGER,"
        "FOREIGN KEY (song_id) PREFERENCES song(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT,"
        "FOREIGN KEY (playlist_id) PREFERENCES playlist(id)"
        "ON UPDATE CASCADE ON DELETE RESTRICT"
        ");",
    };

    char *errmsg;
    for (int i = 0; i < 3; ++i) {
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
    g_log_set_handler(NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL,
        g_log_default_handler, NULL);

    state.db = init_database();
    if (!state.db) {
        status = 1;
        goto clean;
    }
    state.handle = curl_easy_init();
    if (!state.handle) {
        status = 1;
        goto clean;
    }
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers,
        "Cookie: cf_clearance=2KUVcv8AGzrgktzUK28bbHmv_t.l1gvI5GjnYOu9bng-1766191196-1.2.1.1-F68vdmbpw4uxsowg5vkWvVhWEy16UlfjnbkS3.9Wkzc6VoeKnaRiit5OdFjSnflzBV4PKvkvQCoZV3QfOzd7CX48v3rdOHYVIY2wUahyxcDHtq6JdIY.WlF.OZc4oCfblKcOB5.bDAjq1khyfIp4DdYcSbY5aS1sJk8w3s0X97qE03nBChH_XYkVHgi4cwRyBGCDL8x3B2aJt8GmzVixM3XFwyWPAW8NQ41NE8958vU");
    headers = curl_slist_append(headers,
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:144.0) Gecko/20100101 Firefox/144.0");
    curl_easy_setopt(state.handle, CURLOPT_HTTPHEADER, headers);

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
    if (state.handle) curl_easy_cleanup(state.handle);
    if (state.db) sqlite3_close(state.db);
    if (headers) curl_slist_free_all(headers);

    return status;
}

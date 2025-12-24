#include <gtk/gtk.h>
#include <time.h>

#include "db.h"
#include "entrybox.h"
#include "exterr.h"
#include "parse.h"
#include "ui.h"

static void on_search_button_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    call_search((AppState *) user_data);
}

static void on_refresh_rss_clicked(GtkButton *button, gpointer user_data) {
    (void) button;
    AppState *ctx = (AppState *) user_data;
    update_artists(ctx);
    refresh_rss_song(ctx);
}

static void add_entry_factory_setup(
    GtkListItemFactory *factory,
    GtkListItem *item, EntryListCtx *ctx
) {
    VocagtkEntryBox *box = vocagtk_entry_box_new(false, ctx, NULL, item);
    gtk_list_item_set_child(item, GTK_WIDGET(box));
}

static void remove_entry_factory_setup(
    GtkListItemFactory *factory,
    GtkListItem *item, EntryListCtx *ctx
) {
    VocagtkEntryBox *box = vocagtk_entry_box_new(true, ctx, NULL, item);
    gtk_button_set_label(box->add_or_remove, "Remove/Unsubscribe");
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

void build_entry_list(GtkBuilder *builder, AppState *ctx, bool is_remove) {
    GObject *root = gtk_builder_get_object(builder, "root");

    GObject *select = gtk_builder_get_object(builder, "select");
    GObject *model = gtk_builder_get_object(builder, "model");
    GObject *factory = gtk_builder_get_object(builder, "factory");

    GObject *button = gtk_builder_get_object(builder, "add_or_remove");
    GObject *dropdown = gtk_builder_get_object(builder, "playlist_select");
    GObject *select_all = gtk_builder_get_object(builder, "select_all");

    EntryListCtx *listctx = malloc(sizeof(*listctx));
    listctx->app = ctx;
    listctx->select = GTK_SELECTION_MODEL(select);
    listctx->store = G_LIST_STORE(model);
    listctx->songlist_id = -1;

    // bug here ?
    g_signal_connect(
        root, "destroy",
        G_CALLBACK(free_signal_wrapper), listctx
    );

    g_signal_connect(
        factory, "setup",
        G_CALLBACK(
            is_remove
            ? remove_entry_factory_setup
            : add_entry_factory_setup
        ), listctx
    );
    g_signal_connect(
        factory, "bind",
        G_CALLBACK(entry_factory_bind), listctx
    );
}

void build_rss_artist_box(GtkBuilder *main, AppState *ctx) {
    GtkBuilder *builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui"
    );

    GObject *box = gtk_builder_get_object(main, "rss_artist_box");
    GObject *root = gtk_builder_get_object(builder, "root");
    GObject *model = gtk_builder_get_object(builder, "model");
    build_entry_list(builder, ctx, true);

    ctx->rss_artist = G_LIST_STORE(model);

    gtk_box_append(GTK_BOX(box), GTK_WIDGET(root));

    g_object_unref(builder);
    call_init_rss_artist(ctx);
}

void build_rss_song_box(GtkBuilder *main, AppState *ctx) {
    GtkBuilder *builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui"
    );

    GObject *box = gtk_builder_get_object(main, "rss_song_box");
    GObject *root = gtk_builder_get_object(builder, "root");
    GObject *list = gtk_builder_get_object(builder, "model");
    GObject *controls = gtk_builder_get_object(builder, "controls");

    ctx->rss_song = G_LIST_STORE(list);

    build_entry_list(builder, ctx, false);

    // Create refresh button
    GtkWidget *refresh_button = gtk_button_new_with_label("Refresh");
    gtk_widget_set_halign(refresh_button, GTK_ALIGN_END);
    // gtk_widget_set_hexpand(refresh_button, TRUE);
    g_signal_connect(
        refresh_button, "clicked",
        G_CALLBACK(on_refresh_rss_clicked), ctx
    );
    gtk_box_append(GTK_BOX(controls), refresh_button);

    gtk_box_append(GTK_BOX(box), GTK_WIDGET(root));

    refresh_rss_song(ctx);

    g_object_unref(builder);
}

void build_search(GtkBuilder *main, AppState *ctx) {
    GtkBuilder *panel_builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/search.ui"
    );
    GtkBuilder *list_builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui"
    );

    GObject *search_box = gtk_builder_get_object(main, "search_page");
    GObject *entry_list = gtk_builder_get_object(
        list_builder, "root"
    );
    GObject *model = gtk_builder_get_object(
        list_builder, "model"
    );

    build_entry_list(list_builder, ctx, false);

    GObject *search_panel = gtk_builder_get_object(
        panel_builder, "search_panel"
    );
    GObject *search_field = gtk_builder_get_object(
        panel_builder, "search_field"
    );
    GObject *type_selector = gtk_builder_get_object(
        panel_builder, "type_selector"
    );
    GObject *search_button = gtk_builder_get_object(
        panel_builder, "search_button"
    );

    ctx->search_widgets.field = GTK_ENTRY(search_field);
    ctx->search_widgets.type_selector = GTK_DROP_DOWN(type_selector);
    ctx->search_widgets.list = G_LIST_STORE(model);

    g_signal_connect(
        search_button, "clicked",
        G_CALLBACK(on_search_button_clicked), ctx
    );

    gtk_box_append(GTK_BOX(search_box), GTK_WIDGET(search_panel));
    gtk_box_append(GTK_BOX(search_box), GTK_WIDGET(entry_list));

    g_object_unref(panel_builder);
    g_object_unref(list_builder);
}

void build_home(GtkBuilder *main, AppState *ctx) {
    GtkBuilder *builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui"
    );

    GObject *box = gtk_builder_get_object(main, "home_page");
    GObject *root = gtk_builder_get_object(builder, "root");

    gtk_box_append(GTK_BOX(box), GTK_WIDGET(root));

    g_object_unref(builder);
}

void build_playlist(GtkBuilder *main, AppState *ctx) {
    GtkBuilder *builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/entrylist.ui"
    );
    GtkBuilder *ctrl_builder = gtk_builder_new_from_resource(
        "/moe/florious0721/vocagtk/ui/ctrl.ui"
    );

    GObject *box = gtk_builder_get_object(main, "playlist_page");
    GObject *root = gtk_builder_get_object(builder, "root");
    GObject *ctrl = gtk_builder_get_object(ctrl_builder, "playlist_control");

    GObject *select = gtk_builder_get_object(builder, "playlist_select");
    GObject *list = gtk_builder_get_object(builder, "model");

    ctx->songlist_select = GTK_DROP_DOWN(select);
    ctx->current_songlist = G_LIST_STORE(list);

    gtk_box_append(GTK_BOX(box), GTK_WIDGET(ctrl));
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(root));

    g_object_unref(builder);
}

void update_artist(AppState *ctx, VocagtkArtist *artist) {
    int artist_id = vocagtk_artist_get_id(artist);
    time_t last_update = vocagtk_artist_get_update_at(artist);

    DEBUG("Updating artist %d, last update at %ld", artist_id, last_update);

    VocagtkResultIterator iter;
    vocagtk_downloader_update(artist_id, last_update, &iter);

    CURLcode curl_err = CURLE_OK;
    int sql_err = SQLITE_OK;
    yyjson_val *song_json = NULL;

    while (
         (song_json = vocagtk_result_iterator_next(&iter, &ctx->dl, &curl_err))
         != NULL
    ) {
        if (curl_err != CURLE_OK) {
            vocagtk_warn_curl_rcode(curl_err);
            continue;
        }

        // 1. Insert song itself into database
        sql_err = db_song_add_from_json(ctx->db, song_json);
        if (sql_err != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }

        // 2. Insert song-album relations into database
        db_insert_song_albums(ctx->db, song_json, &sql_err);
        if (sql_err != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }

        // 3. Insert song-artist relations into database
        db_insert_song_artists(ctx->db, song_json, &sql_err);
        if (sql_err != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }
    }

    // Update artist's update_at timestamp to current time
    time_t current_time = time(NULL);
    sql_err = db_artist_update_time(ctx->db, artist_id, current_time);
    if (sql_err != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
    } else {
        DEBUG("Updated artist %d update_at to %ld", artist_id, current_time);
    }

    DEBUG("Finished updating artist %d", artist_id);
}

void update_artists(AppState *ctx) {
    DEBUG("Starting batch update of all subscribed artists");

    char const *sql = "SELECT artist_id FROM rss;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
        return;
    }

    // Collect all artist IDs first to avoid database cursor issues
    GArray *artist_ids = g_array_new(FALSE, FALSE, sizeof(int));
    while ((rcode = sqlite3_step(stmt)) == SQLITE_ROW) {
        int artist_id = sqlite3_column_int(stmt, 0);
        g_array_append_val(artist_ids, artist_id);
    }
    sqlite3_finalize(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(ctx->db);
        g_array_free(artist_ids, TRUE);
        return;
    }

    DEBUG("Found %d artists to update", artist_ids->len);

    // Update each artist
    for (guint i = 0; i < artist_ids->len; i++) {
        int artist_id = g_array_index(artist_ids, int, i);
        int sql_err = 0;

        VocagtkArtist *artist =
            db_artist_get_by_id(ctx->db, artist_id, &sql_err);
        if (!artist) {
            DEBUG("Could not load artist %d, skipping", artist_id);
            continue;
        }

        DEBUG("Updating artist %d (%d/%d)", artist_id, i + 1, artist_ids->len);
        update_artist(ctx, artist);
        g_object_unref(artist);
    }

    g_array_free(artist_ids, TRUE);
    DEBUG("Finished batch update of all artists");
}

void refresh_rss_song(AppState *ctx) {
    DEBUG("Refreshing RSS song list");

    // Clear the current list
    g_list_store_remove_all(ctx->rss_song);

    // Query songs from subscribed artists, ordered by publish_date descending, limit 256
    char const *sql =
        "SELECT DISTINCT s.id, s.title, s.artist, s.image_url, s.publish_date "
        "FROM song s "
        "JOIN artist_for_song afs ON s.id = afs.song_id "
        "JOIN rss r ON afs.artist_id = r.artist_id "
        "ORDER BY s.publish_date DESC "
        "LIMIT 256;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
        return;
    }

    while ((rcode = sqlite3_step(stmt)) == SQLITE_ROW) {
        int sql_err = 0;
        VocagtkSong *song = db_song_from_row(stmt, &sql_err);
        if (song) {
            VocagtkEntry *entry = vocagtk_entry_new_song(song);
            g_list_store_append(ctx->rss_song, entry);
            g_object_unref(entry);
        }
    }

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(ctx->db);
    }

    sqlite3_finalize(stmt);

    DEBUG("RSS song list refreshed");
}

void call_init_rss_artist(AppState *ctx) {
    char const *sql =
        "SELECT a.id, a.name, a.avatar_url, a.update_at "
        "FROM artist a "
        "JOIN rss r ON a.id = r.artist_id;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
        return;
    }

    while ((rcode = sqlite3_step(stmt)) == SQLITE_ROW) {
        int sql_err = 0;
        VocagtkArtist *artist = db_artist_from_row(stmt, &sql_err);
        if (!artist) continue;

        VocagtkEntry *entry = vocagtk_entry_new_artist(artist);
        if (entry) {
            g_list_store_append(ctx->rss_artist, entry);
            g_object_unref(entry);
        }
        g_object_unref(artist);
    }

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(ctx->db);
    }

    sqlite3_finalize(stmt);
}

/*void call_update_rss_song(AppState *ctx, int artist_id) {
    int sql_err = SQLITE_OK;
    CURLcode curl_err = CURLE_OK;

    time_t last_update = db_rss_get_update_time(ctx->db, artist_id, &sql_err);

    VocagtkResultIterator it;
    vocagtk_downloader_update(artist_id, last_update, &it);

    yyjson_val *song_json = NULL;
    while ((song_json = vocagtk_result_iterator_next(&it, &ctx->dl,
        &curl_err)) != NULL) {
        db_insert_song_albums(ctx->db, song_json, &sql_err);
        db_insert_song_artists(ctx->db, song_json, &sql_err);

        VocagtkSong *song = json_to_song(song_json);
        if (!song) continue;

        VocagtkEntry *entry = vocagtk_entry_new_song(song);
        if (entry) {
            g_list_store_append(ctx->rss_song, entry);
            g_object_unref(entry);
        }
        g_object_unref(song);
    }
   }*/

void call_search(AppState *ctx) {
    char const *query_str =
        gtk_editable_get_text(GTK_EDITABLE(ctx->search_widgets.field));
    if (!query_str) query_str = "Hello";

    // guint selected = gtk_drop_down_get_selected(ctx->search_widgets.type_selector);
    char const *entry_type = "";
    GtkStringObject *sel_obj =
        gtk_drop_down_get_selected_item(ctx->search_widgets.type_selector);
    if (sel_obj) entry_type = gtk_string_object_get_string(sel_obj);
    if (g_strcmp0(entry_type, "Anything") == 0) entry_type = "";

    VocagtkSearchQuery q = {
        .query = query_str,
        .entry_type = entry_type,
    };

    g_list_store_remove_all(ctx->search_widgets.list);

    VocagtkResultIterator it;
    vocagtk_downloader_search(&q, &it);

    CURLcode curl_err = CURLE_OK;
    yyjson_val *obj = NULL;
    while (
         (obj = vocagtk_result_iterator_next(&it, &ctx->dl, &curl_err))
         != NULL
    ) {
        if (curl_err != CURLE_OK) vocagtk_warn_curl_rcode(curl_err);
        VocagtkEntry *entry = json_to_entry(obj);
        if (!entry) continue;

        // Save the entry to local database
        int rcode = db_entry_add(ctx->db, entry);
        if (rcode != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }

        g_list_store_append(ctx->search_widgets.list, entry);
        g_object_unref(entry);
    }

}

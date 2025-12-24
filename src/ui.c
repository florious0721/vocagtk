#include <gtk/gtk.h>
#include <time.h>

#include "db.h"
#include "entrybox.h"
#include "exterr.h"
#include "parse.h"
#include "ui.h"

typedef struct {
    GtkEditable *field;
    AppState *app;
} PlaylistCreateCtx;

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

static void toggle_select_all(
    GtkCheckButton *check,
    GtkSelectionModel *select
) {
    if (gtk_check_button_get_active(check)) {
        gtk_selection_model_select_all(select);
    } else {
        gtk_selection_model_unselect_all(select);
    }
}

/**
 * Watch all selected entries in the selection model
 * @param _ GtkButton that triggered the action (unused)
 * @param ctx EntryListCtx containing selection model and context
 */
static void watch_entries(GtkButton *_, EntryListCtx *ctx) {
    (void) _; // Mark unused parameter

    GtkBitset *selection = gtk_selection_model_get_selection(ctx->select);
    guint selection_size = gtk_bitset_get_size(selection);

    if (selection_size == 0) {
        DEBUG("No entries selected for watch operation");
        return;
    }

    DEBUG("Starting batch watch for %u selected entries", selection_size);

    GtkBitsetIter iter;
    bool next = true;
    guint idx = 0;

    for (
         next = gtk_bitset_iter_init_first(&iter, selection, &idx);
         next;
         next = gtk_bitset_iter_next(&iter, &idx)
    ) {
        VocagtkEntry *entry = g_list_model_get_item(
            G_LIST_MODEL(ctx->select), idx
        );

        if (!entry) {
            DEBUG("Failed to get entry at index %u, skipping", idx);
            continue;
        }

        // Call watch_entry for each selected entry
        watch_entry(entry, ctx, idx);

        // Unref the entry after use (g_list_model_get_item returns a ref)
        g_object_unref(entry);
    }

    DEBUG("Completed batch watch operation");
}

/**
 * Unwatch all selected entries in the selection model
 * @param _ GtkButton that triggered the action (unused)
 * @param ctx EntryListCtx containing selection model and context
 */
static void unwatch_entries(GtkButton *_, EntryListCtx *ctx) {
    (void) _; // Mark unused parameter

    GtkBitset *selection = gtk_selection_model_get_selection(ctx->select);
    guint selection_size = gtk_bitset_get_size(selection);

    if (selection_size == 0) {
        DEBUG("No entries selected for unwatch operation");
        return;
    }

    DEBUG("Starting batch unwatch for %u selected entries", selection_size);

    // Collect all selected indices in descending order to avoid index shifts
    // when removing items from the list
    GArray *indices = g_array_sized_new(FALSE, FALSE, sizeof(guint),
        selection_size);

    GtkBitsetIter iter;
    bool next = true;
    guint idx = 0;

    // Collect all indices
    for (
         next = gtk_bitset_iter_init_first(&iter, selection, &idx);
         next;
         next = gtk_bitset_iter_next(&iter, &idx)
    ) {
        g_array_append_val(indices, idx);
    }

    // Sort indices in descending order to remove from back to front
    // This prevents index shifting issues during removal
    for (guint i = 0; i < indices->len; i++) {
        for (guint j = i + 1; j < indices->len; j++) {
            guint idx_i = g_array_index(indices, guint, i);
            guint idx_j = g_array_index(indices, guint, j);
            if (idx_i < idx_j) {
                g_array_index(indices, guint, i) = idx_j;
                g_array_index(indices, guint, j) = idx_i;
            }
        }
    }

    // Process each entry from highest index to lowest
    for (guint i = 0; i < indices->len; i++) {
        guint position = g_array_index(indices, guint, i);
        VocagtkEntry *entry = g_list_model_get_item(
            G_LIST_MODEL(ctx->select), position
        );

        if (!entry) {
            DEBUG("Failed to get entry at index %u, skipping", position);
            continue;
        }

        // Call unwatch_entry for each selected entry
        unwatch_entry(entry, ctx, position);

        // Unref the entry after use (g_list_model_get_item returns a ref)
        g_object_unref(entry);
    }

    g_array_free(indices, TRUE);
    DEBUG("Completed batch unwatch operation");
}

/**
 * Set the current_songlist_id in AppState based on the selected item in dropdown
 * and refresh the playlist display with songs from the selected playlist
 * @param dropdown GtkDropDown widget
 * @param ctx AppState to update
 */
static void select_playlist_global(GtkDropDown *dropdown, GParamSpec *spec,
    AppState *ctx) {
    // Get the selected item from dropdown
    GtkStringObject *selected = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(dropdown)
    );

    if (!selected) {
        DEBUG("No playlist selected in global dropdown");
        ctx->current_playlist_name = NULL;

        g_list_store_remove_all(ctx->current_playlist);
        return;
    }

    // Get the playlist name from selected string object
    char const *playlist_name = gtk_string_object_get_string(selected);
    if (!playlist_name) {
        DEBUG("Failed to get playlist name from selected item");
        ctx->current_playlist_name = NULL;

        g_list_store_remove_all(ctx->current_playlist);
        return;
    }

    // Query database to get playlist ID by name
    int sql_err = SQLITE_OK;
    bool exist = db_playlist_exists(ctx->db, playlist_name, &sql_err);

    if (exist) {
        // Successfully found the playlist
        DEBUG(
            "Selected global playlist '%s'",
            playlist_name

        );
        ctx->current_playlist_name = playlist_name;

        // Clear the current list
        g_list_store_remove_all(ctx->current_playlist);

        // Load songs from the selected playlist
        sqlite3_stmt *stmt;
        int rcode = db_playlist_get_songs(ctx->db, playlist_name, &stmt);

        if (rcode != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
            return;
        }

        // Iterate through songs and add to list
        while ((rcode = sqlite3_step(stmt)) == SQLITE_ROW) {
            int song_sql_err = 0;
            VocagtkSong *song = db_song_from_row(stmt, &song_sql_err);
            if (song) {
                VocagtkEntry *entry = vocagtk_entry_new_song(song);
                g_list_store_append(ctx->current_playlist, entry);
                g_object_unref(entry);
            }
        }

        if (rcode != SQLITE_DONE) {
            vocagtk_warn_sql_db(ctx->db);
        }

        sqlite3_finalize(stmt);
        DEBUG(
            "Loaded %d songs from playlist '%s'",
            g_list_model_get_n_items(G_LIST_MODEL(ctx->current_playlist)),
            playlist_name

        );
    } else if (sql_err == SQLITE_OK) {
        // Playlist not found in database
        DEBUG("Global playlist '%s' not found in database", playlist_name);
        ctx->current_playlist_name = NULL;

        g_list_store_remove_all(ctx->current_playlist);
    } else {
        // Database error
        vocagtk_warn_sql_db(ctx->db);
        DEBUG("Database error when querying global playlist '%s'",
            playlist_name);
        ctx->current_playlist_name = NULL;

        g_list_store_remove_all(ctx->current_playlist);
    }
}

/**
 * Set the songlist_id in EntryListCtx based on the selected item in dropdown
 * @param self GtkDropDown widget
 * @param ctx EntryListCtx to update
 */
static void select_playlist(GtkDropDown *self, EntryListCtx *ctx) {
    // Get the selected item from dropdown
    GtkStringObject *selected = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(self)
    );

    if (!selected) {
        DEBUG("No playlist selected in dropdown");
        ctx->playlist_name = NULL;

        return;
    }

    // Get the playlist name from selected string object
    char const *playlist_name = gtk_string_object_get_string(selected);
    if (!playlist_name) {
        DEBUG("Failed to get playlist name from selected item");
        ctx->playlist_name = NULL;

        return;
    }

    // Query database to get playlist ID by name
    int sql_err = SQLITE_OK;
    bool exist = db_playlist_exists(ctx->app->db, playlist_name, &sql_err);

    if (exist) {
        // Successfully found the playlist
        DEBUG("Selected playlist '%s'", playlist_name);
        ctx->playlist_name = playlist_name;

    } else if (sql_err == SQLITE_OK) {
        // Playlist not found in database
        DEBUG("Playlist '%s' not found in database", playlist_name);
        ctx->playlist_name = NULL;

    } else {
        // Database error
        DEBUG("Database error when querying playlist '%s'", playlist_name);
        ctx->playlist_name = NULL;

    }
}

static void create_playlist(GtkButton *button, PlaylistCreateCtx *ctx) {
    (void) button;

    char const *name = gtk_editable_get_text(ctx->field);
    int sql_err = SQLITE_OK;

    int exists = db_playlist_exists(ctx->app->db, name, &sql_err);
    if (sql_err != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->app->db);
        return;
    }
    if (exists == 1) {
        vocagtk_warn_def("playlist %s already exists", name);
        return;
    }

    sql_err = db_playlist_create(ctx->app->db, name);

    if (sql_err != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->app->db);
        return;
    }

    gtk_string_list_append(ctx->app->playlists, name);
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
    listctx->playlist_name = "Default";

    // bug here ?
    g_signal_connect(
        root, "destroy",
        G_CALLBACK(free_signal_wrapper), listctx
    );

    g_signal_connect(
        select_all, "toggled",
        G_CALLBACK(toggle_select_all), select
    );

    g_signal_connect(
        button, "clicked",
        G_CALLBACK(is_remove ? unwatch_entries : watch_entries), listctx
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

    // Connect dropdown to update songlist_id when selection changes
    if (dropdown) {
        gtk_drop_down_set_model(GTK_DROP_DOWN(dropdown),
            G_LIST_MODEL(ctx->playlists));
        g_signal_connect(
            dropdown, "notify::selected-item",
            G_CALLBACK(select_playlist), listctx
        );
    }
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

/**
 * Build playlist control panel with create/delete buttons
 * @param builder GtkBuilder containing the control UI
 * @param ctx AppState containing application context
 */
static void build_ctrl(GtkBuilder *builder, AppState *ctx) {
    GObject *create_button = gtk_builder_get_object(builder, "create");
    GObject *field = gtk_builder_get_object(builder, "field");

    // Allocate context for control panel
    PlaylistCreateCtx *ctrl_ctx = malloc(sizeof(*ctrl_ctx));
    ctrl_ctx->app = ctx;
    ctrl_ctx->field = GTK_EDITABLE(field);

    // Connect create button to callback
    g_signal_connect(
        create_button, "clicked",
        G_CALLBACK(create_playlist), ctrl_ctx
    );

    // Connect destroy signal to free context
    g_signal_connect(
        create_button, "destroy",
        G_CALLBACK(free_signal_wrapper), ctrl_ctx
    );
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

    ctx->playlist_select = GTK_DROP_DOWN(select);
    ctx->current_playlist = G_LIST_STORE(list);
    ctx->current_playlist_name = "Default";

    sqlite3_stmt *stmt;
    int rcode = db_playlist_get_songs(ctx->db, ctx->current_playlist_name,
        &stmt);

    if (rcode == SQLITE_OK) {
        while ((rcode = sqlite3_step(stmt)) == SQLITE_ROW) {
            int song_sql_err = 0;
            VocagtkSong *song = db_song_from_row(stmt, &song_sql_err);
            if (song) {
                VocagtkEntry *entry = vocagtk_entry_new_song(song);
                g_list_store_append(ctx->current_playlist, entry);
                g_object_unref(entry);
            }
        }

        if (rcode != SQLITE_DONE) {
            vocagtk_warn_sql_db(ctx->db);
        }

        sqlite3_finalize(stmt);
    } else {
        vocagtk_warn_sql_db(ctx->db);
        DEBUG("Failed to load songs from default playlist");
    }

    // Connect dropdown to global playlist selection
    if (ctx->playlist_select) {
        gtk_drop_down_set_model(ctx->playlist_select,
            G_LIST_MODEL(ctx->playlists));
        g_signal_connect(
            ctx->playlist_select, "notify::selected-item",
            G_CALLBACK(select_playlist_global), ctx
        );
    }

    build_entry_list(builder, ctx, true);

    // Build control panel (create/delete buttons)
    build_ctrl(ctrl_builder, ctx);

    gtk_box_append(GTK_BOX(box), GTK_WIDGET(ctrl));
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(root));

    g_object_unref(builder);
    g_object_unref(ctrl_builder);
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
        "LIMIT 64;";

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

void watch_entry(VocagtkEntry *entry, EntryListCtx *list_ctx, int position) {
    int id = -1;
    DEBUG("Watching entry at position %d", position);

    switch (entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        DEBUG("Album add not implemented.");
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        id = entry->entry.artist->id;
        int sql_err = SQLITE_OK;
        int inserted = db_rss_add_artist(list_ctx->app->db, id, &sql_err);

        if (sql_err != SQLITE_OK) {
            DEBUG("Failed to add artist %d to RSS, error code: %d", id,
                sql_err);
            break;
        }

        VocagtkArtist *artist = vocagtk_artist_new(id, list_ctx->app);

        if (inserted > 0) {
            // Newly inserted, add to UI list
            VocagtkEntry *new_entry = vocagtk_entry_new_artist(artist);
            g_list_store_append(list_ctx->app->rss_artist, new_entry);
            g_object_unref(new_entry);
        } else {
            DEBUG("Artist %d already subscribed", id);
        }

        // Always update artist's songs and refresh RSS song list
        // (artist may have new songs even if already subscribed)
        update_artist(list_ctx->app, artist);
        refresh_rss_song(list_ctx->app);

        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        // Add song to selected playlist
        if (!list_ctx->playlist_name) {
            vocagtk_warn_def("No playlist selected, cannot add song");
            break;
        }

        id = entry->entry.song->id;
        int song_sql_err = SQLITE_OK;

        // First, ensure the song exists in the database
        int song_add_err = db_song_add(list_ctx->app->db, entry->entry.song);
        if (song_add_err != SQLITE_OK && song_add_err != SQLITE_DONE) {
            DEBUG("Failed to add song %d to database", id);
            vocagtk_warn_sql_db(list_ctx->app->db);
            break;
        }

        // Add song to the selected playlist
        int song_inserted = db_playlist_add_song(
            list_ctx->app->db,
            list_ctx->playlist_name,
            id,
            &song_sql_err
        );

        if (song_sql_err != SQLITE_OK) {
            DEBUG(
                "Failed to add song %d to playlist '%s', error: %s",
                id, list_ctx->playlist_name,
                sqlite3_errmsg(list_ctx->app->db)
            );
            break;
        }

        if (song_inserted > 0) {
            DEBUG(
                "Added song %d to playlist '%s'",
                id, list_ctx->playlist_name
            );

            // Check if this playlist is currently displayed
            if (list_ctx->app->current_playlist_name &&
                 g_strcmp0(list_ctx->playlist_name,
                list_ctx->app->current_playlist_name) == 0) {
                g_list_store_append(list_ctx->app->current_playlist, entry);
                DEBUG(
                    "Appended song %d to current playlist UI (playlist '%s')",
                    id, list_ctx->playlist_name
                );
            }
        } else {
            DEBUG(
                "Song %d already in playlist '%s'",
                id, list_ctx->playlist_name
            );
        }

        break;
    default:
        break;
    }
}

void unwatch_entry(VocagtkEntry *entry, EntryListCtx *list_ctx, int position) {
    DEBUG("Unwatching entry at position %d", position);

    switch (entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        DEBUG("Album remove not implemented.");
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        db_rss_remove_artist(list_ctx->app->db, entry->entry.artist->id);
        g_list_store_remove(list_ctx->store, position);
        // Refresh RSS song list after removing artist
        refresh_rss_song(list_ctx->app);
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        DEBUG("Song remove not implemented.");
        break;
    default:
        break;
    }
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

#ifndef _VOCAGTK_UI_H
#define _VOCAGTK_UI_H
#include <gtk/gtk.h>
#include "helper.h"

#include "entry.h"

typedef struct {
    AppState *app;
    GtkSelectionModel *select;
    GListStore *store;
    char const *playlist_name;
} EntryListCtx;



void build_home(GtkBuilder *main, AppState *ctx);

void build_search(GtkBuilder *main, AppState *ctx);

void build_rss_artist_box(GtkBuilder *main, AppState *ctx);

void build_rss_song_box(GtkBuilder *main, AppState *ctx);

void build_playlist(GtkBuilder *main, AppState *ctx);

void call_search(AppState *ctx);
void call_init_rss_artist(AppState *ctx);
//void call_update_rss_song(AppState *ctx, int artist_id);

void update_artist(AppState *ctx, VocagtkArtist *artist);
void update_artists(AppState *ctx);
void refresh_rss_song(AppState *ctx);

void watch_entry(VocagtkEntry *entry, EntryListCtx *list_ctx, int position);
void unwatch_entry(VocagtkEntry *entry, EntryListCtx *list_ctx, int position);

#endif

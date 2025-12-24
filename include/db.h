#ifndef _VOCAGTK_DB_H
#define _VOCAGTK_DB_H

#include "artist.h"
#include "album.h"
#include "song.h"
#include "entry.h"

int db_album_add(sqlite3 *db, VocagtkAlbum const *album);
int db_album_add_from_json(sqlite3 *db, yyjson_val *album_json);
VocagtkAlbum *db_album_get_by_id(sqlite3 *db, int id, int *sql_err);
VocagtkAlbum *db_album_from_row(sqlite3_stmt *stmt, int *sql_err);

int db_artist_add(sqlite3 *db, VocagtkArtist const *artist);
int db_artist_add_from_json(sqlite3 *db, yyjson_val *artist_json);
int db_artist_update_time(sqlite3 *db, int artist_id, time_t update_at);
VocagtkArtist *db_artist_get_by_id(sqlite3 *db, int id, int *sql_err);
VocagtkArtist *db_artist_from_row(sqlite3_stmt *stmt, int *sql_err);

int db_song_add(sqlite3 *db, VocagtkSong const *song);
int db_song_add_from_json(sqlite3 *db, yyjson_val *song_json);
VocagtkSong *db_song_from_row(sqlite3_stmt *stmt, int *sql_err);
VocagtkSong *db_song_get_by_id(sqlite3 *db, int id, int *sql_err);

int db_insert_song_albums(sqlite3 *db, yyjson_val *song_json, int *sql_err);
int db_insert_song_artists(sqlite3 *db, yyjson_val *song_json, int *sql_err);

// Add artist to RSS subscription
// Returns: number of rows inserted (1 if newly inserted, 0 if already exists)
// sql_err: receives SQLite error code if provided
int db_rss_add_artist(sqlite3 *db, int artist_id, int *sql_err);
int db_rss_remove_artist(sqlite3 *db, int artist_id);

int db_entry_add(sqlite3 *db, VocagtkEntry const *entry);

// Playlist operations
// Returns: number of playlists created (1 if newly created, 0 if already exists)
// sql_err: receives SQLite error code if provided
int db_playlist_create(sqlite3 *db, char const *name, int *sql_err);
int db_playlist_delete(sqlite3 *db, char const *name);
int db_playlist_rename(sqlite3 *db, char const *old_name, char const *new_name);
// Initialize a stmt, which can then be used to get playlist names
int db_playlist_get_all(sqlite3 *db, sqlite3_stmt **stmt);
// Returns: 1 if exists, 0 if not found; error via sql_err
int db_playlist_exists(sqlite3 *db, char const *name, int *sql_err);

// Song in playlist operations
// Returns: number of rows inserted (1 if newly added, 0 if already exists)
int db_playlist_add_song(sqlite3 *db, char const *playlist_name, int song_id, int *sql_err);
int db_playlist_remove_song(sqlite3 *db, char const *playlist_name, int song_id);
// Initialize a stmt, which can then be used to get songs
int db_playlist_get_songs(sqlite3 *db, char const *playlist_name, sqlite3_stmt **stmt);

#endif

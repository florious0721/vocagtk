#ifndef _VOCAGTK_SONG_H
#define _VOCAGTK_SONG_H

#include <curl/curl.h>
#include <glib-object.h>
#include <sqlite3.h>
#include <yyjson.h>

// *INDENT-OFF*
G_BEGIN_DECLS
// *INDENT-ON*

#define VOCAGTK_TYPE_SONG vocagtk_song_get_type()
G_DECLARE_FINAL_TYPE(VocagtkSong, vocagtk_song, VOCAGTK, SONG, GObject)

typedef gint VocagtkSongId;

/*!
 * @brief
 *   Creates a new song object with its id in VocaDB.
 *   The function will firstly attempt to look up
 *   its data in the local database.
 *   If the data of song is not in the local database,
 *   the function will fetch the data from VocaDB.
 *
 * @param id
 *   The id of song in VocaDB.
 * @param db
 *   The local database for query cache.
 * @param handle
 *   The curl handle to use if the information of song is not in the `db`.
 *
 * @returns
 *   A new VocagtkSong instance.
 *   The data is owned by the caller of the function.
 */
VocagtkSong *vocagtk_song_new(
    VocagtkSongId id,
    sqlite3 *db, CURL *handle
);
VocagtkSong *vocagtk_song_new_from_json(yyjson_val *json);
VocagtkSong *vocagtk_song_new_from_db(gint id, sqlite3 *db, int *err);

/*!
 * @brief Creates a new song object from JSON and saves it to the database.
 *
 * @param json
 *   A json object containing song information in VocaDB schema.
 * @param db
 *   The local database to save the song information.
 * @param sql_err
 *   Pointer to store SQL error code, can be NULL.
 *
 * @returns
 *   A new VocagtkSong instance.
 *   The data is owned by the caller of the function.
 */
VocagtkSong *vocagtk_song_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
);

int vocagtk_song_save_to_db(VocagtkSong const *self, sqlite3 *db);

/*!
 * @brief Gets the id of the song in VocaDB.
 *
 * @returns The id of the song.
 */
VocagtkSongId vocagtk_song_get_id(VocagtkSong *self);

/*!
 * @brief Gets the title of the song.
 *
 * @returns
 *   The title of the song.
 *   The data is owned by the instance.
 */
char const *vocagtk_song_get_title(VocagtkSong *self);

/*!
 * @brief Gets the artist string of the song.
 *
 * @returns
 *   The artist string of the song.
 *   The data is owned by the instance.
 */
char const *vocagtk_song_get_artist(VocagtkSong *self);
typedef struct _VocagtkSong {
    GObject parent_instance;
    VocagtkSongId id;
    GString *title;
    GString *artist;
    GString *image_url;
} VocagtkSong;

G_END_DECLS

#endif

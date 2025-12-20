#ifndef _VOCAGTK_ARTIST_H
#define _VOCAGTK_ARTIST_H

#include <curl/curl.h>
#include <glib-object.h>
#include <sqlite3.h>
#include <yyjson.h>

// *INDENT-OFF*
G_BEGIN_DECLS
// *INDENT-ON*

#define VOCAGTK_TYPE_ARTIST vocagtk_artist_get_type()
G_DECLARE_FINAL_TYPE(VocagtkArtist, vocagtk_artist, VOCAGTK, ARTIST, GObject)

typedef gint VocagtkArtistId;

/*!
 * @brief
 *   Creates a new artist object with its id in VocaDB.
 *   The function will firstly attempt to look up its data
 *   in the local database.
 *   If the data of artist is not in the local database,
 *   the function will fetch the data from VocaDB.
 *
 * @param id
 *   The id of artist in VocaDB.
 * @param db
 *   The local database for query cache.
 * @param handle
 *   The curl handle to use
 *   if the information of artist is not in the `db`.
 *
 * @returns
 *   A new VocagtkArtist instance.
 *   The data is owned by the caller of the function.
 */
VocagtkArtist *vocagtk_artist_new(
    VocagtkArtistId id,
    sqlite3 *db, CURL *handle
);

VocagtkArtist *vocagtk_artist_new_from_json(yyjson_val *json);
VocagtkArtist *vocagtk_artist_new_from_db(gint id, sqlite3 *db, int *err);

/*!
 * @brief Creates a new artist object from JSON and saves it to the database.
 *
 * @param json
 *   A json object containing artist information in VocaDB schema.
 * @param db
 *   The local database to save the artist information.
 * @param sql_err
 *   Pointer to store SQL error code, can be NULL.
 *
 * @returns
 *   A new VocagtkArtist instance.
 *   The data is owned by the caller of the function.
 */
VocagtkArtist *vocagtk_artist_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
);

int vocagtk_artist_save_to_db(VocagtkArtist const *self, sqlite3 *db);

/*!
 * @brief Gets the id of the artist in VocaDB.
 *
 * @returns The id of the artist.
 */
gint vocagtk_artist_get_id(VocagtkArtist *self);

/*!
 * @brief Gets the name of the artist.
 *
 * @returns
 *   The name of the artist.
 *   The data is owned by the instance.
 */
char const *vocagtk_artist_get_name(VocagtkArtist *self);

/*!
 * @brief Gets the url of avatar image of the artist.
 *
 * @returns
 *   The url of avatar image of the artist.
 *   The data is owned by the instance.
 */
char const *vocagtk_artist_get_avatar_url(VocagtkArtist *self);

typedef struct _VocagtkArtist {
    GObject parent_instance;
    VocagtkArtistId id;
    GString *name;
    GString *avatar_url;
} VocagtkArtist;

G_END_DECLS
#endif

#ifndef _VOCAGTK_ALBUM_H
#define _VOCAGTK_ALBUM_H

#include <curl/curl.h>
#include <glib-object.h>
#include <sqlite3.h>
#include <yyjson.h>

// Avoid uncrustify's wrong indent because of macro usage
// *INDENT-OFF*
G_BEGIN_DECLS
// *INDENT-ON*

#define VOCAGTK_TYPE_ALBUM vocagtk_album_get_type()
G_DECLARE_FINAL_TYPE(VocagtkAlbum, vocagtk_album, VOCAGTK, ALBUM, GObject)

typedef gint VocagtkAlbumId;

/*!
 * @brief
 *   Creates a new album object with its id in VocaDB.
 *   The function will firstly attempt to look up its data in the local database.
 *   If the data of album is not in the local database,
 *   the function will fetch the data from VocaDB.
 *
 * @param id The id of album in VocaDB.
 * @param db The local database for query cache.
 * @param handle
 *   The curl handle to use if the information of album is not in the `db`.
 *
 * @returns
 *   A new VocagtkAlbum instance.
 *   The data is owned by the caller of the function.
 */
VocagtkAlbum *vocagtk_album_new(
    VocagtkAlbumId id,
    sqlite3 *db, CURL *handle
);

VocagtkAlbum *vocagtk_album_new_from_db(gint id, sqlite3 *db, int *err);
VocagtkAlbum *vocagtk_album_new_from_json(yyjson_val *json);

/*!
 * @brief Creates a new album object from JSON and saves it to the database.
 *
 * @param json
 *   A json object containing album information in VocaDB schema.
 * @param db
 *   The local database to save the album information.
 * @param sql_err
 *   Pointer to store SQL error code, can be NULL.
 *
 * @returns
 *   A new VocagtkAlbum instance.
 *   The data is owned by the caller of the function.
 */
VocagtkAlbum *vocagtk_album_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
);

int vocagtk_album_save_to_db(VocagtkAlbum const *self, sqlite3 *db);

/*!
 * @brief Gets the id of the album in VocaDB.
 *
 * @returns The id of the album.
 */
gint vocagtk_album_get_id(VocagtkAlbum *self);

/*!
 * @brief Gets the title of the album.
 *
 * @returns The title of the album.
 *          The data is owned by the instance.
 */
char const *vocagtk_album_get_title(VocagtkAlbum *self);

/*!
 * @brief Gets the artist string of the album.
 *
 * @returns The artist string of the album.
 *          The data is owned by the instance.
 */
char const *vocagtk_album_get_artist(VocagtkAlbum *self);

/*!
 * @brief Gets the url of cover image of the album.
 *
 * @returns The url of cover image of the album.
 *          The data is owned by the instance.
 */
char const *vocagtk_album_get_cover_url(VocagtkAlbum *self);

typedef struct _VocagtkAlbum {
    GObject parent_instance;
    VocagtkAlbumId id;
    GString *title;
    GString *artist;
    GString *cover_url;
} VocagtkAlbum;

G_END_DECLS
#endif

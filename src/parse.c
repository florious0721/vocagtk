#include "parse.h"
#include "entry.h"
#include "exterr.h"

/*!
 * @param json
 *   A json object containing album information in VocaDB schema,
 *   may cause fundamental error if schema is wrong.
 * @returns A new VocagtkAlbum instance.
 */
VocagtkAlbum *json_to_album(yyjson_val *json) {
    g_assert(yyjson_is_obj(json));

    VocagtkAlbumId id = -1;
    char const *title = NULL, *artist = NULL, *cover_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = yyjson_get_int(iter);

    iter = yyjson_obj_get(json, "name");
    title = yyjson_get_str(iter);

    iter = yyjson_obj_get(json, "artistString");
    artist = yyjson_get_str(iter);

    iter = yyjson_obj_get(json, "mainPicture");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (yyjson_is_str(thumb)) cover_url = yyjson_get_str(thumb);
    else cover_url = "https://vocadb.net/Content/unknown.png";

    return g_object_new(
        VOCAGTK_TYPE_ALBUM,
        "id", id,
        "title", title,
        "artist", artist,
        "cover-url", cover_url,
        NULL
    );
}

/*!
 * @param json
 *   A json object containing artist information in VocaDB schema,
 *   may cause fundamental error if schema is wrong.
 * @returns A new VocagtkArtist instance.
 */
VocagtkArtist *json_to_artist(yyjson_val *json) {
    g_assert(yyjson_is_obj(json));

    VocagtkArtistId id = -1;
    char const *name = NULL, *avatar_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = yyjson_get_int(iter);

    iter = yyjson_obj_get(json, "name");
    name = yyjson_get_str(iter);

    iter = yyjson_obj_get(json, "mainPicture");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (yyjson_is_str(thumb)) avatar_url = yyjson_get_str(thumb);
    else avatar_url = "https://vocadb.net/Content/unknown.png";

    return g_object_new(
        VOCAGTK_TYPE_ARTIST,
        "id", id,
        "name", name,
        "avatar-url", avatar_url,
        NULL
    );
}

VocagtkSong *json_to_song(yyjson_val *json) {
    g_assert(yyjson_is_obj(json));

    VocagtkSongId id = -1;
    char const *title = NULL, *artist = NULL, *image_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = yyjson_get_int(iter);

    iter = yyjson_obj_get(json, "name");
    title = yyjson_get_str(iter);

    iter = yyjson_obj_get(json, "artistString");
    artist = yyjson_get_str(iter);

    iter = yyjson_obj_get(json, "mainPicture");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (yyjson_is_str(thumb)) image_url = yyjson_get_str(thumb);
    else image_url = "https://vocadb.net/Content/unknown.png";

    return g_object_new(
        VOCAGTK_TYPE_SONG,
        "id", id,
        "title", title,
        "artist", artist,
        "image-url", image_url,
        NULL
    );
}

/*!
 * @brief Converts a VocaDB entry JSON object to a VocagtkEntry.
 *
 * The function checks the "entryType" field which may be "Album", "Artist"
 * or "Song" and dispatches to the corresponding json_to_* converter.
 * The created entity is then wrapped by vocagtk_entry_new_*.
 *
 * @param json
 *   A json object containing entry information in VocaDB schema.
 * @returns
 *   A new VocagtkEntry instance on success, or NULL if the entry type is
 *   missing/unknown.
 */
VocagtkEntry *json_to_entry(yyjson_val *json) {
    g_assert(yyjson_is_obj(json));

    yyjson_val *type_field = yyjson_obj_get(json, "entryType");
    if (!yyjson_is_str(type_field)) return NULL;

    char const *type_label = yyjson_get_str(type_field);
    if (!type_label) return NULL;

    if (g_strcmp0(type_label, "Album") == 0) {
        VocagtkAlbum *album = json_to_album(json);
        if (!album) return NULL; // unnecessary check, maybe removed
        return vocagtk_entry_new_album(album);
    }

    if (g_strcmp0(type_label, "Artist") == 0) {
        VocagtkArtist *artist = json_to_artist(json);
        if (!artist) return NULL;
        return vocagtk_entry_new_artist(artist);
    }

    if (g_strcmp0(type_label, "Song") == 0) {
        VocagtkSong *song = json_to_song(json);
        if (!song) return NULL;
        return vocagtk_entry_new_song(song);
    }

    vocagtk_warn_def("Unknown entryType: %s", type_label);
    return NULL;
}

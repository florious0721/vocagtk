#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <yyjson.h>

#include "album.h"
#include "data.h"
#include "exterr.h"
#include "helper.h"

enum {
    ALBUM_PROP_ID = 1,
    ALBUM_PROP_TITLE,
    ALBUM_PROP_ARTIST,
    ALBUM_PROP_COVER_URL,
};

G_DEFINE_TYPE(VocagtkAlbum, vocagtk_album, G_TYPE_OBJECT)

static void vocagtk_album_get_property(
    GObject *obj, guint propid,
    GValue *value, GParamSpec *spec
) {
    VocagtkAlbum *self = VOCAGTK_ALBUM(obj);

    switch (propid) {
    case ALBUM_PROP_ID:
        g_value_set_int(value, self->id);
        break;
    case ALBUM_PROP_TITLE:
        g_value_set_string(value, self->title->str);
        break;
    case ALBUM_PROP_ARTIST:
        g_value_set_string(value, self->artist->str);
        break;
    case ALBUM_PROP_COVER_URL:
        g_value_set_string(value, self->cover_url->str);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        // maybe we also need to call base's property method
        break;
    }
}

static void vocagtk_album_set_property(
    GObject *obj, guint propid,
    GValue const *value, GParamSpec *spec
) {
    VocagtkAlbum *self = VOCAGTK_ALBUM(obj);

    switch (propid) {
    case ALBUM_PROP_ID:
        self->id = g_value_get_int(value);
        break;
    case ALBUM_PROP_TITLE:
        g_string_assign(self->title, g_value_get_string(value));
        break;
    case ALBUM_PROP_ARTIST:
        g_string_assign(self->artist, g_value_get_string(value));
        break;
    case ALBUM_PROP_COVER_URL:
        g_string_assign(self->cover_url, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        // maybe we also need to call base's property method
        break;
    }
}

static void vocagtk_album_init(VocagtkAlbum *self) {
    self->title = g_string_new(NULL);
    self->artist = g_string_new(NULL);
    self->cover_url = g_string_new(NULL);
}

static void vocagtk_album_finalize(GObject *obj) {
    VocagtkAlbum *self = VOCAGTK_ALBUM(obj);
    g_string_free(self->title, true);
    g_string_free(self->artist, true);
    g_string_free(self->cover_url, true);
    G_OBJECT_CLASS(vocagtk_album_parent_class)->finalize(obj);
}

static void vocagtk_album_class_init(VocagtkAlbumClass *klass) {
    char const *const default_failure = "获取专辑信息失败(〒︿〒)";

    GObjectClass *c = G_OBJECT_CLASS(klass);
    c->get_property = vocagtk_album_get_property;
    c->set_property = vocagtk_album_set_property;
    c->finalize = vocagtk_album_finalize;

    GParamSpec *spec = NULL;

    spec = g_param_spec_int(
        "id", "Id", "The id of the album in VocaDB",
        INT_MIN, INT_MAX, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ALBUM_PROP_ID, spec);

    spec = g_param_spec_string(
        "title", "Title", NULL, default_failure,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ALBUM_PROP_TITLE, spec);

    spec = g_param_spec_string(
        "artist", "Artist", NULL, default_failure,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ALBUM_PROP_ARTIST, spec);

    spec = g_param_spec_string(
        "cover-url", "Cover Url", "Album's cover image url in VocaDB.",
        "https://vocadb.net/Content/unknown.png",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ALBUM_PROP_COVER_URL, spec);
}

/*!
 *
 * @param id
 * @param db
 * @param err sqlite error code
 * @returns
 *   A new VocagtkAlbum instance if found in database,
 *   or NULL when error occurs or not found.
 */
VocagtkAlbum *vocagtk_album_new_from_db(
    gint id, sqlite3 *db, int *err
) {
    DEBUG("Try to read from db.");
    char const *title = NULL, *artist = NULL, *cover_url = NULL;

    char const *sql =
        "SELECT title, artist, cover_url FROM album WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;

    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_rcode(rcode);
        if (err) *err = rcode;
        return NULL;
    }

    rcode = sqlite3_bind_int(stmt, 1, id);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_rcode(rcode);
        if (err) *err = rcode;
        return NULL;
    }

    do {
        // columns will be destroyed each step, so do it right away
        rcode = sqlite3_step(stmt);
        if (rcode != SQLITE_ROW) break;
        title = sqlite3_column_str(stmt, 0);
        artist = sqlite3_column_str(stmt, 1);
        cover_url = sqlite3_column_str(stmt, 2);
    } while (0);

    VocagtkAlbum *album = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No album found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return album;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        album = g_object_new(
            VOCAGTK_TYPE_ALBUM,
            "id", id,
            "title", title,
            "artist", artist,
            "cover-url", cover_url,
            NULL
        );
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return album;
    default:
        rcode = sqlite3_finalize(stmt);
        vocagtk_warn_sql_rcode(rcode);
        if (err) *err = rcode;
        return album;
    }
}

/*!
 * @param json
 *   A json object containing album information in VocaDB schema,
 *   may cause fundamental error if schema is wrong.
 * @returns A new VocagtkAlbum instance.
 */
VocagtkAlbum *vocagtk_album_new_from_json(yyjson_val *json) {
    g_assert(json->tag != YYJSON_TYPE_OBJ);

    VocagtkAlbumId id = -1;
    char const *title = NULL, *artist = NULL, *cover_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = iter->uni.i64;

    iter = yyjson_obj_get(json, "name");
    title = iter->uni.str;

    iter = yyjson_obj_get(json, "artistString");
    artist = iter->uni.str;

    iter = yyjson_obj_get(json, "mainPicture");
    iter = yyjson_obj_get(iter, "urlSmallThumb");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (thumb) cover_url = iter->uni.str;
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

int vocagtk_album_save_to_db(
    VocagtkAlbum const *self,
    sqlite3 *db
) {
    char const *sql =
        "REPLACE INTO album(id, title, artist, cover_url)"
        "VALUES(?, ?, ?, ?);";
    DEBUG("Save album %d to db.", self->id);

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, self->id);
    sqlite3_bind_text(stmt, 2, self->title->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, self->artist->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, self->cover_url->str, -1, SQLITE_STATIC);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkAlbum *vocagtk_album_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
) {
    VocagtkAlbum *album = vocagtk_album_new_from_json(json);

    int rcode = vocagtk_album_save_to_db(album, db);
    if (sql_err) *sql_err = rcode;

    return album;
}

/*!
 * @param db
 *   When network error happens, the album information is not saved into database.
 * @returns
 *   A new VocagtkAlbum instance.
 *   It may contain invalid information if network error happens,
 *   but it should never be NULL.
 */
static VocagtkAlbum *vocagtk_album_new_from_remote(
    gint id,
    CURL *handle, CURLcode *curl_err,
    sqlite3 *db, int *sql_err
) {
    DEBUG("Fetch from remote!");
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/albums/%d"
        "?fields=MainPicture&lang=Default", id
    );

    GByteArray *apibuf = g_byte_array_new();
    CURLcode rcode = 0;

    curl_easy_setopt(handle, CURLOPT_URL, urlbuf);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_g_byte_array_writer);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, apibuf);
    rcode = curl_easy_perform(handle);

    if (rcode != CURLE_OK) {
        vocagtk_warn_curl_rcode(rcode);
        if (curl_err) *curl_err = rcode;
        return g_object_new(VOCAGTK_TYPE_ALBUM, "id", id, NULL);
    }

    yyjson_doc *doc = NULL;
    doc = yyjson_read((char const *) apibuf->data, apibuf->len, 0);

    yyjson_val *root = yyjson_doc_get_root(doc);
    VocagtkAlbum *album = vocagtk_album_new_from_json(root);

    yyjson_doc_free(doc);
    g_byte_array_free(apibuf, true);

    int sqlrc = 0;
    sqlrc = vocagtk_album_save_to_db(album, db);
    if (!sql_err) *sql_err = sqlrc;

    return album;
}

VocagtkAlbum *vocagtk_album_new(gint id, sqlite3 *db, CURL *handle) {
    CURLcode curl_err = 0;
    int sql_err = 0;

    VocagtkAlbum *album = vocagtk_album_new_from_db(id, db, &sql_err);
    if (!album) {
        album = vocagtk_album_new_from_remote(
            id, handle, &curl_err, db, &sql_err
        );
    }
    return album;
}

VocagtkAlbumId vocagtk_album_get_id(VocagtkAlbum *self) {
    return self->id;
}

char const *vocagtk_album_get_title(VocagtkAlbum *self) {
    return self->title->str;
}

char const *vocagtk_album_get_artist(VocagtkAlbum *self) {
    return self->artist->str;
}

char const *vocagtk_album_get_cover_url(VocagtkAlbum *self) {
    return self->cover_url->str;
}

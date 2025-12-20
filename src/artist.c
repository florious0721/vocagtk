#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <yyjson.h>

#include "artist.h"
#include "data.h"
#include "exterr.h"
#include "helper.h"

enum {
    ARTIST_PROP_ID = 1,
    ARTIST_PROP_NAME,
    ARTIST_PROP_AVATAR_URL,
};

G_DEFINE_TYPE(VocagtkArtist, vocagtk_artist, G_TYPE_OBJECT)

static void vocagtk_artist_get_property(
    GObject *obj, guint propid,
    GValue *value, GParamSpec *spec
) {
    VocagtkArtist *self = VOCAGTK_ARTIST(obj);

    switch (propid) {
    case ARTIST_PROP_ID:
        g_value_set_int(value, self->id);
        break;
    case ARTIST_PROP_NAME:
        g_value_set_string(value, self->name->str);
        break;
    case ARTIST_PROP_AVATAR_URL:
        g_value_set_string(value, self->avatar_url->str);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        // maybe we also need to call base's property method
        break;
    }
}

static void vocagtk_artist_set_property(
    GObject *obj, guint propid,
    GValue const *value, GParamSpec *spec
) {
    VocagtkArtist *self = VOCAGTK_ARTIST(obj);

    switch (propid) {
    case ARTIST_PROP_ID:
        self->id = g_value_get_int(value);
        break;
    case ARTIST_PROP_NAME:
        g_string_assign(self->name, g_value_get_string(value));
        break;
    case ARTIST_PROP_AVATAR_URL:
        g_string_assign(self->avatar_url, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        // maybe we also need to call base's property method
        break;
    }
}

static void vocagtk_artist_init(VocagtkArtist *self) {
    self->name = g_string_new(NULL);
    self->avatar_url = g_string_new(NULL);
}

static void vocagtk_artist_finalize(GObject *obj) {
    VocagtkArtist *self = VOCAGTK_ARTIST(obj);
    g_string_free(self->name, true);
    g_string_free(self->avatar_url, true);
    G_OBJECT_CLASS(vocagtk_artist_parent_class)->finalize(obj);
}

static void vocagtk_artist_class_init(VocagtkArtistClass *klass) {
    char const *const default_failure = "获取艺术家信息失败(〒︿〒)";

    GObjectClass *c = G_OBJECT_CLASS(klass);
    c->get_property = vocagtk_artist_get_property;
    c->set_property = vocagtk_artist_set_property;
    c->finalize = vocagtk_artist_finalize;

    GParamSpec *spec = NULL;

    spec = g_param_spec_int(
        "id", "Id", "The id of the artist in VocaDB",
        INT_MIN, INT_MAX, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ARTIST_PROP_ID, spec);

    spec = g_param_spec_string(
        "name", "Name", NULL, default_failure,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ARTIST_PROP_NAME, spec);

    spec = g_param_spec_string(
        "avatar-url", "Avatar Url", "Artist's avatar image url in VocaDB.",
        "https://vocadb.net/Content/unknown.png",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ARTIST_PROP_AVATAR_URL, spec);
}

/*!
 *
 * @param id
 * @param db
 * @param err sqlite error code
 * @returns
 *   A new VocagtkArtist instance if found in database,
 *   or NULL when error occurs or not found.
 */
VocagtkArtist *vocagtk_artist_new_from_db(
    gint id, sqlite3 *db, int *err
) {
    DEBUG("Try to read from db.");
    char const *name = NULL, *avatar_url = NULL;

    char const *sql =
        "SELECT name, avatar_url FROM artist WHERE id = ?;";
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
        rcode = sqlite3_step(stmt);
        if (rcode != SQLITE_ROW) break;
        name = sqlite3_column_str(stmt, 0);
        avatar_url = sqlite3_column_str(stmt, 1);
    } while (0);

    VocagtkArtist *artist = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No artist found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return artist;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        artist = g_object_new(
            VOCAGTK_TYPE_ARTIST,
            "id", id,
            "name", name,
            "avatar-url", avatar_url,
            NULL
        );
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return artist;
    default:
        rcode = sqlite3_finalize(stmt);
        vocagtk_warn_sql_rcode(rcode);
        if (err) *err = rcode;
        return artist;
    }
}

/*!
 * @param json
 *   A json object containing artist information in VocaDB schema,
 *   may cause fundamental error if schema is wrong.
 * @returns A new VocagtkArtist instance.
 */
VocagtkArtist *vocagtk_artist_new_from_json(yyjson_val *json) {
    g_assert(json->tag != YYJSON_TYPE_OBJ);

    VocagtkArtistId id = -1;
    char const *name = NULL, *avatar_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = iter->uni.i64;

    iter = yyjson_obj_get(json, "name");
    name = iter->uni.str;

    iter = yyjson_obj_get(json, "mainPicture");
    iter = yyjson_obj_get(iter, "urlSmallThumb");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (thumb) avatar_url = iter->uni.str;
    else avatar_url = "https://vocadb.net/Content/unknown.png";


    return g_object_new(
        VOCAGTK_TYPE_ARTIST,
        "id", id,
        "name", name,
        "avatar-url", avatar_url,
        NULL
    );
}

int vocagtk_artist_save_to_db(
    VocagtkArtist const *self,
    sqlite3 *db
) {
    char const *sql =
        "REPLACE INTO artist(id, name, avatar_url)"
        "VALUES(?, ?, ?);";
    DEBUG("Save album %d to db.", self->id);

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, self->id);
    sqlite3_bind_text(stmt, 2, self->name->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, self->avatar_url->str, -1, SQLITE_STATIC);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkArtist *vocagtk_artist_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
) {
    VocagtkArtist *artist = vocagtk_artist_new_from_json(json);

    int rcode = vocagtk_artist_save_to_db(artist, db);
    if (sql_err) *sql_err = rcode;
    return artist;
}

/*!
 * @param db
 *   When network error happens, the artist information is not saved into database.
 * @returns
 *   A new VocagtkArtist instance.
 *   It may contain invalid information if network error happens,
 *   but it should never be NULL.
 */
static VocagtkArtist *vocagtk_artist_new_from_remote(
    gint id,
    CURL *handle, CURLcode *curl_err,
    sqlite3 *db, int *sql_err
) {
    DEBUG("Fetch from remote!");
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/artists/%d"
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
        return g_object_new(VOCAGTK_TYPE_ARTIST, "id", id, NULL);
    }

    yyjson_doc *doc = NULL;
    doc = yyjson_read((char const *) apibuf->data, apibuf->len, 0);

    yyjson_val *root = yyjson_doc_get_root(doc);
    VocagtkArtist *artist = vocagtk_artist_new_from_json(root);

    yyjson_doc_free(doc);
    g_byte_array_free(apibuf, true);

    int sqlrc = 0;
    sqlrc = vocagtk_artist_save_to_db(artist, db);
    if (!sql_err) *sql_err = sqlrc;

    return artist;
}

VocagtkArtist *vocagtk_artist_new(gint id, sqlite3 *db, CURL *handle) {
    CURLcode curl_err = 0;
    int sql_err = 0;

    VocagtkArtist *artist = vocagtk_artist_new_from_db(id, db, &sql_err);
    if (!artist) {
        artist = vocagtk_artist_new_from_remote(
            id, handle, &curl_err, db, &sql_err
        );
    }
    return artist;
}

VocagtkArtistId vocagtk_artist_get_id(VocagtkArtist *self) {
    return self->id;
}

char const *vocagtk_artist_get_name(VocagtkArtist *self) {
    return self->name->str;
}

char const *vocagtk_artist_get_avatar_url(VocagtkArtist *self) {
    return self->avatar_url->str;
}

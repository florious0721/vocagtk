#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <yyjson.h>

#include "song.h"
#include "data.h"
#include "exterr.h"
#include "helper.h"

enum {
    SONG_PROP_ID = 1,
    SONG_PROP_TITLE,
    SONG_PROP_ARTIST,
    SONG_PROP_IMAGE_URL,
};

G_DEFINE_TYPE(VocagtkSong, vocagtk_song, G_TYPE_OBJECT)

static void vocagtk_song_get_property(
    GObject *obj, guint propid,
    GValue *value, GParamSpec *spec
) {
    VocagtkSong *self = VOCAGTK_SONG(obj);

    switch (propid) {
    case SONG_PROP_ID:
        g_value_set_int(value, self->id);
        break;
    case SONG_PROP_TITLE:
        g_value_set_string(value, self->title->str);
        break;
    case SONG_PROP_ARTIST:
        g_value_set_string(value, self->artist->str);
        break;
    case SONG_PROP_IMAGE_URL:
        g_value_set_string(value, self->image_url->str);
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        break;
    }
}

static void vocagtk_song_set_property(
    GObject *obj, guint propid,
    GValue const *value, GParamSpec *spec
) {
    VocagtkSong *self = VOCAGTK_SONG(obj);

    switch (propid) {
    case SONG_PROP_ID:
        self->id = g_value_get_int(value);
        break;
    case SONG_PROP_TITLE:
        g_string_assign(self->title, g_value_get_string(value));
        break;
    case SONG_PROP_ARTIST:
        g_string_assign(self->artist, g_value_get_string(value));
        break;
    case SONG_PROP_IMAGE_URL:
        g_string_assign(self->image_url, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        break;
    }
}

static void vocagtk_song_init(VocagtkSong *self) {
    self->title = g_string_new(NULL);
    self->artist = g_string_new(NULL);
    self->image_url = g_string_new(NULL);
}

static void vocagtk_song_finalize(GObject *obj) {
    VocagtkSong *self = VOCAGTK_SONG(obj);
    g_string_free(self->title, true);
    g_string_free(self->artist, true);
    g_string_free(self->image_url, true);
    G_OBJECT_CLASS(vocagtk_song_parent_class)->finalize(obj);
}

static void vocagtk_song_class_init(VocagtkSongClass *klass) {
    char const *const default_failure = "获取歌曲信息失败(〒︿〒)";

    GObjectClass *c = G_OBJECT_CLASS(klass);
    c->get_property = vocagtk_song_get_property;
    c->set_property = vocagtk_song_set_property;
    c->finalize = vocagtk_song_finalize;

    GParamSpec *spec = NULL;

    spec = g_param_spec_int(
        "id", "Id", "The id of the song in VocaDB",
        INT_MIN, INT_MAX, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_ID, spec);

    spec = g_param_spec_string(
        "title", "Title", NULL, default_failure,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_TITLE, spec);

    spec = g_param_spec_string(
        "artist", "Artist", NULL, default_failure,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_ARTIST, spec);

    spec = g_param_spec_string(
        "image_url", "Image url", NULL, "https://vocadb.net/Content/unknown.png",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_IMAGE_URL, spec);
}

VocagtkSong *vocagtk_song_new_from_db(
    gint id, sqlite3 *db, int *err
) {
    DEBUG("Try to read from db.");
    char const *title = NULL, *artist = NULL, *image_url = NULL;

    char const *sql =
        "SELECT title, artist, image_url, release_date FROM song WHERE id = ?;";
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
        title = sqlite3_column_str(stmt, 0);
        artist = sqlite3_column_str(stmt, 1);
        image_url = sqlite3_column_str(stmt, 2);
    } while (0);

    VocagtkSong *song = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No song found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return song;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        song = g_object_new(
            VOCAGTK_TYPE_SONG,
            "id", id,
            "title", title,
            "artist", artist,
            "image_url", image_url,
            NULL
        );
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return song;
    default:
        rcode = sqlite3_finalize(stmt);
        vocagtk_warn_sql_rcode(rcode);
        if (err) *err = rcode;
        return song;
    }
}

VocagtkSong *vocagtk_song_new_from_json(yyjson_val *json) {
    g_assert(json->tag != YYJSON_TYPE_OBJ);

    VocagtkSongId id = -1;
    char const *title = NULL, *artist = NULL, *image_url = NULL;

    yyjson_val *iter = NULL;
    iter = yyjson_obj_get(json, "id");
    id = iter->uni.i64;

    iter = yyjson_obj_get(json, "name");
    title = iter->uni.str;

    iter = yyjson_obj_get(json, "artistString");
    artist = iter->uni.str;

    iter = yyjson_obj_get(json, "mainPicture");
    yyjson_val *thumb = yyjson_obj_get(iter, "urlSmallThumb");
    if (!thumb) thumb = yyjson_obj_get(iter, "urlThumb");
    if (thumb) image_url = iter->uni.str;
    else image_url = "https://vocadb.net/Content/unknown.png";

    return g_object_new(
        VOCAGTK_TYPE_SONG,
        "id", id,
        "title", title,
        "artist", artist,
        "image_url", image_url,
        NULL
    );
}

int vocagtk_song_save_to_db(
    VocagtkSong const *self,
    sqlite3 *db
) {
    char const *sql =
        "REPLACE INTO song(id, title, artist)"
        "VALUES(?, ?, ?);";
    DEBUG("Save album %d to db.", self->id);

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, self->id);
    sqlite3_bind_text(stmt, 2, self->title->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, self->artist->str, -1, SQLITE_STATIC);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkSong *vocagtk_song_new_from_json_with_db(
    yyjson_val *json, sqlite3 *db, int *sql_err
) {
    VocagtkSong *song = vocagtk_song_new_from_json(json);

    int rcode = vocagtk_song_save_to_db(song, db);
    if (sql_err) *sql_err = rcode;

    return song;
}

static VocagtkSong *vocagtk_song_new_from_remote(
    gint id,
    CURL *handle, CURLcode *curl_err,
    sqlite3 *db, int *sql_err
) {
    DEBUG("Fetch from remote!");
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/songs/%d"
        "?fields=ArtistString&lang=Default", id
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
        return g_object_new(VOCAGTK_TYPE_SONG, "id", id, NULL);
    }

    yyjson_doc *doc = NULL;
    doc = yyjson_read((char const *) apibuf->data, apibuf->len, 0);

    yyjson_val *root = yyjson_doc_get_root(doc);
    VocagtkSong *song = vocagtk_song_new_from_json(root);

    yyjson_doc_free(doc);
    g_byte_array_free(apibuf, true);

    int sqlrc = 0;
    sqlrc = vocagtk_song_save_to_db(song, db);
    if (!sql_err) *sql_err = sqlrc;

    return song;
}

VocagtkSong *vocagtk_song_new(VocagtkSongId id, sqlite3 *db, CURL *handle) {
    CURLcode curl_err = 0;
    int sql_err = 0;

    VocagtkSong *song = vocagtk_song_new_from_db(id, db, &sql_err);
    if (!song) {
        song = vocagtk_song_new_from_remote(
            id, handle, &curl_err, db, &sql_err
        );
    }
    return song;
}

VocagtkSongId vocagtk_song_get_id(VocagtkSong *self) {
    return self->id;
}

char const *vocagtk_song_get_title(VocagtkSong *self) {
    return self->title->str;
}

char const *vocagtk_song_get_artist(VocagtkSong *self) {
    return self->artist->str;
}

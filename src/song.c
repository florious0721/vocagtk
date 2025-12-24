#include <limits.h>
#include <sqlite3.h>
#include <yyjson.h>

#include "song.h"
#include "db.h"
#include "dl.h"
#include "parse.h"

#include "exterr.h"
#include "helper.h"


enum {
    SONG_PROP_ID = 1,
    SONG_PROP_TITLE,
    SONG_PROP_ARTIST,
    SONG_PROP_IMAGE_URL,
    SONG_PROP_PUBLISH_DATE,
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
        break;
    case SONG_PROP_PUBLISH_DATE:
        g_value_set_int64(value, self->publish_date);
        break;
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
    case SONG_PROP_PUBLISH_DATE:
        self->publish_date = g_value_get_int64(value);
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
    self->publish_date = -1;
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
        "image-url", "Image url", NULL, "https://vocadb.net/Content/unknown.png",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_IMAGE_URL, spec);

    spec = g_param_spec_int64(
        "publish-date", "Publish Date", "Song's publish date as unix timestamp.",
        0, G_MAXINT64, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, SONG_PROP_PUBLISH_DATE, spec);
}

VocagtkSong *vocagtk_song_new(VocagtkSongId id, AppState *ctx) {
    if (!ctx) return g_object_new(VOCAGTK_TYPE_SONG, "id", id, NULL);

    // 1) Try local DB cache first.
    int sql_err = SQLITE_OK;
    VocagtkSong *song = NULL;
    if (ctx->db) {
        song = db_song_get_by_id(ctx->db, id, &sql_err);
        if (song) return song;
        if (sql_err != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }
    }

    // 2) Not in DB: fetch from remote.
    CURLcode curl_err = CURLE_OK;
    yyjson_doc *doc = vocagtk_downloader_song(&ctx->dl, id);
    if (!doc) {
        // Network or parse error already logged by downloader.
        return g_object_new(VOCAGTK_TYPE_SONG, "id", id, NULL);
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    song = json_to_song(root);

    // 3) Save to DB as cache (best-effort).
    if (ctx->db && song) {
        sql_err = db_song_add(ctx->db, song);
        if (sql_err != SQLITE_OK) {
            vocagtk_warn_sql_db(ctx->db);
        }

        // Also persist relations if present.
        db_insert_song_albums(ctx->db, root, NULL);
        db_insert_song_artists(ctx->db, root, NULL);
    }

    yyjson_doc_free(doc);
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

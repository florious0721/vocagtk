#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <yyjson.h>

#include "artist.h"
#include "db.h"
#include "dl.h"
#include "exterr.h"
#include "helper.h"
#include "parse.h"



enum {
    ARTIST_PROP_ID = 1,
    ARTIST_PROP_NAME,
    ARTIST_PROP_AVATAR_URL,
    ARTIST_PROP_UPDATE_AT,
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
    case ARTIST_PROP_UPDATE_AT:
        g_value_set_int64(value, self->update_at);
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
    case ARTIST_PROP_UPDATE_AT:
        self->update_at = g_value_get_int64(value);
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
    self->update_at = 0;
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

    spec = g_param_spec_int64(
        "update-at", "Update At", "The timestamp when the artist was last updated.",
        0, G_MAXINT64, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ARTIST_PROP_UPDATE_AT, spec);
}



VocagtkArtist *vocagtk_artist_new(VocagtkArtistId id, AppState *ctx) {
    int sql_err = SQLITE_OK;
    VocagtkArtist *artist = db_artist_get_by_id(ctx->db, id, &sql_err);
    if (artist) return artist;

    yyjson_doc *doc = vocagtk_downloader_artist(&ctx->dl, id);
    if (!doc) {
        return g_object_new(VOCAGTK_TYPE_ARTIST, "id", id, NULL);
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    artist = json_to_artist(root);
    yyjson_doc_free(doc);

    if (!artist) {
        return g_object_new(VOCAGTK_TYPE_ARTIST, "id", id, NULL);
    }

    sql_err = db_artist_add(ctx->db, artist);
    if (sql_err != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
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

time_t vocagtk_artist_get_update_at(VocagtkArtist *self) {
    return self->update_at;
}

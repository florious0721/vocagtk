#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <yyjson.h>

#include "album.h"
#include "db.h"
#include "dl.h"
#include "exterr.h"
#include "helper.h"
#include "parse.h"


enum {
    ALBUM_PROP_ID = 1,
    ALBUM_PROP_TITLE,
    ALBUM_PROP_ARTIST,
    ALBUM_PROP_COVER_URL,
    ALBUM_PROP_PUBLISH_DATE,
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
    case ALBUM_PROP_PUBLISH_DATE:
        g_value_set_int64(value, self->publish_date);
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
    case ALBUM_PROP_PUBLISH_DATE:
        self->publish_date = g_value_get_int64(value);
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
    self->publish_date = -1;
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

    spec = g_param_spec_int64(
        "publish-date", "Publish Date", "Album's publish date as unix timestamp.",
        0, G_MAXINT64, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
    );
    g_object_class_install_property(c, ALBUM_PROP_PUBLISH_DATE, spec);
}

VocagtkAlbum *vocagtk_album_new(VocagtkAlbumId id, AppState *ctx) {
    int sql_err = SQLITE_OK;
    VocagtkAlbum *album = db_album_get_by_id(ctx->db, id, &sql_err);

    if (album) return album;

    yyjson_doc *doc = vocagtk_downloader_album(&ctx->dl, id);
    if (!doc) {
        // Network/parse error: keep behaviour similar to other entities: return minimal object.
        return g_object_new(VOCAGTK_TYPE_ALBUM, "id", id, NULL);
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    album = json_to_album(root);
    yyjson_doc_free(doc);

    if (!album) {
        return g_object_new(VOCAGTK_TYPE_ALBUM, "id", id, NULL);
    }

    sql_err = db_album_add(ctx->db, album);
    if (sql_err != SQLITE_OK) {
        vocagtk_warn_sql_db(ctx->db);
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

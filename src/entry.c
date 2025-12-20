#include <assert.h>
#include <glib-object.h>

#include "album.h"
#include "artist.h"
#include "entry.h"
#include "song.h"

enum {
    ENTRY_PROP_TYPE_LABEL = 1,
    ENTRY_PROP_IMAGE,
    ENTRY_PROP_MAIN_INFO,
    ENTRY_PROP_SUB_INFO,
    ENTRY_PROP_ID,
};

G_DEFINE_TYPE(VocagtkEntry, vocagtk_entry, G_TYPE_OBJECT)

static void vocagtk_entry_init(VocagtkEntry *self) {
    self->type_label = 0;
    self->entry.album = NULL;
}

static void vocagtk_entry_finalize(GObject *obj) {
    VocagtkEntry *self = VOCAGTK_ENTRY(obj);

    if (self->entry.album) {
        g_object_unref(self->entry.album);
    }

    G_OBJECT_CLASS(vocagtk_entry_parent_class)->finalize(obj);
}

static void vocagtk_entry_get_property(
    GObject *obj, guint propid,
    GValue *value, GParamSpec *spec
) {
    VocagtkEntry *self = VOCAGTK_ENTRY(obj);

    switch (propid) {
    case ENTRY_PROP_TYPE_LABEL:
        g_value_set_int(value, self->type_label);
        break;
    case ENTRY_PROP_IMAGE:
        g_value_set_string(value, vocagtk_entry_get_image(self));
        break;
    case ENTRY_PROP_MAIN_INFO:
        g_value_set_string(value, vocagtk_entry_get_main_info(self));
        break;
    case ENTRY_PROP_SUB_INFO:
        g_value_set_string(value, vocagtk_entry_get_sub_info(self));
        break;
    case ENTRY_PROP_ID:
        g_value_set_int(value, vocagtk_entry_get_id(self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(self, propid, spec);
        break;
    }
}

static void vocagtk_entry_class_init(VocagtkEntryClass *klass) {
    GObjectClass *c = G_OBJECT_CLASS(klass);
    c->get_property = vocagtk_entry_get_property;
    c->finalize = vocagtk_entry_finalize;

    GParamSpec *spec = NULL;
    spec = g_param_spec_int(
        "type-label", "Type label",
        "Type label indicates whether the entry is an album, artist or song.",
        VOCAGTK_ENTRY_TYPE_LABEL_ALBUM, VOCAGTK_ENTRY_TYPE_LABEL_SONG, 0,
        G_PARAM_READABLE
    );
    g_object_class_install_property(c, ENTRY_PROP_TYPE_LABEL, spec);

    spec = g_param_spec_string(
        "image", "Image url",
        "The image url for the entry.",
        NULL, G_PARAM_READABLE
    );
    g_object_class_install_property(c, ENTRY_PROP_IMAGE, spec);

    spec = g_param_spec_string(
        "main-info", "Main info",
        "The main information for the entry. For album and song, it's title."
        "For artist, it's artist's name.",
        NULL, G_PARAM_READABLE
    );
    g_object_class_install_property(c, ENTRY_PROP_MAIN_INFO, spec);

    spec = g_param_spec_string(
        "sub-info", "Sub info",
        "The sub information for the entry. For album and song, it's artist."
        "For artist, it's an empty string.",
        NULL, G_PARAM_READABLE
    );
    g_object_class_install_property(c, ENTRY_PROP_SUB_INFO, spec);

    spec = g_param_spec_int(
        "id", "Id",
        "The id of the underlying album, artist or song. You should use this with type label.",
        INT_MIN, INT_MAX, 0,
        G_PARAM_READABLE
    );
    g_object_class_install_property(c, ENTRY_PROP_ID, spec);
}

VocagtkEntry *vocagtk_entry_new_album(VocagtkAlbum *album) {
    assert(VOCAGTK_ALBUM(album));

    VocagtkEntry *self = g_object_new(VOCAGTK_TYPE_ENTRY, NULL);
    self->type_label = VOCAGTK_ENTRY_TYPE_LABEL_ALBUM;
    self->entry.album = album;
    return self;
}

VocagtkEntry *vocagtk_entry_new_artist(VocagtkArtist *artist) {
    assert(VOCAGTK_ARTIST(artist));

    VocagtkEntry *self = g_object_new(VOCAGTK_TYPE_ENTRY, NULL);
    self->type_label = VOCAGTK_ENTRY_TYPE_LABEL_ARTIST;
    self->entry.artist = artist;
    return self;
}

VocagtkEntry *vocagtk_entry_new_song(VocagtkSong *song) {
    assert(VOCAGTK_SONG(song));

    VocagtkEntry *self = g_object_new(VOCAGTK_TYPE_ENTRY, NULL);
    self->type_label = VOCAGTK_ENTRY_TYPE_LABEL_SONG;
    self->entry.song = song;
    return self;
}

VocagtkEntryTypeLabel vocagtk_entry_get_type_label(VocagtkEntry *self) {
    return self->type_label;
}

char const *vocagtk_entry_get_image(VocagtkEntry *self) {
    switch (self->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        return vocagtk_album_get_cover_url(self->entry.album);
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        return vocagtk_artist_get_avatar_url(self->entry.artist);
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        // TODO: add image url for song (the album )
        return "https://vocadb.net/Content/unknown.png";
    default:
        g_abort();
        return NULL;
    }
}

char const *vocagtk_entry_get_main_info(VocagtkEntry *self) {
    switch (self->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        return vocagtk_album_get_title(self->entry.album);
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        return vocagtk_artist_get_name(self->entry.artist);
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        return vocagtk_song_get_title(self->entry.song);
    default:
        g_abort();
        return NULL;
    }
}

char const *vocagtk_entry_get_sub_info(VocagtkEntry *self) {
    switch (self->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        return vocagtk_album_get_artist(self->entry.album);
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        return "";
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        return vocagtk_song_get_artist(self->entry.song);
    default:
        g_abort();
        return NULL;
    }
}

gint vocagtk_entry_get_id(VocagtkEntry *self) {
    switch (self->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        return vocagtk_album_get_id(self->entry.album);
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        return vocagtk_artist_get_id(self->entry.artist);
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        return vocagtk_song_get_id(self->entry.song);
    default:
        g_abort();
        return -1;
    }
}

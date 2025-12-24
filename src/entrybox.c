#include "db.h"
#include "entrybox.h"
#include "exterr.h"
#include "helper.h"
#include "ui.h"

G_DEFINE_TYPE(VocagtkEntryBox, vocagtk_entry_box, GTK_TYPE_BOX)

static void watch(GtkButton *_, VocagtkEntryBox *box) {
    int id = -1;
    DEBUG("Clicked entry at %d", gtk_list_item_get_position(box->item));
    switch (box->entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        DEBUG("Album add not implemented.");
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        id = box->entry->entry.artist->id;
        int sql_err = SQLITE_OK;
        int inserted = db_rss_add_artist(
            box->list->app->db,
            box->entry->entry.artist->id,
            &sql_err
        );

        if (sql_err != SQLITE_OK) {
            DEBUG("Failed to add artist %d to RSS, error code: %d", id,
                sql_err);
            break;
        }

        VocagtkArtist *artist = vocagtk_artist_new(id, box->list->app);

        if (inserted > 0) {
            // Newly inserted, add to UI list
            VocagtkEntry *entry = vocagtk_entry_new_artist(artist);
            g_list_store_append(box->list->app->rss_artist, entry);
            g_object_unref(entry);
        } else {
            DEBUG("Artist %d already subscribed", id);
        }

        // Always update artist's songs and refresh RSS song list
        // (artist may have new songs even if already subscribed)
        update_artist(box->list->app, artist);
        refresh_rss_song(box->list->app);

        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        DEBUG("Song add not implemented.");
    default:
        break;
    }
}

static void unwatch(GtkButton *_, VocagtkEntryBox *box) {
    int position = gtk_list_item_get_position(box->item);
    DEBUG("Trying to remove entry at %d", position);

    switch (box->entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        DEBUG("Album remove not implemented.");
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        db_rss_remove_artist(box->list->app->db, box->entry->entry.artist->id);
        g_list_store_remove(box->list->store, position);
        // Refresh RSS song list after removing artist
        refresh_rss_song(box->list->app);
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        DEBUG("Song remove not implemented.");
        break;
    default:
        break;
    }
}

static void view_src(GtkButton *_, VocagtkEntryBox *box) {}

static void vocagtk_entry_box_dispose(GObject *obj) {
    gtk_widget_dispose_template(GTK_WIDGET(obj), VOCAGTK_TYPE_ENTRY_BOX);
    G_OBJECT_CLASS(vocagtk_entry_box_parent_class)->dispose(obj);
}

static void vocagtk_entry_box_class_init(VocagtkEntryBoxClass *klass) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    G_OBJECT_CLASS(klass)->dispose = vocagtk_entry_box_dispose;

    gtk_widget_class_set_template_from_resource(
        widget_class,
        "/moe/florious0721/vocagtk/ui/entry.ui"
    );

    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, image
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, type_label
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, main_info
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, sub_info
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, add_or_remove
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, view_src
    );
}

static void vocagtk_entry_box_init(VocagtkEntryBox *self) {
    gtk_widget_init_template(GTK_WIDGET(self));
    self->list = NULL;
    self->entry = NULL;
    self->item = NULL;
}

void vocagtk_entry_box_bind(VocagtkEntryBox *self, VocagtkEntry *entry) {
    self->entry = entry;

    char const *label = NULL;
    switch (entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        label = "💽";
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        label = "👤";
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        label = "🎵";
        break;
    default:
        vocagtk_log(
            "VOCAGTK", G_LOG_LEVEL_ERROR, "Unknown type label %d in %s",
            entry->type_label, vocagtk_entry_get_main_info(entry)
        );
        break;
    }
    gtk_label_set_label(self->type_label, label);

    gtk_image_set_from_file(self->image, "example/unknown.png");
    gtk_label_set_label(self->main_info, vocagtk_entry_get_main_info(entry));
    gtk_label_set_label(self->sub_info, vocagtk_entry_get_sub_info(entry));
}

VocagtkEntryBox *vocagtk_entry_box_new(
    bool is_remove,
    EntryListCtx *list, VocagtkEntry *entry, GtkListItem *item
) {
    VocagtkEntryBox *self = g_object_new(VOCAGTK_TYPE_ENTRY_BOX, NULL);
    self->list = list;
    self->item = item;
    self->entry = entry;

    if (is_remove) {
        gtk_button_set_label(self->add_or_remove, "Remove/Unsubscribe 🚫");
        g_signal_connect(self->add_or_remove, "clicked", G_CALLBACK(unwatch),
            self);
    } else {
        g_signal_connect(self->add_or_remove, "clicked", G_CALLBACK(watch),
            self);
    }

    g_signal_connect(self->view_src, "clicked", G_CALLBACK(view_src), self);

    if (entry) vocagtk_entry_box_bind(self, entry);
    return self;
}

#include "db.h"
#include "entrybox.h"
#include "exterr.h"
#include "helper.h"
#include "ui.h"

G_DEFINE_TYPE(VocagtkEntryBox, vocagtk_entry_box, GTK_TYPE_BOX)

static void watch(GtkButton *_, VocagtkEntryBox *box) {
    int position = gtk_list_item_get_position(box->item);
    watch_entry(box->entry, box->list, position);
}

static void unwatch(GtkButton *_, VocagtkEntryBox *box) {
    int position = gtk_list_item_get_position(box->item);
    unwatch_entry(box->entry, box->list, position);
}

static void view_src(GtkButton *_, VocagtkEntryBox *box) {
    int id = -1;
    char url[128] = {0};
    switch (box->entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        id = box->entry->entry.album->id;
        snprintf(url, sizeof(url), "https://vocadb.net/Al/%d", id);
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        id = box->entry->entry.artist->id;
        snprintf(url, sizeof(url), "https://vocadb.net/Ar/%d", id);
        break;
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        id = box->entry->entry.song->id;
        snprintf(url, sizeof(url), "https://vocadb.net/S/%d", id);
        break;
    default:
        DEBUG("Unknown entry type for view_src");
        return;
    }
    DEBUG("Opening VocaDB URL: %s", url);
    GtkUriLauncher *launcher = gtk_uri_launcher_new(url);
    gtk_uri_launcher_launch(launcher, NULL, NULL, NULL, NULL);
    g_object_unref(launcher);
}

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

    // Get image URL from entry
    char const *image_url = vocagtk_entry_get_image(entry);
    char const *fallback_image = "example/unknown.png";

    do {
        if (!image_url) {
            // No image URL available, use fallback
            gtk_image_set_from_file(self->image, fallback_image);
            break;
        }

        // Extract filename from URL
        char const *filename = image_url + strlen(image_url);
        while (*filename != '/' && filename != image_url) filename--;

        if (filename == image_url) {
            // Failed to extract filename, use fallback
            DEBUG("Failed to extract filename from URL: %s", image_url);
            gtk_image_set_from_file(self->image, fallback_image);
            break;
        }

        // Build cache file path
        GString *cache_path =
            g_string_new(self->list->app->dl.cache_path);
        g_string_append(cache_path, filename);

        // Download image to cache if not exists
        CURLcode curl_err = vocagtk_downloader_image(
            &self->list->app->dl, image_url, cache_path->str
        );

        if (curl_err != CURLE_OK) {
            // Download failed, use fallback
            DEBUG(
                "Failed to download image from %s, error: %s",
                image_url, curl_easy_strerror(curl_err)
            );
            gtk_image_set_from_file(self->image, fallback_image);
            g_string_free(cache_path, TRUE);
            break;
        }

        // Load image as GdkTexture
        GError *error = NULL;
        GdkTexture *texture = gdk_texture_new_from_filename(cache_path->str, &error);

        if (!texture) {
            // Failed to load texture, use fallback
            DEBUG(
                "Failed to load texture from %s: %s",
                cache_path->str, error ? error->message : "unknown error"
            );
            if (error) g_error_free(error);
            gtk_image_set_from_file(self->image, fallback_image);
            g_string_free(cache_path, TRUE);
            break;
        }

        // Successfully loaded texture, set image
        gtk_image_set_from_paintable(self->image, GDK_PAINTABLE(texture));
        g_object_unref(texture);
        //DEBUG("Loaded image texture from cache: %s", cache_path->str);
        g_string_free(cache_path, TRUE);
    } while (0);

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
        g_signal_connect(
            self->add_or_remove, "clicked",
            G_CALLBACK(unwatch), self
        );
    } else {
        g_signal_connect(
            self->add_or_remove, "clicked",
            G_CALLBACK(watch), self
        );
    }

    g_signal_connect(self->view_src, "clicked", G_CALLBACK(view_src), self);

    if (entry) vocagtk_entry_box_bind(self, entry);
    return self;
}

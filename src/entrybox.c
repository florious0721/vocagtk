#include "entrybox.h"
#include "exterr.h"
#include "helper.h"

G_DEFINE_TYPE(VocagtkEntryBox, vocagtk_entry_box, GTK_TYPE_BOX)

static void watch(GtkButton *_,  VocagtkEntryBox *box) {}

static void view_src(GtkButton *_, VocagtkEntryBox *box) {}

static void vocagtk_entry_box_init(VocagtkEntryBox *self) {
    gtk_widget_init_template(GTK_WIDGET(self));
    self->entry = NULL;

    g_signal_connect(self->watch, "clicked", G_CALLBACK(watch), self);
    g_signal_connect(self->view_src, "clicked", G_CALLBACK(view_src), self);
    // Only once to avoid double free
    g_signal_connect(self->watch, "destroy", G_CALLBACK(free_signal_wrapper), self);
}

static void vocagtk_entry_box_dispose(GObject *obj) {
    gtk_widget_dispose_template(GTK_WIDGET(obj), VOCAGTK_TYPE_ENTRY_BOX);
    G_OBJECT_CLASS(vocagtk_entry_box_parent_class)->dispose(obj);
}

static void vocagtk_entry_box_class_init(VocagtkEntryBoxClass *klass) {
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    G_OBJECT_CLASS(klass)->dispose = vocagtk_entry_box_dispose;

    // Adjust this path to match your compiled resource setup
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
        widget_class, VocagtkEntryBox, watch
    );
    gtk_widget_class_bind_template_child(
        widget_class, VocagtkEntryBox, view_src
    );
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

VocagtkEntryBox *vocagtk_entry_box_new(AppState *ctx, VocagtkEntry *entry) {
    VocagtkEntryBox *self = g_object_new(VOCAGTK_TYPE_ENTRY_BOX, NULL);

    self->app = ctx;
    if (entry) vocagtk_entry_box_bind(self, entry);
    return self;
}

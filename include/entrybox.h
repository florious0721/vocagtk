#ifndef _VOCAGTK_ENTRY_BOX_H
#define _VOCAGTK_ENTRY_BOX_H
#include <gtk/gtk.h>

#include "entry.h"
#include "ui.h"

// *INDENT-OFF*
G_BEGIN_DECLS

#define VOCAGTK_TYPE_ENTRY_BOX (vocagtk_entry_box_get_type())
G_DECLARE_FINAL_TYPE(
    VocagtkEntryBox,
    vocagtk_entry_box,
    VOCAGTK, ENTRY_BOX,
    GtkBox
)
// *INDENT-ON*

void vocagtk_entry_box_bind(VocagtkEntryBox *self, VocagtkEntry *entry);

VocagtkEntryBox *vocagtk_entry_box_new(
    bool is_remove,
    EntryListCtx *list, VocagtkEntry *entry, GtkListItem *item
);

typedef struct _VocagtkEntryBox {
    GtkWidget parent_instance;

    GtkImage  *image;
    GtkLabel  *type_label;
    GtkLabel  *main_info;
    GtkLabel  *sub_info;
    GtkButton *add_or_remove; // Add or subscribe
    GtkButton *view_src;

    EntryListCtx *list;
    VocagtkEntry *entry; // A reference to the entry bound to this box, but this won't add reference count
    GtkListItem *item;
} VocagtkEntryBox;

G_END_DECLS
#endif

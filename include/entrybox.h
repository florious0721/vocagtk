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
    GtkBox parent_type;

    GtkImage  *image;
    GtkLabel  *type_label;
    GtkLabel  *main_info;
    GtkLabel  *sub_info;
    GtkButton *add_or_remove; // Add or subscribe
    GtkButton *view_src;

    EntryListCtx *list; // managed by entry list, doesn't take reference count
    VocagtkEntry *entry; // A reference to the entry bound to this box, but doesn't take reference count
    GtkListItem *item; // doesn't take reference count
} VocagtkEntryBox;

G_END_DECLS
#endif

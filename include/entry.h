#ifndef _VOCAGTK_ENTRY_H
#define _VOCAGTK_ENTRY_H

#include <glib-object.h>
#include "album.h"
#include "artist.h"
#include "song.h"

G_BEGIN_DECLS

#define VOCAGTK_TYPE_ENTRY vocagtk_entry_get_type()
G_DECLARE_FINAL_TYPE(VocagtkEntry, vocagtk_entry, VOCAGTK, ENTRY, GObject)

/*!
 * @brief The union tag for VocagtkEntry.
 */
typedef enum {
    VOCAGTK_ENTRY_TYPE_LABEL_ALBUM,
    VOCAGTK_ENTRY_TYPE_LABEL_ARTIST,
    VOCAGTK_ENTRY_TYPE_LABEL_SONG
} VocagtkEntryTypeLabel;

/*!
 * @brief
 *  A union used to present a search entry and
 *  can hold either an album, artist, or song.
 */
typedef struct _VocagtkEntry {
    GObject parent_instance;
    VocagtkEntryTypeLabel type_label;
    union {
        VocagtkAlbum *album;
        VocagtkArtist *artist;
        VocagtkSong *song;
    } entry;
} VocagtkEntry;

/*!
 * @brief Creates a new VocagtkEntry with an album.
 *
 * @param album
 *   The VocagtkAlbum instance to wrap.
 *   The data is owned by the created instance.
 * @returns A new VocagtkEntry instance.
 */
VocagtkEntry *vocagtk_entry_new_album(VocagtkAlbum *album);

/*!
 * @brief Creates a new VocagtkEntry with an artist.
 *
 * @param artist
 *   The VocagtkArtist instance to wrap.
 *   The data is owned by the created instance.
 * @returns A new VocagtkEntry instance.
 */
VocagtkEntry *vocagtk_entry_new_artist(VocagtkArtist *artist);

/*!
 * @brief Creates a new VocagtkEntry with a song.
 *
 * @param song
 *   The VocagtkSong instance to wrap.
 *   The data is owned by the created instance.
 * @returns A new VocagtkEntry instance.
 */
VocagtkEntry *vocagtk_entry_new_song(VocagtkSong *song);

/*!
 * @brief Gets the type label of the entry.
 *
 * @returns The type label of the entry.
 */
VocagtkEntryTypeLabel vocagtk_entry_get_type_label(VocagtkEntry *self);

/*!
 * @brief Gets the image URL of the entry.
 *
 * @returns
 *   The image URL of the entry.
 *   The data is owned by the instance.
 */
const char *vocagtk_entry_get_image(VocagtkEntry *self);

/*!
 * @brief Gets the main information of the entry.
 *
 * @returns
 *   The main information of the entry.
 *   The data is owned by the instance.
 */
const char *vocagtk_entry_get_main_info(VocagtkEntry *self);

/*!
 * @brief Gets the sub information of the entry.
 *
 * @returns
 *   The sub information of the entry.
 *   The data is owned by the instance.
 */
const char *vocagtk_entry_get_sub_info(VocagtkEntry *self);

/*!
* @brief
*   Gets the id of the underlying album, artist or song.
*   You should use this with type label.
*
* @returns
*   The id of the underlying album, artist or song.
*/
gint vocagtk_entry_get_id(VocagtkEntry *self);

G_END_DECLS
#endif

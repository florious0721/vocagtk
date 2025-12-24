#ifndef _VOCAGTK_SONG_H
#define _VOCAGTK_SONG_H

#include <glib-object.h>

#include "helper.h"

// *INDENT-OFF*
G_BEGIN_DECLS
// *INDENT-ON*

#define VOCAGTK_TYPE_SONG vocagtk_song_get_type()
G_DECLARE_FINAL_TYPE(VocagtkSong, vocagtk_song, VOCAGTK, SONG, GObject)

typedef gint VocagtkSongId;

/*!
 * @brief
 *   Creates a new song object with its id in VocaDB.
 *
 *   The function will firstly attempt to look up its data in the local
 *   database. If the song is not in the local database, the
 *   function will fetch the song JSON from VocaDB using the downloader,
 *   parse it into a VocagtkSong, and then save it into the local database.
 *
 * @param id
 *   The id of song in VocaDB.
 * @param ctx
 *   Application state.
 *
 * @returns
 *   A new VocagtkSong instance.
 *   The returned object is owned by the caller of the function.
 */
VocagtkSong *vocagtk_song_new(
    VocagtkSongId id,
    AppState *ctx
);

/*!
 * @brief Gets the id of the song in VocaDB.
 *
 * @returns The id of the song.
 */
VocagtkSongId vocagtk_song_get_id(VocagtkSong *self);

/*!
 * @brief Gets the title of the song.
 *
 * @returns
 *   The title of the song.
 *   The data is owned by the instance.
 */
char const *vocagtk_song_get_title(VocagtkSong *self);

/*!
 * @brief Gets the artist string of the song.
 *
 * @returns
 *   The artist string of the song.
 *   The data is owned by the instance.
 */
char const *vocagtk_song_get_artist(VocagtkSong *self);
typedef struct _VocagtkSong {
    GObject parent_instance;
    VocagtkSongId id;
    GString *title;
    GString *artist;
    GString *image_url;
    time_t publish_date;
} VocagtkSong;

G_END_DECLS

#endif

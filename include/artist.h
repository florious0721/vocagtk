#ifndef _VOCAGTK_ARTIST_H
#define _VOCAGTK_ARTIST_H

#include <glib-object.h>
#include <time.h>

#include "helper.h"

// *INDENT-OFF*
G_BEGIN_DECLS
// *INDENT-ON*

#define VOCAGTK_TYPE_ARTIST vocagtk_artist_get_type()
G_DECLARE_FINAL_TYPE(VocagtkArtist, vocagtk_artist, VOCAGTK, ARTIST, GObject)

typedef gint VocagtkArtistId;

/*!
 * @brief
 *   Creates a new artist object with its id in VocaDB.
 *   The function will first attempt to look up its data in the local database.
 *   If the artist is not found in the local database, the function will fetch
 *   the artist information from VocaDB and update the local database.
 *
 * @param id
 *   The id of the artist in VocaDB.
 * @param ctx
 *   Application state which provides the local database handle and downloader
 *   used for remote fetching.
 *
 * @returns
 *   A new VocagtkArtist instance.
 *   The returned object is owned by the caller.
 */
VocagtkArtist *vocagtk_artist_new(
    VocagtkArtistId id,
    AppState *ctx
);

/*!
 * @brief Gets the id of the artist in VocaDB.
 *
 * @returns The id of the artist.
 */
gint vocagtk_artist_get_id(VocagtkArtist *self);

/*!
 * @brief Gets the name of the artist.
 *
 * @returns
 *   The name of the artist.
 *   The data is owned by the instance.
 */
char const *vocagtk_artist_get_name(VocagtkArtist *self);

/*!
 * @brief Gets the url of avatar image of the artist.
 *
 * @returns
 *   The url of avatar image of the artist.
 *   The data is owned by the instance.
 */
char const *vocagtk_artist_get_avatar_url(VocagtkArtist *self);

/*!
 * @brief Gets the update timestamp of the artist.
 *
 * @returns
 *   The update timestamp (Unix time) of the artist.
 */
time_t vocagtk_artist_get_update_at(VocagtkArtist *self);

typedef struct _VocagtkArtist {
    GObject parent_instance;
    VocagtkArtistId id;
    GString *name;
    GString *avatar_url;
    time_t update_at;
} VocagtkArtist;

G_END_DECLS
#endif

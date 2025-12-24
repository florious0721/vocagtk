#ifndef _VOCAGTK_PARSE_H
#define _VOCAGTK_PARSE_H

#include <yyjson.h>

#include "entry.h"

VocagtkAlbum *json_to_album(yyjson_val *json);
VocagtkArtist *json_to_artist(yyjson_val *json);
VocagtkSong *json_to_song(yyjson_val *json);

VocagtkEntry *json_to_entry(yyjson_val *json);

#endif

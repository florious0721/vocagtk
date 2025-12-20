#include <curl/curl.h>
#include <glib-unix.h>
#include <gdk/gdk.h>
#include <sqlite3.h>

#include <string.h>
#include <stdio.h>

#include "album.h"
#include "artist.h"
#include "song.h"

#include "data.h"
#include "entry.h"
#include "exterr.h"

GdkTexture *vocagtk_downloader_download_image(
    VocagtkDownloader *dl,
    char const *url
) {
    return gdk_texture_new_from_filename("example/unknown.png", NULL);

    // TODO: 图片下载与缓存
    char const *filename = url + strlen(url);
    while (*filename != '/' && filename != url) filename--;
    if (filename == url) {
        vocagtk_warn_def("Failed to get the filename of %s, use unknown", url);
    }

    GString *path = g_string_new(dl->cache_path);
    g_string_append(path, filename);
    FILE *f = fopen(path->str, "wb");

    curl_easy_reset(dl->handle);
    curl_easy_setopt(dl->handle, CURLOPT_URL, url);
    curl_easy_setopt(dl->handle, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(dl->handle, CURLOPT_WRITEDATA, f);
    CURLcode rc = curl_easy_perform(dl->handle);
    fclose(f);

    if (rc != CURLE_OK) {}

    GError *err = NULL;
    GdkTexture *texture = gdk_texture_new_from_filename(path->str, &err);
    if (err) {
        g_error_free(err);
    }

}

void vocagtk_downloader_search(
    VocagtkDownloader *dl,
    VocagtkSearchQuery *query,
    VocagtkSearchResultIterator *iter // out
) {
    GByteArray *buf = g_byte_array_new();

    // TODO: 查询 url 拼接

    //curl_easy_reset(dl->handle);
    curl_easy_setopt(
        dl->handle, CURLOPT_URL,
        "https://vocadb.net/api/entries?"
        "query=hello&childTags=false&start=0&maxResults=10&getTotalCount=false"
    );
    curl_easy_setopt(dl->handle, CURLOPT_WRITEFUNCTION,
        curl_g_byte_array_writer);
    curl_easy_setopt(dl->handle, CURLOPT_WRITEDATA, buf);
    CURLcode rc = curl_easy_perform(dl->handle);

    if (rc != CURLE_OK) {
        memset(iter, 0, sizeof(*iter));
        return;
    }

    iter->doc = yyjson_read(
        (char const *) buf->data, buf->len,
        YYJSON_READ_NOFLAG
    );
    yyjson_val *item = yyjson_doc_get_root(iter->doc);
    item = yyjson_obj_get(item, "items");
    yyjson_arr_iter_init(item, &iter->iter);
}

void vocagtk_get_search_entries(
    VocagtkSearchResultIterator *iter,
    GListStore *list, sqlite3 *db, int *sql_err
) {
    yyjson_val *item;
    while ((item = yyjson_arr_iter_next(&iter->iter))) {
        const char *entry_type = yyjson_get_str(
            yyjson_obj_get(item, "entryType")
        );

        if (g_strcmp0(entry_type, "Song") == 0) {
            VocagtkSong *song = vocagtk_song_new_from_json_with_db(item, db, sql_err);
            VocagtkEntry *entry = vocagtk_entry_new_song(song);
            g_list_store_append(list, entry);
            g_object_unref(entry);
        } else if (g_strcmp0(entry_type, "Album") == 0) {
            VocagtkAlbum *album = vocagtk_album_new_from_json_with_db(item, db, sql_err);
            VocagtkEntry *entry = vocagtk_entry_new_album(album);
            g_list_store_append(list, entry);
            g_object_unref(entry);
        } else if (g_strcmp0(entry_type, "Artist") == 0) {
            VocagtkArtist *artist = vocagtk_artist_new_from_json_with_db(item, db, sql_err);
            VocagtkEntry *entry = vocagtk_entry_new_artist(artist);
            g_list_store_append(list, entry);
            g_object_unref(entry);
        }
    }

}

#include <curl/curl.h>
#include <gdk/gdk.h>
#include <yyjson.h>
#include <time.h>

#include "dl.h"
#include "entry.h"
#include "exterr.h"
#include "helper.h"

static inline GByteArray *download(VocagtkDownloader *dl, char const *url,
    CURLcode *err) {
    GByteArray *arr = g_byte_array_new();
    curl_easy_setopt(dl->handle, CURLOPT_URL, url);
    curl_easy_setopt(dl->handle, CURLOPT_WRITEFUNCTION,
        curl_g_byte_array_writer);
    curl_easy_setopt(dl->handle, CURLOPT_WRITEDATA, arr);

    CURLcode rcode = curl_easy_perform(dl->handle);
    if (err) *err = rcode;

    return arr;
}

// *INDENT-OFF*
static inline yyjson_doc *vocagtk_downloader_json(
    VocagtkDownloader *dl,
    char const *url
) {
    yyjson_doc *r = NULL;
    CURLcode err = CURLE_OK;
    GByteArray *content = download(dl, url, &err);
    if (err != CURLE_OK) {
        vocagtk_err_curl_rcode(err);
    } else {
        r = yyjson_read(
            (char *) content->data, content->len,
            YYJSON_READ_NOFLAG
        );
    }
    g_byte_array_free(content, true);
    return r;
}
// *INDENT-ON*

yyjson_doc *vocagtk_downloader_album(VocagtkDownloader *dl, int id) {
    DEBUG("Try to fetch album %d.", id);
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/albums/%d"
        "?fields=Artists,MainPicture,Tracks&lang=Default", id
    );
    return vocagtk_downloader_json(dl, urlbuf);
}
yyjson_doc *vocagtk_downloader_artist(VocagtkDownloader *dl, int id) {
    DEBUG("Try to fetch artist %d.", id);
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/artists/%d"
        "?fields=MainPicture&lang=Default", id
    );
    return vocagtk_downloader_json(dl, urlbuf);
}
yyjson_doc *vocagtk_downloader_song(VocagtkDownloader *dl, int id) {
    DEBUG("Try to fetch song %d.", id);
    char urlbuf[0x80] = {0};
    g_snprintf(
        urlbuf, 0x80,
        "https://vocadb.net/api/songs/%d"
        "?fields=Albums,Artists,MainPicture&lang=Default", id
    );
    return vocagtk_downloader_json(dl, urlbuf);
}
void vocagtk_downloader_search(
    VocagtkSearchQuery const *query, // in
    VocagtkResultIterator *iter // out
) {
    memset(iter, 0, sizeof(*iter));
    iter->start = 0;
    iter->page_size = VOCAGTK_DOWNLOADER_PAGE_SIZE;
    iter->last_update_at = -1;

    iter->url = g_string_new(NULL);
    g_string_append_printf(
        iter->url,
        "https://vocadb.net/api/entries"
        "?fields=MainPicture&query=%s&maxResults=%lu"
        "&entryTypes=%s&start=0",
        query->query, iter->page_size, query->entry_type
    );
    iter->start_offset_in_url = iter->url->len - 1;
}
void vocagtk_downloader_update(
    int artist_id, time_t last_update_at,
    VocagtkResultIterator *iter // out
) {
    memset(iter, 0, sizeof(*iter));
    iter->start = 0;
    iter->page_size = VOCAGTK_DOWNLOADER_PAGE_SIZE;
    iter->last_update_at = last_update_at;

    iter->url = g_string_new(NULL);
    g_string_append_printf(
        iter->url,
        "https://vocadb.net/api/songs"
        "?fields=Albums,Artists,MainPicture&artistId[]=%d"
        "&maxResults=%lu&sort=PublishDate&start=0",
        artist_id, iter->page_size
    );
    iter->start_offset_in_url = iter->url->len - 1;
}
yyjson_val *vocagtk_result_iterator_next(
    VocagtkResultIterator *iter,
    VocagtkDownloader *dl, CURLcode *err
) {
    if (iter->start % iter->page_size == 0) {
        if (iter->start != 0 && iter->last_update_at == -1) goto clean;
        iter->url->len = iter->start_offset_in_url;
        g_string_append_printf(iter->url, "%lu", iter->start);

        yyjson_doc_free(iter->doc);
        iter->doc = vocagtk_downloader_json(dl, iter->url->str);

        yyjson_val *root = yyjson_doc_get_root(iter->doc);
        yyjson_val *arr = yyjson_obj_get(root, "items");
        yyjson_arr_iter_init(arr, &iter->iter);
    }
    yyjson_val *next = yyjson_arr_iter_next(&iter->iter);
    if (!next) goto clean;

    // Check publish date if last_update_at is set
    if (iter->last_update_at >= 0) {
        yyjson_val *publish_date_val = yyjson_obj_get(next, "publishDate");
        if (yyjson_is_str(publish_date_val)) {
            char const *publish_date_str = yyjson_get_str(publish_date_val);
            time_t publish_date = parse_iso8601_datetime(publish_date_str);

            if (publish_date >= 0 && publish_date <= iter->last_update_at) {
                DEBUG(
                    "Song publish date %s (%ld) is before or equal to last update %ld, stopping iteration",
                      publish_date_str, publish_date, iter->last_update_at);
                goto clean;
            }
        }
    }

    iter->start++;
    return next;

clean:
    yyjson_doc_free(iter->doc);
    g_string_free(iter->url, true);
    memset(iter, 0, sizeof(*iter));
    return NULL;
}

GdkTexture *vocagtk_downloader_image(
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

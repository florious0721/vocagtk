#include <time.h>

#include "db.h"
#include "exterr.h"
#include "helper.h"

// album helpers
VocagtkAlbum *db_album_from_row(sqlite3_stmt *stmt, int *sql_err) {
    int id = sqlite3_column_int(stmt, 0);
    char const *title = sqlite3_column_str(stmt, 1);
    char const *artist = sqlite3_column_str(stmt, 2);
    char const *cover_url = sqlite3_column_str(stmt, 3);
    time_t publish_date = (time_t) sqlite3_column_int64(stmt, 4);
    VocagtkAlbum *album = g_object_new(
        VOCAGTK_TYPE_ALBUM,
        "id", id,
        "title", title,
        "artist", artist,
        "cover-url", cover_url,
        "publish-date", publish_date,
        NULL
    );
    if (sql_err) *sql_err = SQLITE_OK;
    return album;
}

int db_album_add(sqlite3 *db, VocagtkAlbum const *album) {
    char const *sql =
        "INSERT INTO album(id, title, artist, cover_url, publish_date) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "artist = excluded.artist, "
        "cover_url = excluded.cover_url, "
        "publish_date = excluded.publish_date;";
    DEBUG("Save album %d to db.", album->id);

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, album->id);
    sqlite3_bind_text(stmt, 2, album->title->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, album->artist->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, album->cover_url->str, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, album->publish_date);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

int db_artist_add_from_json(sqlite3 *db, yyjson_val *artist_json) {
    if (!yyjson_is_obj(artist_json)) return SQLITE_ERROR;

    yyjson_val *id_val = yyjson_obj_get(artist_json, "id");
    yyjson_val *name_val = yyjson_obj_get(artist_json, "name");
    yyjson_val *picture_val = yyjson_obj_get(artist_json, "mainPicture");

    if (!id_val) return SQLITE_ERROR;

    int id = yyjson_get_int(id_val);
    char const *name = name_val ? yyjson_get_str(name_val) : "";

    char const *avatar_url = "https://vocadb.net/Content/unknown.png";
    if (picture_val) {
        yyjson_val *thumb = yyjson_obj_get(picture_val, "urlSmallThumb");
        if (!thumb) thumb = yyjson_obj_get(picture_val, "urlThumb");
        if (yyjson_is_str(thumb)) avatar_url = yyjson_get_str(thumb);
    }

    DEBUG("Save artist %d to db from JSON.", id);
    char const *sql =
        "INSERT INTO artist(id, name, avatar_url, update_at) "
        "VALUES(?, ?, ?, 0) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, "
        "avatar_url = excluded.avatar_url;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, avatar_url, -1, SQLITE_STATIC);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    rcode = sqlite3_finalize(stmt);
    return rcode;
}

int db_album_add_from_json(sqlite3 *db, yyjson_val *album_json) {
    if (!yyjson_is_obj(album_json)) return SQLITE_ERROR;

    yyjson_val *id_val = yyjson_obj_get(album_json, "id");
    yyjson_val *title_val = yyjson_obj_get(album_json, "name");
    yyjson_val *artist_val = yyjson_obj_get(album_json, "artistString");
    yyjson_val *picture_val = yyjson_obj_get(album_json, "mainPicture");

    if (!id_val) return SQLITE_ERROR;

    int id = (int) yyjson_get_int(id_val);
    char const *title = title_val ? yyjson_get_str(title_val) : "";
    char const *artist = artist_val ? yyjson_get_str(artist_val) : "";

    char const *cover_url = "https://vocadb.net/Content/unknown.png";
    if (picture_val) {
        yyjson_val *thumb = yyjson_obj_get(picture_val, "urlSmallThumb");
        if (!thumb) thumb = yyjson_obj_get(picture_val, "urlThumb");
        if (yyjson_is_str(thumb)) cover_url = yyjson_get_str(thumb);
    }

    DEBUG("Save album %d to db from JSON.", id);
    char const *sql =
        "INSERT INTO album(id, title, artist, cover_url) "
        "VALUES(?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "artist = excluded.artist, "
        "cover_url = excluded.cover_url;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, artist, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, cover_url, -1, SQLITE_STATIC);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    rcode = sqlite3_finalize(stmt);
    return rcode;
}

int db_artist_update_time(sqlite3 *db, int artist_id, time_t update_at) {
    DEBUG("Update artist %d update_at to %ld", artist_id, update_at);
    char const *sql = "UPDATE artist SET update_at = ? WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_int64(stmt, 1, update_at);
    sqlite3_bind_int(stmt, 2, artist_id);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    rcode = sqlite3_finalize(stmt);
    return rcode;
}

int db_song_add_from_json(sqlite3 *db, yyjson_val *song_json) {
    if (!yyjson_is_obj(song_json)) return SQLITE_ERROR;

    yyjson_val *id_val = yyjson_obj_get(song_json, "id");
    yyjson_val *title_val = yyjson_obj_get(song_json, "name");
    yyjson_val *artist_val = yyjson_obj_get(song_json, "artistString");
    yyjson_val *picture_val = yyjson_obj_get(song_json, "mainPicture");
    yyjson_val *publish_date_val = yyjson_obj_get(song_json, "publishDate");

    if (!id_val) return SQLITE_ERROR;

    int id = (int) yyjson_get_int(id_val);
    char const *title = title_val ? yyjson_get_str(title_val) : "";
    char const *artist = artist_val ? yyjson_get_str(artist_val) : "";

    char const *image_url = "https://vocadb.net/Content/unknown.png";
    if (picture_val) {
        yyjson_val *thumb = yyjson_obj_get(picture_val, "urlSmallThumb");
        if (!thumb) thumb = yyjson_obj_get(picture_val, "urlThumb");
        if (yyjson_is_str(thumb)) image_url = yyjson_get_str(thumb);
    }

    // Parse publishDate string to unix timestamp using helper
    time_t publish_date = 0;
    if (yyjson_is_str(publish_date_val)) {
        const char *publish_date_str = yyjson_get_str(publish_date_val);
        publish_date = parse_iso8601_datetime(publish_date_str);
        if (publish_date == (time_t) -1) publish_date = 0;
    }

    DEBUG("Save song %d to db from JSON.", id);
    char const *sql =
        "INSERT INTO song(id, title, artist, image_url, publish_date) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "artist = excluded.artist, "
        "image_url = excluded.image_url, "
        "publish_date = excluded.publish_date;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, artist, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, image_url, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, publish_date);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkAlbum *db_album_get_by_id(sqlite3 *db, int id, int *err) {
    DEBUG("Try to read album %d from db.", id);

    char const *sql =
        "SELECT id, title, artist, cover_url, publish_date FROM album WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;

    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        return NULL;
    }

    rcode = sqlite3_bind_int(stmt, 1, id);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        sqlite3_finalize(stmt);
        return NULL;
    }

    rcode = sqlite3_step(stmt);
    VocagtkAlbum *album = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No album found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return album;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        album = db_album_from_row(stmt, NULL);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return album;
    default:
        vocagtk_warn_sql_db(db);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return album;
    }
}

// artist helpers
VocagtkArtist *db_artist_from_row(sqlite3_stmt *stmt, int *sql_err) {
    int id = sqlite3_column_int(stmt, 0);
    char const *name = sqlite3_column_str(stmt, 1);
    char const *avatar_url = sqlite3_column_str(stmt, 2);
    time_t update_at = (time_t) sqlite3_column_int64(stmt, 3);
    VocagtkArtist *artist = g_object_new(
        VOCAGTK_TYPE_ARTIST,
        "id", id,
        "name", name,
        "avatar-url", avatar_url,
        "update-at", update_at,
        NULL
    );
    if (sql_err) *sql_err = SQLITE_OK;
    return artist;
}

int db_artist_add(sqlite3 *db, VocagtkArtist const *artist) {
    DEBUG("Save artist %d to db.", artist->id);
    char const *sql =
        "INSERT INTO artist(id, name, avatar_url, update_at) "
        "VALUES(?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "name = excluded.name, "
        "avatar_url = excluded.avatar_url, "
        "update_at = excluded.update_at;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, artist->id);
    sqlite3_bind_text(stmt, 2, artist->name->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, artist->avatar_url->str, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, artist->update_at);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkArtist *db_artist_get_by_id(sqlite3 *db, int id, int *err) {
    DEBUG("Try to read artist %d from db.", id);

    char const *sql =
        "SELECT id, name, avatar_url, update_at FROM artist WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;

    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        return NULL;
    }

    rcode = sqlite3_bind_int(stmt, 1, id);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        sqlite3_finalize(stmt);
        return NULL;
    }

    rcode = sqlite3_step(stmt);
    VocagtkArtist *artist = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No artist found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return artist;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        artist = db_artist_from_row(stmt, NULL);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return artist;
    default:
        vocagtk_warn_sql_db(db);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return artist;
    }
}

// song helpers
VocagtkSong *db_song_from_row(sqlite3_stmt *stmt, int *sql_err) {
    int id = sqlite3_column_int(stmt, 0);
    char const *title = sqlite3_column_str(stmt, 1);
    char const *artist = sqlite3_column_str(stmt, 2);
    char const *image_url = sqlite3_column_str(stmt, 3);
    time_t publish_date = (time_t) sqlite3_column_int64(stmt, 4);
    VocagtkSong *song = g_object_new(
        VOCAGTK_TYPE_SONG,
        "id", id,
        "title", title,
        "artist", artist,
        "image-url", image_url,
        "publish-date", publish_date,
        NULL
    );
    if (sql_err) *sql_err = SQLITE_OK;
    return song;
}

int db_song_add(sqlite3 *db, VocagtkSong const *song) {
    DEBUG("Save song %d to db.", song->id);
    char const *sql =
        "INSERT INTO song(id, title, artist, image_url, publish_date) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET "
        "title = excluded.title, "
        "artist = excluded.artist, "
        "image_url = excluded.image_url, "
        "publish_date = excluded.publish_date;";

    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) return rcode;

    sqlite3_bind_int(stmt, 1, song->id);
    sqlite3_bind_text(stmt, 2, song->title->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, song->artist->str, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, song->image_url->str, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, song->publish_date);

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) goto clean;

clean:
    rcode = sqlite3_finalize(stmt);
    return rcode;
}

VocagtkSong *db_song_get_by_id(sqlite3 *db, int id, int *err) {
    DEBUG("Try to read song %d from db.", id);

    char const *sql =
        "SELECT id, title, artist, image_url, publish_date FROM song WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;

    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        return NULL;
    }

    rcode = sqlite3_bind_int(stmt, 1, id);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (err) *err = rcode;
        return NULL;
    }

    rcode = sqlite3_step(stmt);
    VocagtkSong *song = NULL;
    switch (rcode) {
    case SQLITE_DONE:
        vocagtk_log_sql(G_LOG_LEVEL_INFO, "No song found in database");
        if (err) *err = sqlite3_finalize(stmt);
        return song;
    case SQLITE_ROW:
        DEBUG("Found in database.");
        song = db_song_from_row(stmt, NULL);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return song;
    default:
        vocagtk_warn_sql_db(db);
        rcode = sqlite3_finalize(stmt);
        if (err) *err = rcode;
        return song;
    }
}

// @returns the number of relations inserted into the table
int db_insert_song_albums(sqlite3 *db, yyjson_val *song_json, int *sql_err) {
    yyjson_val *id_val = yyjson_obj_get(song_json, "id");
    yyjson_val *albums = yyjson_obj_get(song_json, "albums");
    if (!id_val || !albums || !yyjson_is_arr(albums)) return 0;
    int song_id = (int) yyjson_get_int(id_val);
    size_t idx, max;
    yyjson_val *album;
    int total = 0;
    char const *sql =
        "INSERT OR IGNORE INTO song_in_album(song_id, album_id) VALUES(?, ?);";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return 0;
    }
    yyjson_arr_foreach(albums, idx, max, album) {
        yyjson_val *album_id_val = yyjson_obj_get(album, "id");
        if (!album_id_val) continue;
        int album_id = (int) yyjson_get_int(album_id_val);

        // Check if album exists in album table
        char const *check_sql = "SELECT 1 FROM album WHERE id = ?;";
        sqlite3_stmt *check_stmt;
        int check_rcode = sqlite3_prepare_v2(
            db, check_sql, -1, &check_stmt, NULL
        );
        int exists = 0;
        if (check_rcode == SQLITE_OK) {
            sqlite3_bind_int(check_stmt, 1, album_id);
            if (sqlite3_step(check_stmt) == SQLITE_ROW) exists = 1;
            sqlite3_finalize(check_stmt);
        }
        if (!exists) {
            // Insert album if not exists
            db_album_add_from_json(db, album);
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, song_id);
        sqlite3_bind_int(stmt, 2, album_id);
        rcode = sqlite3_step(stmt);
        if (rcode == SQLITE_DONE) ++total;
        else vocagtk_warn_sql_db(db);
    }
    sqlite3_finalize(stmt);
    return total;
}

// @returns the number of relations inserted into the table
int db_insert_song_artists(sqlite3 *db, yyjson_val *song_json, int *sql_err) {
    yyjson_val *id_val = yyjson_obj_get(song_json, "id");
    yyjson_val *artists = yyjson_obj_get(song_json, "artists");
    if (!id_val || !yyjson_is_arr(artists)) return 0;
    int song_id = (int) yyjson_get_int(id_val);
    size_t idx, max;
    yyjson_val *artist;
    int total = 0;
    char const *sql =
        "INSERT OR IGNORE INTO artist_for_song(song_id, artist_id) VALUES(?, ?);";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return 0;
    }
    yyjson_arr_foreach(artists, idx, max, artist) {
        yyjson_val *artist_val = yyjson_obj_get(artist, "artist");
        yyjson_val *artist_id_val = yyjson_obj_get(artist_val, "id");
        if (!artist_id_val) continue;
        int artist_id = (int) yyjson_get_int(artist_id_val);

        // Check if artist exists in artist table
        char const *check_sql = "SELECT 1 FROM artist WHERE id = ?;";
        sqlite3_stmt *check_stmt;
        int check_rcode = sqlite3_prepare_v2(db, check_sql, -1, &check_stmt,
            NULL);
        bool exists = false;
        if (check_rcode == SQLITE_OK) {
            sqlite3_bind_int(check_stmt, 1, artist_id);
            if (sqlite3_step(check_stmt) == SQLITE_ROW) exists = true;
            sqlite3_finalize(check_stmt);
        }
        if (!exists) {
            // Insert artist if not exists
            db_artist_add_from_json(db, artist_val);
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_int(stmt, 1, song_id);
        sqlite3_bind_int(stmt, 2, artist_id);
        rcode = sqlite3_step(stmt);
        if (rcode == SQLITE_DONE) ++total;
        else vocagtk_warn_sql_db(db);
    }
    sqlite3_finalize(stmt);
    return total;
}

// 向 rss 订阅添加 artist_id
// 返回值：成功插入的行数（1=新插入，0=已存在）
// sql_err: 如果提供，接收 SQLite 错误码
int db_rss_add_artist(sqlite3 *db, int artist_id, int *sql_err) {
    char const *sql = "INSERT OR IGNORE INTO rss(artist_id) VALUES(?);";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return 0;
    }
    sqlite3_bind_int(stmt, 1, artist_id);
    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        if (sql_err) *sql_err = rcode;
        return 0;
    }

    // 获取受影响的行数：1 表示新插入，0 表示已存在
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (sql_err) *sql_err = SQLITE_OK;

    // 返回受影响的行数
    return changes;
}
int db_rss_remove_artist(sqlite3 *db, int artist_id) {
    char const *sql = "DELETE FROM rss WHERE artist_id = ?;";
    sqlite3_stmt *stmt = NULL;

    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    rcode = sqlite3_bind_int(stmt, 1, artist_id);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    rcode = sqlite3_step(stmt);
    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

// 查询 rss 表 artist_id 的最近更新时间
time_t db_rss_get_update_time(sqlite3 *db, int artist_id, int *sql_err) {
    char const *sql = "SELECT update_at FROM rss WHERE artist_id = ?;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return (time_t) -1;
    }
    sqlite3_bind_int(stmt, 1, artist_id);
    rcode = sqlite3_step(stmt);
    time_t result = -1;
    if (rcode == SQLITE_ROW) {
        result = (time_t) sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    if (rcode != SQLITE_ROW && rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
    }
    return result;
}

int db_entry_add(sqlite3 *db, VocagtkEntry const *entry) {
    switch (entry->type_label) {
    case VOCAGTK_ENTRY_TYPE_LABEL_ALBUM:
        return db_album_add(db, entry->entry.album);
    case VOCAGTK_ENTRY_TYPE_LABEL_ARTIST:
        return db_artist_add(db, entry->entry.artist);
    case VOCAGTK_ENTRY_TYPE_LABEL_SONG:
        return db_song_add(db, entry->entry.song);
    default:
        vocagtk_warn_def("Unknown type %d", entry->type_label);
        return SQLITE_ERROR;
    }
}

// Playlist operations

/**
 * Create a new playlist with the given name
 * @param db Database connection
 * @param name Playlist name (must be unique)
 * @return SQLITE_OK on success, error code on failure
 */
int db_playlist_create(sqlite3 *db, char const *name) {
    char const *sql = "INSERT INTO playlist(name) VALUES(?);";
    sqlite3_stmt *stmt = NULL;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    rcode = sqlite3_step(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    DEBUG("Created playlist '%s'", name);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

/**
 * Delete a playlist and all its song associations
 * @param db Database connection
 * @param name Playlist name to delete
 * @return SQLite error code
 */
int db_playlist_delete(sqlite3 *db, char const *name) {
    char const *sql = "DELETE FROM playlist WHERE name = ?;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    rcode = sqlite3_step(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    DEBUG("Deleted playlist '%s'", name);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

/**
 * Rename a playlist
 * @param db Database connection
 * @param old_name Current playlist name
 * @param new_name New name for the playlist
 * @return SQLite error code
 */
int db_playlist_rename(sqlite3 *db, char const *old_name, char const *new_name) {
    char const *sql = "UPDATE playlist SET name = ? WHERE name = ?;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, old_name, -1, SQLITE_STATIC);
    rcode = sqlite3_step(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    DEBUG("Renamed playlist '%s' to '%s'", old_name, new_name);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

/**
 * Get all playlists
 * @param db Database connection
 * @param stmt Output parameter for prepared statement (caller must finalize)
 * @return SQLite error code
 *
 * Usage:
 *   sqlite3_stmt *stmt;
 *   if (db_playlist_get_all(db, &stmt) == SQLITE_OK) {
 *       while (sqlite3_step(stmt) == SQLITE_ROW) {
 *           char const *name = sqlite3_column_str(stmt, 0);
 *       }
 *       sqlite3_finalize(stmt);
 *   }
 */
int db_playlist_get_all(sqlite3 *db, sqlite3_stmt **stmt) {
    char const *sql = "SELECT name FROM playlist ORDER BY name ASC;";
    int rcode = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }
    return SQLITE_OK;
}

/**
 * Check if a playlist exists
 * @param db Database connection
 * @param name Playlist name to check
 * @param sql_err Output parameter for SQLite error code (can be NULL)
 * @return 1 if exists, 0 if not found
 *
 * Usage:
 *   int sql_err;
 *   int exists = db_playlist_exists(db, "Default", &sql_err);
 *   if (sql_err == SQLITE_OK) {
 *       if (exists) {
 *           // Playlist exists
 *       } else {
 *           // Playlist not found
 *       }
 *   } else {
 *       // Error occurred
 *   }
 */
int db_playlist_exists(sqlite3 *db, char const *name, int *sql_err) {
    char const *sql = "SELECT 1 FROM playlist WHERE name = ? LIMIT 1;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    rcode = sqlite3_step(stmt);

    if (rcode == SQLITE_ROW) {
        // Found the playlist
        DEBUG("Playlist '%s' exists", name);
        sqlite3_finalize(stmt);
        if (sql_err) *sql_err = SQLITE_OK;
        return 1;
    } else if (rcode == SQLITE_DONE) {
        // Playlist not found
        DEBUG("Playlist '%s' not found", name);
        sqlite3_finalize(stmt);
        if (sql_err) *sql_err = SQLITE_OK;
        return 0;
    } else {
        // Error occurred
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        if (sql_err) *sql_err = rcode;
        return 0;
    }
}

// Song in playlist operations

/**
 * Add a song to a playlist
 * @param db Database connection
 * @param playlist_name Playlist name
 * @param song_id Song ID to add
 * @param sql_err Output parameter for error code (can be NULL)
 * @return Number of rows inserted (1 if newly added, 0 if already exists)
 */
int db_playlist_add_song(
    sqlite3 *db, char const *playlist_name,
    int song_id, int *sql_err
) {
    char const *sql =
        "INSERT OR IGNORE INTO song_in_playlist(song_id, playlist_name) VALUES(?, ?);";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        if (sql_err) *sql_err = rcode;
        return 0;
    }

    sqlite3_bind_int(stmt, 1, song_id);
    sqlite3_bind_text(stmt, 2, playlist_name, -1, SQLITE_STATIC);
    rcode = sqlite3_step(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        if (sql_err) *sql_err = rcode;
        return 0;
    }

    // Get number of rows affected: 1 = newly inserted, 0 = already exists
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (sql_err) *sql_err = SQLITE_OK;
    DEBUG(
        "Added song %d to playlist '%s' (changes: %d)",
        song_id, playlist_name, changes
    );
    return changes;
}

/**
 * Remove a song from a playlist
 * @param db Database connection
 * @param playlist_name Playlist name
 * @param song_id Song ID to remove
 * @return SQLite error code
 */
int db_playlist_remove_song(sqlite3 *db, char const *playlist_name, int song_id) {
    char const *sql =
        "DELETE FROM song_in_playlist WHERE playlist_name = ? AND song_id = ?;";
    sqlite3_stmt *stmt;
    int rcode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_text(stmt, 1, playlist_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, song_id);
    rcode = sqlite3_step(stmt);

    if (rcode != SQLITE_DONE) {
        vocagtk_warn_sql_db(db);
        sqlite3_finalize(stmt);
        return rcode;
    }

    DEBUG("Removed song %d from playlist '%s'", song_id, playlist_name);
    sqlite3_finalize(stmt);
    return SQLITE_OK;
}

/**
 * Get all songs in a playlist
 * @param db Database connection
 * @param playlist_name Playlist name
 * @param stmt Output parameter for prepared statement (caller must finalize)
 * @return SQLite error code
 *
 * Usage:
 *   sqlite3_stmt *stmt;
 *   if (db_playlist_get_songs(db, "Default", &stmt) == SQLITE_OK) {
 *       while (sqlite3_step(stmt) == SQLITE_ROW) {
 *           VocagtkSong *song = db_song_from_row(stmt, NULL);
 *           // use song...
 *           g_object_unref(song);
 *       }
 *       sqlite3_finalize(stmt);
 *   }
 */
int db_playlist_get_songs(sqlite3 *db, char const *playlist_name, sqlite3_stmt **stmt) {
    char const *sql =
        "SELECT s.id, s.title, s.artist, s.image_url, s.publish_date "
        "FROM song s "
        "JOIN song_in_playlist sip ON s.id = sip.song_id "
        "WHERE sip.playlist_name = ? "
        "ORDER BY s.id ASC;";

    int rcode = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
    if (rcode != SQLITE_OK) {
        vocagtk_warn_sql_db(db);
        return rcode;
    }

    sqlite3_bind_text(*stmt, 1, playlist_name, -1, SQLITE_STATIC);
    return SQLITE_OK;
}

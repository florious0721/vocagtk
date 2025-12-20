#include <curl/curl.h>
#include <glib.h>
#include <sqlite3.h>

#define CURL_LOG_DOMAIN "CURL"
#define SQL_LOG_DOMAIN "SQLITE3"

#define vocagtk_log(domain, level, format, ...) \
    (g_log((domain), (level), "[%s:%d]:" format, __FILE_NAME__, __LINE__ __VA_OPT__(,) __VA_ARGS__))

#define vocagtk_warn_def(format, ...) \
    vocagtk_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, format, __VA_ARGS__)

#define vocagtk_log_sql(level, format, ...) \
    vocagtk_log(SQL_LOG_DOMAIN, level, format, __VA_ARGS__)

#define vocagtk_warn_sql(format, ...) \
    vocagtk_log(SQL_LOG_DOMAIN, G_LOG_LEVEL_WARNING, format, __VA_ARGS__)

#define vocagtk_log_sql_rcode(level, rcode) \
    vocagtk_log_sql(level, "%s", sqlite3_errstr(rcode))

#define vocagtk_warn_sql_rcode(rcode) \
    vocagtk_log_curl_rcode(G_LOG_LEVEL_WARNING, rcode)

#define vocagtk_log_curl(level, format, ...) \
    vocagtk_log(CURL_LOG_DOMAIN, level, format, __VA_ARGS__)

#define vocagtk_warn_curl(format, ...) \
    vocagtk_log(CURL_LOG_DOMAIN, G_LOG_LEVEL_WARNING, format, __VA_ARGS__)

#define vocagtk_log_curl_rcode(level, rcode) \
    vocagtk_log_curl(level, "%s", curl_easy_strerror(rcode))

#define vocagtk_warn_curl_rcode(rcode) \
    vocagtk_log_curl_rcode(G_LOG_LEVEL_WARNING, rcode)

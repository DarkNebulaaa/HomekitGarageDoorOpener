#pragma once
#ifndef USER_WEB_H
#define USER_WEB_H
#include <esp_http_server.h>

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
httpd_handle_t start_webserver(void);
#endif

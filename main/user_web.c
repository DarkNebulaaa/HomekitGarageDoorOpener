#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_tls_crypto.h"
#include "esp_http_server.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "dns_server.h"
#include "cJSON.h"

#include "user_web.h"
#include "user_wifi_ap.h"
#include "user_nvs.h"

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}
static esp_err_t scan_get_handler(httpd_req_t *req){
    wifi_ap_record_t aps_ret[20];
    uint16_t number_ret = 20;
    uint8_t found = wifi_scan_aps(&aps_ret[0], number_ret);

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < found; i++) {
        if (aps_ret[i].ssid[0] == 0) continue; // skip hidden/empty

        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "ssid", (const char *)aps_ret[i].ssid);
        cJSON_AddNumberToObject(o, "rssi", aps_ret[i].rssi);
        cJSON_AddBoolToObject(o, "sec", aps_ret[i].authmode != WIFI_AUTH_OPEN);
        cJSON_AddItemToArray(arr, o);
    }

    char *json = cJSON_PrintUnformatted(arr);
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    // 4) cleanup
    cJSON_Delete(arr);
    free(json);

    return ESP_OK;
};
httpd_uri_t scan_uri = {
        .uri      = "/scan",
        .method   = HTTP_GET,
        .handler  = scan_get_handler,
        .user_ctx = NULL
    };

static esp_err_t read_req_body(httpd_req_t *req, char **out_buf, int *out_len)
{
    int total = req->content_len;
    if (total <= 0 || total > 4096) { // 你可以調大，但別無限大
        return ESP_FAIL;
    }

    char *buf = (char *)malloc(total + 1);
    if (!buf) return ESP_ERR_NO_MEM;

    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) continue; // 等一下再收
        if (r < 0) { free(buf); return ESP_FAIL; }
        received += r;
    }
    buf[received] = '\0';

    *out_buf = buf;
    *out_len = received;
    return ESP_OK;
}
static esp_err_t save_post_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    // 1) 讀 body
    char *body = NULL;
    int body_len = 0;
    if (read_req_body(req, &body, &body_len) != ESP_OK) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"bad_body\"}");
        return ESP_OK;
    }

    // 2) 解析 JSON
    cJSON *root = cJSON_Parse(body);
    free(body);

    if (!root) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"invalid_json\"}");
        return ESP_OK;
    }

    // 3) 取欄位
    const cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *j_pwd  = cJSON_GetObjectItemCaseSensitive(root, "password");

    if (!cJSON_IsString(j_ssid) || (j_ssid->valuestring == NULL)) {
        cJSON_Delete(root);
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"missing_ssid\"}");
        return ESP_OK;
    }

    const char *ssid = j_ssid->valuestring;
    const char *pwd  = (cJSON_IsString(j_pwd) && j_pwd->valuestring) ? j_pwd->valuestring : "";

    ESP_LOGI(TAG, "Got ssid='%s', pwd=%s", ssid, pwd);

    // 4) TODO：存 NVS / 嘗試連線（你接這裡）
    // nvs_set_str(..., "ssid", ssid); nvs_set_str(..., "pwd", pwd); nvs_commit(...)
    // esp_wifi_set_config(...); esp_wifi_connect();


    // 5) 回應 JSON
    httpd_resp_sendstr(req, "{\"ok\":true}");
    write_wifi_data(ssid, pwd);
    cJSON_Delete(root);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 等 NVS 寫入完成
    esp_restart();
    return ESP_OK;
};
httpd_uri_t save_uri = {
        .uri      = "/save",
        .method   = HTTP_POST,
        .handler  = save_post_handler,
        .user_ctx = NULL
    };
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
        httpd_register_uri_handler(server, &scan_uri);
        httpd_register_uri_handler(server, &save_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
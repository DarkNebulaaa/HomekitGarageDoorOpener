#include "user_wifi_ap.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "user_web.h"
#include "dns_server.h"
#include "user_hap_serv.h"

#define MAX_APS 20
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

static bool ssid_exists(const wifi_ap_record_t *list, int n, const uint8_t *ssid)
{
    for (int i = 0; i < n; i++) {
        if (strcmp((const char*)list[i].ssid, (const char*)ssid) == 0) return true;
    }
    return false;
}
u_int8_t wifi_scan_aps(wifi_ap_record_t *aps_ret, uint16_t number_ret){

    uint16_t number = 0;
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 30,
                .max = 80,
            },
        },
        .home_chan_dwell_time = 30,
        .channel_bitmap = {
            .ghz_2_channels = 0,
            .ghz_5_channels = 0,
    }};
    esp_wifi_scan_start(&scan_config, true);
    esp_wifi_scan_get_ap_num(&number);
    if (number > MAX_APS) number = MAX_APS;

    wifi_ap_record_t founds_aps[number];
    esp_wifi_scan_get_ap_records(&number, founds_aps);
    int n = 0;
    for (int i = 0; i < number && n < number_ret; i++) {
        if (founds_aps[i].ssid[0] == 0) continue; // skip hidden/empty

        if (!ssid_exists(aps_ret, n, founds_aps[i].ssid)) {// 去重：同 SSID 只收一次
            aps_ret[n++] = founds_aps[i];
        } else continue;
    }
    return n;
}
static void wifi_init_sta(char ssid[32], char password[64])
{
    app_wifi_init();
    app_wifi_start_custom_wifi(portMAX_DELAY, ssid, password);
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}
static void wifi_init_softap(void)
{   
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifiAP_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifiAP_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiAP_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             WIFI_SSID, WIFI_PASS, 1);


}

WifiMode_t wifi_serv_init(void){
    nvs_handle_t wifi_nvs_handle;
    char NVS_WIFI_SSID[32] = {0};
    char NVS_WIFI_PASS[64] = {0};
    size_t len_ssid = sizeof(NVS_WIFI_SSID);
    size_t len_pass = sizeof(NVS_WIFI_PASS);
    nvs_open("wifi", NVS_READONLY, &wifi_nvs_handle);
    esp_err_t err = nvs_get_str(wifi_nvs_handle, "NVS_WIFI_SSID", NVS_WIFI_SSID, &len_ssid);
                    nvs_get_str(wifi_nvs_handle, "NVS_WIFI_PASS", NVS_WIFI_PASS, &len_pass);
    if(err == ESP_OK) {
        ESP_LOGI(TAG, "Starting STA with default SSID: %s", NVS_WIFI_SSID);
        wifi_init_sta(NVS_WIFI_SSID, NVS_WIFI_PASS);
        return RF_MODE_STA;
    } else{
        ESP_LOGI(TAG, "Starting AP with default SSID: %s", WIFI_SSID);
        wifi_init_softap();
        start_webserver();
        /*啟動DNS Server，將所有客戶端的連線自動導向WiFi設定頁面*/
        dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
        start_dns_server(&config);
        return RF_MODE_AP;
    }
    nvs_close(wifi_nvs_handle);
}
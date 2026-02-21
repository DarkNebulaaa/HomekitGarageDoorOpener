#pragma once
#ifndef USER_WIFI_AP_H
#define USER_WIFI_AP_H
#include "config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define WIFI_SSID "HOMEKIT_ACCESSORY_CONTROLLER"
#define WIFI_PASS "hap_esp32"

typedef enum WifiMode {
    RF_MODE_STA = 0,
    RF_MODE_AP = 1,
}WifiMode_t;
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
/**
 * @brief call this API 可以獲取掃描到附近熱點的資料 return值是實際掃描到的熱點數量 
 * @param aps_ret 熱點資料結構體指針
 * @param number_ret 指定回傳熱點數量上限
 * @return return值是實際掃描到的熱點數量 */                           
u_int8_t wifi_scan_aps(wifi_ap_record_t *aps_ret, uint16_t number_ret);
/*初始化STA模模式*/
static void wifi_init_sta(char ssid[32], char password[64]);
/*初始化AP模式*/
static void wifi_init_softap(void);
/*初始化網路服務，如果NVS有已存在的WiFi資料，會啟動STA，反之啟動AP*/  
WifiMode_t wifi_serv_init(void);
/*釋放AP模式資源，切換到STA模式前呼叫，目前未使用*/
void wifi_deinit_softap(void);                         
#endif /* USER_WIFI_AP_H */
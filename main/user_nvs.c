#include "user_nvs.h"
#include "nvs_flash.h"
esp_err_t nvs_init(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    return ret;
}
esp_err_t write_wifi_data(const char *ssid, const char *password){
    nvs_handle_t write_nvs_handle;
    nvs_open("wifi", NVS_READWRITE, &write_nvs_handle);
    nvs_set_str(write_nvs_handle, "NVS_WIFI_SSID", ssid);
    nvs_set_str(write_nvs_handle, "NVS_WIFI_PASS", password);
    nvs_commit(write_nvs_handle);
    nvs_close(write_nvs_handle);
    return ESP_OK;
}
esp_err_t factory_reset_nvs(void){
    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}
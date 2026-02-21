#pragma once
#ifndef CONFIG_H
#define CONFIG_H

extern const char *TAG;

#define IO_STATE_ZERO GPIO_NUM_0
#define IO_STATE_ONE  GPIO_NUM_1
#define IO_CTRL_UP    GPIO_NUM_3
#define IO_CTRL_STOP  GPIO_NUM_4
#define IO_CTRL_DOWN  GPIO_NUM_5
#define IO_INDECATOR  GPIO_NUM_7
#define RESET_GPIO    GPIO_NUM_9
#define PLUGIN_GPIO   GPIO_NUM_10

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        5

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     5

#define WIFI_SSID "Homekit_Accessory_Controller"
#define WIFI_PASS "hap_esp32"

#define FW_VERSION "1.0.4"

#endif /* CONFIG_H */
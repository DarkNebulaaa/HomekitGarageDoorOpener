#pragma once
#ifndef USER_HAP_SERV_H
#define USER_HAP_SERV_H
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_event.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <hap_fw_upgrade.h>
#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

//led_strip
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "user_wifi_ap.h"

#define CONFIG_HARDCODED_SETUP_CODE
#define GARAGE_DOOR_TASK_PRIORITY  1
#define GARAGE_DOOR_TASK_STACKSIZE 4 * 1024
#define GARAGE_DOOR_TASK_NAME      "hap_garage_door"

//enum
enum door_cmd {
    Idle = 0,
    Open,
    Close,
    Stop
};

enum door_state {
    Opened = 0,
    Closed,
    Moving,
    Stopped
};


typedef enum door_cmd DoorCmd;
typedef enum door_state DoorState;
extern led_strip_handle_t led_strip;
static hap_char_t *current_door_state_char;
static hap_char_t *target_door_state_char;


static DoorCmd DoorOperation;
extern DoorState State, Priv_State;
static int state_Changed = 1;

static char server_cert[] = {};
static hap_char_t *current_state, *target_state;


#define CONFIG_USE_HARDCODED_SETUP_CODE
#define SETUP_CODE "457-86-921"
#define SETUP_ID "DO01"

#define IO_STATE_ZERO GPIO_NUM_0
#define IO_STATE_ONE  GPIO_NUM_1
#define IO_CTRL_UP    GPIO_NUM_3
#define IO_CTRL_STOP  GPIO_NUM_4
#define IO_CTRL_DOWN  GPIO_NUM_5
#define IO_INDECATOR  GPIO_NUM_7
#define RESET_GPIO    GPIO_NUM_9

/* Reset network credentials if button is pressed for more than 3 seconds and then released */
#define RESET_NETWORK_BUTTON_TIMEOUT        3

/* Reset to factory if button is pressed and held for more than 10 seconds */
#define RESET_TO_FACTORY_BUTTON_TIMEOUT     10

void configure_led(void);
/**
 * 指示燈控制任務，AP模式下呼吸燈，STA模式下根據狀態變化顏色
 *@param mode 指示燈模式，AP或STA會有不同的顯示
 */
void led_pwm(void* mode);
/*初始化GPIO*/
static void configure_gpio(void);
/*獲取車庫門狀態*/
DoorState get_door_state(void);
/*控制車庫門開關*/
static void ctrl_door(DoorCmd cmd);
static void reset_network_handler(void* arg);
static void reset_to_factory_handler(void* arg);
/*初始化重設出廠設置按鈕*/
void reset_key_init(uint32_t key_gpio_pin);
static int garage_door_identify(hap_acc_t *ha);
static void garage_door_hap_event_handler(void* arg, esp_event_base_t event_base, int32_t event, void *data);
static int garage_door_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv);
static int garage_door_write(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv);
static void garage_door_thread_entry(void *p);
void garage_door_serv();

#endif
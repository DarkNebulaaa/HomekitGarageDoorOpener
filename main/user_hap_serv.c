#include "user_hap_serv.h"
#include "user_nvs.h"
#include "esp_system.h"

led_strip_handle_t led_strip;
DoorState State, Priv_State;
//指示燈初始化

static void garage_door_led(){
    static int direction = 0;
    static int brightness = 0;
    while(1){
        if (State == Moving){
            if (!direction){
                brightness += 2;
                if (brightness >=250){
                    direction = 1;
                }
            }else if (direction){
                brightness -= 2;
                if (brightness <= 0){
                    direction = 0;
                }
        }
            led_strip_set_pixel(led_strip, 0, 0, brightness, brightness);
            led_strip_refresh(led_strip);
            vTaskDelay(10 / portTICK_PERIOD_MS);

       }else if (State == Closed){
        led_strip_set_pixel(led_strip, 0, 255, 0, 0);
        led_strip_refresh(led_strip);

       }else if (State == Opened){
        led_strip_set_pixel(led_strip, 0, 0, 255, 0);
        led_strip_refresh(led_strip);

       }else{
        led_strip_clear(led_strip);
       }
    }
}

static void led_blink(uint8_t R, uint8_t G, uint8_t B){
    for (int i = 0; i <= 10; i++) {
    led_strip_set_pixel(led_strip, 0, R, G, B);
    led_strip_refresh(led_strip);   // 亮
    vTaskDelay(pdMS_TO_TICKS(100));

    led_strip_clear(led_strip);     // 滅
    vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void configure_gpio(){
    ESP_LOGI(TAG, "configured GPIO DONE");
    // 車庫門腳位初始化
    gpio_reset_pin(IO_STATE_ZERO);
    gpio_reset_pin(IO_STATE_ONE);
    gpio_reset_pin(IO_CTRL_UP);
    gpio_reset_pin(IO_CTRL_STOP);
    gpio_reset_pin(IO_CTRL_DOWN);

    gpio_set_direction(IO_STATE_ZERO, GPIO_MODE_INPUT);
    gpio_set_direction(IO_STATE_ONE, GPIO_MODE_INPUT);
    gpio_set_direction(IO_CTRL_UP, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_CTRL_STOP, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_CTRL_DOWN, GPIO_MODE_OUTPUT);

    gpio_set_level(IO_CTRL_UP,   1);
    gpio_set_level(IO_CTRL_STOP, 1);
    gpio_set_level(IO_CTRL_DOWN, 1);

    // 澆水系統腳位初始化
    gpio_reset_pin(PLUGIN_GPIO);
    gpio_set_direction(PLUGIN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(PLUGIN_GPIO, 0);

}

DoorState get_door_state(){
    int s0 = gpio_get_level(IO_STATE_ZERO);
    int s1 = gpio_get_level(IO_STATE_ONE);
    vTaskDelay(500/portTICK_PERIOD_MS);
    s0 = gpio_get_level(IO_STATE_ZERO);
    s1 = gpio_get_level(IO_STATE_ONE);
        if (s0 == 1 && s1 == 0)
        {
            return Opened;
        }
        if (s0 == 1 && s1 == 1)
        {
            return Closed;
        }
        if (s0 == 0 && s1 == 0)
        {
            return Moving;
        }
        else
        {
            return Stopped;
        }
        
    }
    
static void ctrl_door(DoorCmd cmd){
    switch (cmd)
    {
    case 0:
        /* code */
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        cmd = Idle;
        break;

    case 1:
        gpio_set_level(IO_CTRL_UP,   0);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        cmd = Idle;
        break;

    case 2:
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 0);
        gpio_set_level(IO_CTRL_DOWN, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        cmd = Idle;
        break;

    case 3:
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 0);
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        cmd = Idle;
        break;

    default:
        gpio_set_level(IO_CTRL_UP,   1);
        gpio_set_level(IO_CTRL_STOP, 1);
        gpio_set_level(IO_CTRL_DOWN, 1);
        cmd = Idle;
        break;
    }
}


static void reset_network_handler(void* arg)
{
    hap_reset_network();
}
/**
 * @brief The factory reset button callback handler.
 */
static void reset_to_factory_handler(void* arg)
{
    hap_reset_homekit_data();
    hap_reset_to_factory();
    factory_reset_nvs();
    esp_restart();
}

/**
 * The Reset button  GPIO initialisation function.
 * Same button will be used for resetting Wi-Fi network as well as for reset to factory based on
 * the time for which the button is pressed.
 */
void reset_key_init(uint32_t key_gpio_pin)
{
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(handle, RESET_NETWORK_BUTTON_TIMEOUT, reset_network_handler, NULL);
    iot_button_add_on_press_cb(handle, RESET_TO_FACTORY_BUTTON_TIMEOUT, reset_to_factory_handler, NULL);
}

/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */

static int garage_door_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

static int watering_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

/*
 * An optional HomeKit Event handler which can be used to track HomeKit
 * specific events.
 */
static void hap_event_handler(void* arg, esp_event_base_t event_base, int32_t event, void *data)
{
    switch(event) {
        case HAP_EVENT_PAIRING_STARTED :
            ESP_LOGI(TAG, "Pairing Started");
            break;
        case HAP_EVENT_PAIRING_ABORTED :
            ESP_LOGI(TAG, "Pairing Aborted");
            break;
        case HAP_EVENT_CTRL_PAIRED :
            ESP_LOGI(TAG, "Controller %s Paired. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_UNPAIRED :
            ESP_LOGI(TAG, "Controller %s Removed. Controller count: %d",
                        (char *)data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_CONNECTED :
            ESP_LOGI(TAG, "Controller %s Connected", (char *)data);
            break;
        case HAP_EVENT_CTRL_DISCONNECTED :
            ESP_LOGI(TAG, "Controller %s Disconnected", (char *)data);
            break;
        case HAP_EVENT_ACC_REBOOTING : {
            char *reason = (char *)data;
            ESP_LOGI(TAG, "Accessory Rebooting (Reason: %s)",  reason ? reason : "null");
            break;
        case HAP_EVENT_PAIRING_MODE_TIMED_OUT :
            ESP_LOGI(TAG, "Pairing Mode timed out. Please reboot the device.");
        }
        default:
            /* Silently ignore unknown events */
            break;
    }
}

/* A dummy callback for handling a read on the "Direction" characteristic of Fan.
 * In an actual accessory, this should read from hardware.
 * Read routines are generally not required as the value is available with th HAP core
 * when it is updated from write routines. For external triggers (like fan switched on/off
 * using physical button), accessories should explicitly call hap_char_update_val()
 * instead of waiting for a read request.
 */

 static int garage_door_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv){
    if (hap_req_get_ctrl_id(read_priv)) {
        ESP_LOGI(TAG, "Read data !! Received read from %s", hap_req_get_ctrl_id(read_priv));
    }
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_DOOR_STATE)) {
        const hap_val_t *cur_val = hap_char_get_val(hc);
        hap_val_t new_val;
        new_val.u = get_door_state();
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }
    return HAP_SUCCESS;
 }

 static int watering_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv){
    if (hap_req_get_ctrl_id(read_priv)) {
        ESP_LOGI(TAG, "Read data !! Received read from %s", hap_req_get_ctrl_id(read_priv));
    }
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_OUTLET_IN_USE)) {
        hap_val_t new_val;
        new_val.b = gpio_get_level(PLUGIN_GPIO);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_ON)){
        hap_val_t new_val;
        new_val.b = gpio_get_level(PLUGIN_GPIO);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_IN_USE)){
        hap_val_t new_val;
        new_val.b = gpio_get_level(PLUGIN_GPIO);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }
    if (!strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_ACTIVE)){
        hap_val_t new_val;
        new_val.b = gpio_get_level(PLUGIN_GPIO);
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }
    return HAP_SUCCESS;
 }

/* A dummy callback for handling a write on the Fan service
 * In an actual accessory, this should also control the hardware.
 */
static int garage_door_write(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv){
    static DoorCmd DoorOperation;
    if (hap_req_get_ctrl_id(write_priv)) {
        ESP_LOGI(TAG, "Write data !!Received write from %s", hap_req_get_ctrl_id(write_priv));
    }
    ESP_LOGI(TAG, "GarageDoor Write called with %d chars", count);
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_TARGET_DOOR_STATE)) {
            ESP_LOGI(TAG, "Received Write. GarageDoor state %s",write->val.u ? "Close" : "Open");
            if (write->val.u){   
                DoorOperation = Close;
            } else{
                DoorOperation = Open;
            }
            ctrl_door(DoorOperation);
            hap_char_update_val(write->hc, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
    }
    return ret;
}

static int watering_write(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv){
    if (hap_req_get_ctrl_id(write_priv)) {
        ESP_LOGI(TAG, "Write data !!Received write from %s", hap_req_get_ctrl_id(write_priv));
    }
    ESP_LOGI(TAG, "Watering Write called with %d chars", count);
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
            ESP_LOGI(TAG, "Received Write. Watering state %s",write->val.b ? "On" : "Off");
            gpio_set_level(PLUGIN_GPIO, write->val.b);
            hap_char_update_val(write->hc, &(write->val));
            hap_char_update_val(current_watering_state_char, &(write->val));
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_OUTLET_IN_USE)){
            hap_val_t new_val;
            new_val.b = gpio_get_level(PLUGIN_GPIO);
            hap_char_update_val(write->hc, &new_val);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_IN_USE)){
            hap_val_t new_val;
            new_val.b = gpio_get_level(PLUGIN_GPIO);
            hap_char_update_val(write->hc, &new_val);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ACTIVE)){
            hap_val_t new_val;
            new_val.b = gpio_get_level(PLUGIN_GPIO);
            hap_char_update_val(write->hc, &new_val);
            *(write->status) = HAP_STATUS_SUCCESS;
        }
    }
    return ret;
}

hap_serv_t *hap_serv_custom_valve_create()
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_VALVE);
    if (!hs) {
        return NULL;
    }
    if (hap_serv_add_char(hs, hap_char_valve_type_create(1)) != HAP_SUCCESS) {
        goto err;
    }
    if( hap_serv_add_char(hs, hap_char_in_use_create(0)) != HAP_SUCCESS) {
        goto err;
    }
    if( hap_serv_add_char(hs, hap_char_active_create(0)) != HAP_SUCCESS) {
        goto err;
    }
    return hs;
err:
    hap_serv_delete(hs);
    return NULL;
} 
/*The main thread for handling the Fan Accessory */
static void garage_door_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Configure HomeKit core to make the Accessory name (and thus the WAC SSID) unique,
     * instead of the default configuration wherein only the WAC SSID is made unique.
     */
    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_SSID;
    hap_set_config(&hap_cfg);

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);
    led_blink(0, 255, 255);
    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
        .name = "GarageDoorOpener",
        .manufacturer = "Mason",
        .model = "Garage Door_V1",
        .serial_num = "1120260812",
        .fw_rev = "1.0.2",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = garage_door_identify,
        .cid = HAP_CID_GARAGE_DOOR_OPENER,
    };
    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Add Wi-Fi Transport service required for HAP Spec R16 */
    hap_acc_add_wifi_transport_service(accessory, 0);

    /* Create the Fan Service. Include the "name" since this is a user visible service  */
    service = hap_serv_garage_door_opener_create(1, 1, 0);
    hap_serv_add_char(service, hap_char_name_create("Garage Door"));
    //hap_serv_add_char(service, hap_char_current_door_state_create(0));

    /* Set the write callback for the service */
    hap_serv_set_write_cb(service, garage_door_write);

    /* Set the read callback for the service (optional) */
    hap_serv_set_read_cb(service, garage_door_read);


    /* Set the current state for the service */
    current_door_state_char = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_CURRENT_DOOR_STATE);
    target_door_state_char = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_TARGET_DOOR_STATE);
    /* Add the Fan Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Create the Firmware Upgrade HomeKit Custom Service.
     * Please refer the FW Upgrade documentation under components/homekit/extras/include/hap_fw_upgrade.h
     * and the top level README for more information.
     */
    hap_fw_upgrade_config_t ota_config = {
        .server_cert_pem = server_cert,
    };
    service = hap_serv_fw_upgrade_create(&ota_config);
    /* Add the service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* Register a common button for reset Wi-Fi network and reset to factory.
     */
    //reset_key_init(RESET_GPIO);

    /* Query the controller count (just for information) */
    ESP_LOGI(TAG, "Accessory is paired with %d controllers",
                hap_get_paired_controller_count());

    /* TODO: Do the actual hardware initialization here */

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(SETUP_ID_GarageDoor);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(SETUP_CODE, SETUP_ID_GarageDoor, true, cfg.cid);
#else
    app_hap_setup_payload(SETUP_CODE, SETUP_ID_GarageDoor, false, cfg.cid);
#endif
#endif
    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    //app_wifi_init();

    /* Register an event handler for HomeKit specific events.
     * All event handlers should be registered only after app_wifi_init()
     */
    esp_event_handler_register(HAP_EVENT, ESP_EVENT_ANY_ID, &hap_event_handler, NULL);

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    //app_wifi_start(portMAX_DELAY);
    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelay(pdMS_TO_TICKS(1500));
    xTaskCreate(garage_door_led, "garage_door_led", 2048, NULL, 1, NULL);
    vTaskDelete(NULL);
}

static void watering_thread_entry(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Configure HomeKit core to make the Accessory name (and thus the WAC SSID) unique,
     * instead of the default configuration wherein only the WAC SSID is made unique.
     */
    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_SSID;
    hap_set_config(&hap_cfg);
    hap_init(HAP_TRANSPORT_WIFI);
    led_blink(255, 40, 0);


    hap_acc_cfg_t cfg = {
        .name = "WateringSystem",
        .manufacturer = "Mason",
        .model = "Watering System_V2",
        .serial_num = "114021159",
        .fw_rev = "1.0.3",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = watering_identify,
        .cid = HAP_CID_SPRINKLER,
    };

    /* Create accessory object */
    accessory = hap_acc_create(&cfg);

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Add Wi-Fi Transport service required for HAP Spec R16 */
    hap_acc_add_wifi_transport_service(accessory, 0);

    /* Create the Fan Service. Include the "name" since this is a user visible service  */

    service = hap_serv_custom_valve_create();
    current_watering_state_char = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_ACTIVE);
    hap_acc_add_serv(accessory, service);
    service = hap_serv_outlet_create(0,1);
    hap_serv_add_char(service, hap_char_name_create("Watering System"));
    hap_serv_set_write_cb(service, watering_write);
    hap_serv_set_read_cb(service, watering_read);
    outlet_state_char = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_ON);
     /* Add the service to the Accessory Object */
    hap_acc_add_serv(accessory, service);
    
    hap_fw_upgrade_config_t ota_config = {
        .server_cert_pem = server_cert,
    };
    service = hap_serv_fw_upgrade_create(&ota_config);
    /* Add the service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    ESP_LOGI(TAG, "Accessory is paired with %d controllers",
                hap_get_paired_controller_count());


    hap_set_setup_code(SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(SETUP_ID_Watering);
    app_hap_setup_payload(SETUP_CODE, SETUP_ID_Watering, false, cfg.cid);

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    /* Initialize Wi-Fi */
    //app_wifi_init();

    /* Register an event handler for HomeKit specific events.
     * All event handlers should be registered only after app_wifi_init()
     */
    esp_event_handler_register(HAP_EVENT, ESP_EVENT_ANY_ID, &hap_event_handler, NULL);

    /* After all the initializations are done, start the HAP core */
    hap_start();
    /* Start Wi-Fi */
    //app_wifi_start(portMAX_DELAY);
    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);
}

void garage_door_serv()
{
    configure_gpio();
    /* Refresh the strip to send data */
    State = get_door_state();
    Priv_State = Moving;
    state_Changed = 1;
    xTaskCreate(garage_door_thread_entry, GARAGE_DOOR_TASK_NAME, GARAGE_DOOR_TASK_STACKSIZE, NULL, GARAGE_DOOR_TASK_PRIORITY, NULL);
    while (1){  
        //vTaskDelay(3000/portTICK_PERIOD_MS);
        hap_val_t data, t_data;
        State = get_door_state();
        if (State != Priv_State){
            state_Changed = 1;
        }
        if(state_Changed){
            ESP_LOGI(TAG, "state is update");
            if (Priv_State == Closed && State == Moving){ //開啟中
                data.u = 2;
                t_data.u = 0;
                
                hap_char_update_val(target_door_state_char, &t_data);
                vTaskDelay(800/portTICK_PERIOD_MS);
                hap_char_update_val(current_door_state_char, &data);

            }else if (Priv_State == Opened && State == Moving){//關閉中
                data.u = 3;
                t_data.u = 1;

                hap_char_update_val(target_door_state_char, &t_data);
                vTaskDelay(800/portTICK_PERIOD_MS);
                hap_char_update_val(current_door_state_char, &data);

            }else if (Priv_State == Moving && State == Opened){//已開啟
                data.u = 0;
                t_data.u = 0;

                hap_char_update_val(target_door_state_char, &t_data);
                vTaskDelay(500/portTICK_PERIOD_MS);
                hap_char_update_val(current_door_state_char, &data);

            }else if (Priv_State == Moving && State == Closed){//已關閉

                data.u = 1;
                t_data.u = 1;

                hap_char_update_val(target_door_state_char, &t_data);
                vTaskDelay(500/portTICK_PERIOD_MS);
                hap_char_update_val(current_door_state_char, &data);
            }else{
                data.u = data.u;
                t_data.u = t_data.u;
                ESP_LOGI(TAG, "IN ELSE STATE");
                hap_char_update_val(target_door_state_char, &t_data);
                vTaskDelay(500/portTICK_PERIOD_MS);
                hap_char_update_val(current_door_state_char, &data);

            }
            Priv_State = State;
            state_Changed = 0;
        } 
    }

}

void watering_serv()
{
    configure_gpio();
    xTaskCreate(watering_thread_entry, WATERING_TASK_NAME, WATERING_TASK_STACKSIZE, NULL, WATERING_TASK_PRIORITY, NULL);
    vTaskDelete(NULL);
}
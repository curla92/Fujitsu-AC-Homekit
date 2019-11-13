#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <esplibs/libmain.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "../common/custom_characteristics.h"
#include <wifi_config.h>

#include <led_codes.h>
#include <adv_button.h>
#include <dht/dht.h>

#include <sysparam.h>
#include <rboot-api.h>

#include <irremote/irremote.h>
#include <Fujitsu_Commands.h>

const int led_gpio = 2; // BUILT-IN LED on pin D4
const int TEMPERATURE_SENSOR_PIN = 4; // DHT sensor on pin D2
const int GPIO_IR_PIN = 14; // IR sensor on pin D5

#define SHOW_SETUP_SYSPARAM "A"
#define INIT_STATE_TH_SYSPARAM "B"
#define LAST_TARGET_STATE_TH_SYSPARAM "C"
#define TARGET_TEMPERATURE_SYSPARAM "D"
#define TEMP_OFFSET "E"
#define HUM_OFFSET "F"
#define FAN_SYSPARAM "G"
#define SWING_SYSPARAM "H"

#define ALLOWED_FACTORY_RESET_TIME 60000

uint8_t reset_toggle_counter = 0;
uint8_t init_state = 0;
uint8_t target_fan = 2;
bool target_swing = false;

volatile float old_humidity_value = 0.0, old_temperature_value = 0.0, switch_temp_update = 0;

ETSTimer device_restart_timer, change_settings_timer, save_states_timer, reset_toggle_timer, extra_func_timer;

volatile bool paired = false;
volatile bool Wifi_Connected = false;

homekit_value_t read_ip_addr();
homekit_value_t custom_fan_get();
homekit_value_t enable_swing_get();

void custom_fan_set(homekit_value_t value);
void enable_swing_set(homekit_value_t value);

void update_state();
void update_temp();

void show_setup_callback();
void save_states_callback();
void save_settings();
void change_settings_callback();
void reboot_callback();
void ota_firmware_callback();

void on_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    update_state();
}
void on_temp_update(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    update_temp();
}

// GENERAL AND CUSTOM
homekit_characteristic_t show_setup = HOMEKIT_CHARACTERISTIC_(CUSTOM_SHOW_SETUP, true, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(show_setup_callback));
homekit_characteristic_t device_type_name = HOMEKIT_CHARACTERISTIC_(CUSTOM_DEVICE_TYPE_NAME, "Fujitsu AC", .id=107);
homekit_characteristic_t custom_init_state_th = HOMEKIT_CHARACTERISTIC_(CUSTOM_INIT_STATE_TH, 0, .id=108, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t temp_offset = HOMEKIT_CHARACTERISTIC_(CUSTOM_TEMPERATURE_OFFSET, 0.0, .id = 109, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t hum_offset = HOMEKIT_CHARACTERISTIC_(CUSTOM_HUMIDITY_OFFSET, 0, .id = 110, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t ip_addr = HOMEKIT_CHARACTERISTIC_(CUSTOM_IP_ADDR, "", .id = 111, .getter = read_ip_addr);
homekit_characteristic_t wifi_reset = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RESET, false, .id=112, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t reboot_device = HOMEKIT_CHARACTERISTIC_(CUSTOM_REBOOT_DEVICE, false, .id = 113, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(reboot_callback));
homekit_characteristic_t ota_firmware = HOMEKIT_CHARACTERISTIC_(CUSTOM_OTA_UPDATE, false, .id = 114, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(ota_firmware_callback));
// TEMP AND HUM
homekit_characteristic_t current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t current_humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 0);

// AC PARAMETERS
homekit_characteristic_t target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 22, .min_value = (float[]) {18}, .max_value = (float[]) {30}, .min_step = (float[]) {1}, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_temp_update));
homekit_characteristic_t current_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 0);
homekit_characteristic_t target_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 0, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(on_update), .min_value = (float[]) {0}, .max_value = (float[]) {2}, .min_step = (float[]) {1});

homekit_characteristic_t custom_fan = HOMEKIT_CHARACTERISTIC_(CUSTOM_FAN, 2, .getter=custom_fan_get, .setter=custom_fan_set);
homekit_characteristic_t enable_swing = HOMEKIT_CHARACTERISTIC_(CUSTOM_SWING, false, .getter=enable_swing_get, .setter=enable_swing_set);


void update_state() {
    uint8_t state = target_heating_cooling_state.value.int_value;
    
    if (state == 1 && current_temperature.value.int_value >= target_temperature.value.int_value) {
        
        current_heating_cooling_state.value = HOMEKIT_UINT8(0);
        // printf("Target State1: %d\n",state );
    }else if (state == 2 && current_temperature.value.int_value <= target_temperature.value.int_value) {
        current_heating_cooling_state.value = HOMEKIT_UINT8(0);
        // printf("Target State2: %d\n",state );
    }else{
        current_heating_cooling_state.value = HOMEKIT_UINT8(state);
        // printf("Target State3: %d\n",state );
    }
    
    vTaskDelay(200 / portTICK_PERIOD_MS);
    homekit_characteristic_notify(&current_heating_cooling_state, current_heating_cooling_state.value);
    // printf("switch_temp_update: %f\n", switch_temp_update );
    // OFF CLIMA
    
    if (state == 0 && init_state != 0) {
        enable_swing.value.bool_value = false;
        target_swing = enable_swing.value.bool_value;
        init_state = 0;
        ac_button_off();
        led_code(led_gpio, FUNCTION_C);
        homekit_characteristic_notify(&enable_swing, enable_swing.value);
    }else if(switch_temp_update == 0) {
        update_temp();
    }
    
    switch_temp_update = 0;
    
    save_states_callback();
}

void update_temp() {
    
    // printf("Running update_temp() \n" );
    uint8_t target_state = target_heating_cooling_state.value.int_value;
    
    float target_temp = 0;
    if (target_state == 1) {
        // Read the Heat target
        init_state = 1;
        target_temp = target_temperature.value.float_value;
    }else if(target_state == 2) {
        // Else read the Cool target
        init_state = 1;
        target_temp = target_temperature.value.float_value;
    }
    // printf("Target State: %d\n",target_state );
    // printf("Target temp: %f\n",target_temp );
    
    target_temp = (int)target_temp;
    pass_temp_mode_values(target_temp, target_state, target_fan);
    
    ac_button_temp();

    if (target_state == 1 && target_temp == 30) {
    led_code(led_gpio, FUNCTION_B);
    }else if (target_state == 2 && target_temp == 18) {
    led_code(led_gpio, FUNCTION_B);
    }

    switch_temp_update = 1;
    update_state();

}

// FAN GET VALUE
homekit_value_t custom_fan_get() {
    return HOMEKIT_UINT8(target_fan);
}

// FAN SET VALUE
void custom_fan_set(homekit_value_t value) {
    if (value.format != homekit_format_uint8) {
        //printf("Invalid Fan-value format: %d\n", value.format);
        return;
    }
    target_fan = value.uint8_value;
    update_temp();
}

// SWING GET VALUE
homekit_value_t enable_swing_get() {
    return HOMEKIT_BOOL(target_swing);
}

// SWING SET VALUE
void enable_swing_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        //printf("Invalid Swing-value format: %d\n", value.format);
        return;
    }
    target_swing = value.bool_value;
    if (target_heating_cooling_state.value.int_value != 0) {
        ac_button_swing();
    }
}

// CHANGE SETTINGS
void change_settings_callback() {
    sdk_os_timer_arm(&change_settings_timer, 3500, 0);
}

// SAVE LAST STATE
void save_states_callback() {
    sdk_os_timer_arm(&save_states_timer, 5000, 0);
}

// SAVE STATES
void save_states() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    if (custom_init_state_th.value.int_value > 0) {
        status = sysparam_set_int8(LAST_TARGET_STATE_TH_SYSPARAM, target_heating_cooling_state.value.int_value);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_set_int32(TARGET_TEMPERATURE_SYSPARAM, target_temperature.value.float_value * 100);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    if (flash_error != SYSPARAM_OK) {
        printf("Saving last states error -> %i\n", flash_error);
    }
}

// SAVE SETTINGS
void save_settings() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    status = sysparam_set_bool(SHOW_SETUP_SYSPARAM, show_setup.value.bool_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(HUM_OFFSET, hum_offset.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int32(TEMP_OFFSET, temp_offset.value.float_value * 100);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(INIT_STATE_TH_SYSPARAM, custom_init_state_th.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    if (flash_error != SYSPARAM_OK) {
        printf("Saving settings error -> %i\n", flash_error);
    }
}

// SETTINGS INIT
void settings_init() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    bool bool_value;
    
    int8_t int8_value;
    int32_t int32_value;
    
    status = sysparam_get_bool(SHOW_SETUP_SYSPARAM, &bool_value);
    if (status == SYSPARAM_OK) {
        show_setup.value.bool_value = bool_value;
    } else {
        status = sysparam_set_bool(SHOW_SETUP_SYSPARAM, true);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(INIT_STATE_TH_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        custom_init_state_th.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(INIT_STATE_TH_SYSPARAM, 0);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(HUM_OFFSET, &int8_value);
    if (status == SYSPARAM_OK) {
        hum_offset.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(HUM_OFFSET, 0);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int32(TEMP_OFFSET, &int32_value);
    if (status == SYSPARAM_OK) {
        temp_offset.value.float_value = int32_value / 100.00f;
    } else {
        status = sysparam_set_int32(TEMP_OFFSET, 0);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    // LOAD SAVED STATES
    if (custom_init_state_th.value.int_value < 1) {
        target_heating_cooling_state.value.int_value = custom_init_state_th.value.int_value;
    } else {
        status = sysparam_get_int8(LAST_TARGET_STATE_TH_SYSPARAM, &int8_value);
        if (status == SYSPARAM_OK) {
            target_heating_cooling_state.value.int_value = int8_value;
        } else {
            status = sysparam_set_int8(LAST_TARGET_STATE_TH_SYSPARAM, 0);
            if (status != SYSPARAM_OK) {
                flash_error = status;
            }
        }
    }
    
    status = sysparam_get_int32(TARGET_TEMPERATURE_SYSPARAM, &int32_value);
    if (status == SYSPARAM_OK) {
        target_temperature.value.float_value = int32_value / 100.00f;
    } else {
        status = sysparam_set_int32(TARGET_TEMPERATURE_SYSPARAM, 22 * 100);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    if (flash_error == SYSPARAM_OK) {
    } else {
        printf("ERROR settings INIT -> %i\n", flash_error);
    }
}

// IP Address
homekit_value_t read_ip_addr() {
    struct ip_info info;
    
    if (sdk_wifi_get_ip_info(STATION_IF, &info)) {
        char *buffer = malloc(16);
        snprintf(buffer, 16, IPSTR, IP2STR(&info.ip));
        return HOMEKIT_STRING(buffer);
    }
    return HOMEKIT_STRING("");
}

// RESTART
void device_restart_task() {
    vTaskDelay(5500 / portTICK_PERIOD_MS);
    
    if (wifi_reset.value.bool_value) {
        wifi_config_reset();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
    if (ota_firmware.value.bool_value) {
        rboot_set_temp_rom(1);
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    
    sdk_system_restart();
    vTaskDelete(NULL);
}

void device_restart() {
    printf("Restarting device\n");
    led_code(led_gpio, RESTART_DEVICE);
    xTaskCreate(device_restart_task, "device_restart_task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
}

// RESET
void reset_configuration_task() {
    led_code(led_gpio, WIFI_CONFIG_RESET);
    printf("Resetting Wifi Config\n");
    
    wifi_config_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    printf("Resetting HomeKit Config\n");
    homekit_server_reset();
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Restarting\n");
    
    sdk_system_restart();
    vTaskDelete(NULL);
}

void reset_mode_call(const uint8_t gpio, void *args) {
    if (xTaskGetTickCountFromISR() < ALLOWED_FACTORY_RESET_TIME / portTICK_PERIOD_MS) {
        led_code(led_gpio, FUNCTION_D);
        xTaskCreate(reset_configuration_task, "Reset configuration", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    } else {
        printf("Factory reset not allowed after %ims since boot. Repower device and try again\n", ALLOWED_FACTORY_RESET_TIME);
    }
}

// RESET COUNT CALLBACK
void reset_toggle_upcount() {
    reset_toggle_counter++;
    sdk_os_timer_arm(&reset_toggle_timer, 3100, 0);
}

void reset_toggle() {
    if (reset_toggle_counter > 10) {
        reset_mode_call(0, NULL);
    }
    reset_toggle_counter = 0;
}

// REBOOT
void reboot_callback() {
    if (reboot_device.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
    }
}

// OTA
void ota_firmware_callback() {
    if (ota_firmware.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
    }
}

// SHOW SETUP CALLBACK
void show_setup_callback() {
    if (show_setup.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
        
    }
    save_settings();
    reset_toggle_upcount();
}

// LED IDENTIFY TASK
void identify_task(void *_args) {
    // Identify dispositive by Pulsing LED.
    led_code(led_gpio, IDENTIFY_ACCESSORY);
    vTaskDelete(NULL);
}

// LED IDENTIFY
void identify(homekit_value_t _value) {
    printf("Clima Identify\n");
    xTaskCreate(identify_task, "Clima identify", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}

// TEMP HUM FUNCTION
void temperature_sensor() {
    // Temperature measurement
    float humidity_value, temperature_value;
    bool get_temp = false;
    
    get_temp = dht_read_float_data(DHT_TYPE_DHT22, TEMPERATURE_SENSOR_PIN, &humidity_value, &temperature_value);
    
    if (get_temp) {
        temperature_value += temp_offset.value.float_value;
        humidity_value += hum_offset.value.float_value;
        // printf("Sensor: temperature %g, humidity %g\n", temperature_value, humidity_value);
        
        if (temperature_value != old_temperature_value) {
            
            old_temperature_value = temperature_value;
            current_temperature.value = HOMEKIT_FLOAT(temperature_value);
            homekit_characteristic_notify( &current_temperature, HOMEKIT_FLOAT(temperature_value));
        }
        if (humidity_value != old_humidity_value) {
            
            old_humidity_value = humidity_value;
            current_humidity.value = HOMEKIT_FLOAT(humidity_value);
            homekit_characteristic_notify( & current_humidity, current_humidity.value);
        }
    }else{
        // led_code(led_gpio, SENSOR_ERROR);
        printf("Couldnt read data from sensor\n");
    }
}

// HOMEKIT
void on_event(homekit_event_t event) {
    
    switch (event) {
        case HOMEKIT_EVENT_SERVER_INITIALIZED:
            printf("on_homekit_event: Server initialised\n");
            break;
        case HOMEKIT_EVENT_CLIENT_CONNECTED:
            printf("on_homekit_event: Client connected\n");
            break;
        case HOMEKIT_EVENT_CLIENT_VERIFIED:
            printf("on_homekit_event: Client verified\n");
            if (!paired ){
                paired = true;
            }
            break;
        case HOMEKIT_EVENT_CLIENT_DISCONNECTED:
            printf("on_homekit_event: Client disconnected\n");
            break;
        case HOMEKIT_EVENT_PAIRING_ADDED:
            printf("on_homekit_event: Pairing added\n");
            break;
        case HOMEKIT_EVENT_PAIRING_REMOVED:
            printf("on_homekit_event: Pairing removed\n");
            if (!homekit_is_paired())
            /* if we have no more pairtings then restart */
                printf("on_homekit_event: no more pairings so restart\n");
            sdk_system_restart();
            break;
        default:
            printf("on_homekit_event: Default event %d ", event);
    }
    
}


// GLOBAL CHARACTERISTICS
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, "Curla92");
homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Air Conditioner");
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, NULL);
homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, "Fujitsu AC");
homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, "2.7.1");
homekit_characteristic_t identify_function = HOMEKIT_CHARACTERISTIC_(IDENTIFY, identify);
homekit_characteristic_t service_name = HOMEKIT_CHARACTERISTIC_(NAME, "Fujitsu AC");
homekit_characteristic_t setup_name = HOMEKIT_CHARACTERISTIC_(NAME, "Setup");

// ACCESSORY NAME
void create_accessory_name() {
    // Accessory Name
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    int name_len = snprintf(NULL, 0, "Fujitsu AC-%02X%02X%02X", macaddr[3],
                            macaddr[4], macaddr[5]);
    
    char *name_value = malloc(name_len + 1);
    snprintf(name_value, name_len + 1, "Fujitsu AC-%02X%02X%02X", macaddr[3],
             macaddr[4], macaddr[5]);
    name.value = HOMEKIT_STRING(name_value);
    
    // Accessory Serial
    char *serial_value = malloc(13);
    snprintf(serial_value, 13, "%02X%02X%02X%02X%02X%02X", macaddr[0], macaddr[1],
             macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    serial.value = HOMEKIT_STRING(serial_value);
}

homekit_server_config_t config;

//CREATE ACCESSORY
void create_accessory() {
    
    uint8_t service_count = 3;
    
    if (show_setup.value.bool_value) {
        service_count++;
    }
    
    homekit_accessory_t **accessories = calloc(2, sizeof(homekit_accessory_t *));
    
    homekit_accessory_t *accessory = accessories[0] = calloc(1, sizeof(homekit_accessory_t));
    accessory->id = 1;
    accessory->category = homekit_accessory_category_thermostat;
    accessory->services = calloc(service_count, sizeof(homekit_service_t *));
    
    homekit_service_t *accessory_info = accessory->services[0] = calloc(1, sizeof(homekit_service_t));
    accessory_info->type = HOMEKIT_SERVICE_ACCESSORY_INFORMATION;
    accessory_info->characteristics = calloc(7, sizeof(homekit_characteristic_t *));
    
    accessory_info->characteristics[0] = &name;
    accessory_info->characteristics[1] = &manufacturer;
    accessory_info->characteristics[2] = &serial;
    accessory_info->characteristics[3] = &model;
    accessory_info->characteristics[4] = &revision;
    accessory_info->characteristics[5] = &identify_function;
    accessory_info->characteristics[6] = NULL;
    
    homekit_service_t *thermostat_function = accessory->services[1] = calloc(1, sizeof(homekit_service_t));
    thermostat_function->type = HOMEKIT_SERVICE_THERMOSTAT;
    thermostat_function->primary = true;
    thermostat_function->characteristics = calloc(11, sizeof(homekit_characteristic_t *));
    
    thermostat_function->characteristics[0] = &service_name;
    thermostat_function->characteristics[1] = &current_temperature;
    thermostat_function->characteristics[2] = &current_humidity;
    thermostat_function->characteristics[3] = &units;
    thermostat_function->characteristics[4] = &target_temperature;
    thermostat_function->characteristics[5] = &current_heating_cooling_state;
    thermostat_function->characteristics[6] = &target_heating_cooling_state;
    thermostat_function->characteristics[7] = &custom_fan;
    thermostat_function->characteristics[8] = &enable_swing;
    thermostat_function->characteristics[9] = &show_setup;
    thermostat_function->characteristics[10] = NULL;
    
    if (show_setup.value.bool_value) {
        homekit_service_t *thermostat_setup = accessory->services[2] = calloc(1, sizeof(homekit_service_t));
        thermostat_setup->type = HOMEKIT_SERVICE_CUSTOM_SETUP;
        thermostat_setup->primary = false;
        thermostat_setup->characteristics = calloc(10, sizeof(homekit_characteristic_t *));
        
        thermostat_setup->characteristics[0] = &setup_name;
        thermostat_setup->characteristics[1] = &device_type_name;
        thermostat_setup->characteristics[2] = &custom_init_state_th;
        thermostat_setup->characteristics[3] = &hum_offset;
        thermostat_setup->characteristics[4] = &temp_offset;
        thermostat_setup->characteristics[5] = &ip_addr;
        thermostat_setup->characteristics[6] = &wifi_reset;
        thermostat_setup->characteristics[7] = &reboot_device;
        thermostat_setup->characteristics[8] = &ota_firmware;
        thermostat_setup->characteristics[9] = NULL;
    } else {
        accessory->services[2] = NULL;
    }
    
    config.accessories = accessories;
    config.password = "277-66-227";
    config.setupId="1AI0",
    config.on_event = on_event;
    
    homekit_server_init(&config);
    
}

// WIFI
void on_wifi_event(wifi_config_event_t event) {
    if (event == WIFI_CONFIG_CONNECTED) {
        printf("CONNECTED TO >>> WIFI <<<\n");
        Wifi_Connected = true;
        led_code(led_gpio, WIFI_CONNECTED);
        
        create_accessory_name();
        create_accessory();
        
    } else if (event == WIFI_CONFIG_DISCONNECTED) {
        Wifi_Connected = false;
        printf("DISCONNECTED FROM >>> WIFI <<<\n");
    }
}

void hardware_init() {
    
    // LED INIT
    led_create(led_gpio, true);
    printf("Led gpio init -> %d\n", led_gpio);
    
    // DHT GPIO INIT
    gpio_set_pullup(TEMPERATURE_SENSOR_PIN, false, false);
    printf("DHT gpio init -> %d\n", TEMPERATURE_SENSOR_PIN);
    
    sdk_os_timer_setfn(&extra_func_timer, temperature_sensor, NULL);
    sdk_os_timer_arm(&extra_func_timer, 10000, 1);
    
    // IR Common INIT
    gpio_enable(GPIO_IR_PIN, GPIO_OUTPUT);
    gpio_write(GPIO_IR_PIN, 0);
    
    ir_set_pin(GPIO_IR_PIN);
    ir_set_frequency(38);
    printf("IR gpio init -> %d\n", GPIO_IR_PIN);
    printf("IR Frequency(38kHz)\n");
    
    printf("Clima Init -> Mode %d, Temp %.1fc°  \n", target_heating_cooling_state.value.int_value, target_temperature.value.float_value);
}

void user_init(void) {
    
    uart_set_baud(0, 115200);
    
    settings_init();
    
    hardware_init();
    
    sdk_os_timer_setfn(&reset_toggle_timer, reset_toggle, NULL);
    sdk_os_timer_setfn(&change_settings_timer, save_settings, NULL);
    sdk_os_timer_setfn(&save_states_timer, save_states, NULL);
    
    wifi_config_init2("Fujitsu AC", NULL, on_wifi_event);
}

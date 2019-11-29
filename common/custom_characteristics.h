#ifndef __HOMEKIT_CUSTOM_CHARACTERISTICS__
#define __HOMEKIT_CUSTOM_CHARACTERISTICS__

#define HOMEKIT_CUSTOM_UUID(value) (value "-03a1-4971-92bf-af2b7d833922")

#define HOMEKIT_SERVICE_CUSTOM_SETUP HOMEKIT_CUSTOM_UUID("F00000FF")

#define HOMEKIT_CHARACTERISTIC_CUSTOM_FAN HOMEKIT_CUSTOM_UUID("F0000104")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_FAN(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_FAN, \
.description = "Fan Speed", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {4}, \
.min_step = (float[]) {1}, \
.valid_values = { \
.count = 5, \
.values = (uint8_t[]) {0, 1, 2, 3, 4}, \
}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_SWING HOMEKIT_CUSTOM_UUID("F0000105")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_SWING(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_SWING, \
.description = "Vertical Swing", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_SHOW_SETUP HOMEKIT_CUSTOM_UUID("F0000106")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_SHOW_SETUP(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_SHOW_SETUP, \
.description = "Show Setup", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME HOMEKIT_CUSTOM_UUID("F0000107")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME, \
.description = "1) Actual Dev Type", \
.format = homekit_format_string, \
.permissions = homekit_permissions_paired_read, \
.value = HOMEKIT_STRING_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_INIT_STATE_TH HOMEKIT_CUSTOM_UUID("F0000108")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_INIT_STATE_TH(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_INIT_STATE_TH, \
.description = "2) Init State", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {1}, \
.min_step = (float[]) {1}, \
.valid_values = { \
.count = 2, \
.values = (uint8_t[]) {0, 1}, \
}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_TEMPERATURE_OFFSET HOMEKIT_CUSTOM_UUID("F0000109")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_TEMPERATURE_OFFSET(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_TEMPERATURE_OFFSET, \
.description = "3) Offset TEMP", \
.format = homekit_format_float, \
.unit = homekit_unit_celsius, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {-15}, \
.max_value = (float[]) {15}, \
.min_step = (float[]) {0.1}, \
.value = HOMEKIT_FLOAT_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_HUMIDITY_OFFSET HOMEKIT_CUSTOM_UUID("F0000110")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_HUMIDITY_OFFSET(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_HUMIDITY_OFFSET, \
.description = "4) Offset HUM", \
.format = homekit_format_float, \
.unit = homekit_unit_percentage, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {-15}, \
.max_value = (float[]) {15}, \
.min_step = (float[]) {1}, \
.value = HOMEKIT_FLOAT_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR HOMEKIT_CUSTOM_UUID("F0000111")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_IP_ADDR(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR, \
.description = "5) Wifi IP Addr", \
.format = homekit_format_string, \
.permissions = homekit_permissions_paired_read, \
.value = HOMEKIT_STRING_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RESET HOMEKIT_CUSTOM_UUID("F0000112")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_WIFI_RESET(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RESET, \
.description = "6) Wifi Reset", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE HOMEKIT_CUSTOM_UUID("F0000113")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE, \
.description = "7) Reboot", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_UPDATE HOMEKIT_CUSTOM_UUID("F0000114")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_OTA_UPDATE(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_UPDATE, \
.description = "8) Firmware Update", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#endif

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "include/wifi.h"
#include "include/mqtt.h"

static xTimerHandle timerMQTT;

void app_main(void)
{
    esp_err_t status = WIFI_FAIL_BIT;

    // Initialize storage NVS(non-volatile-storage)
    esp_err_t ret = nvs_flash_init();

    // Erase flash memory if it is not free
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    InitializeI2C();
    ConnectWifi();

    timerMQTT = xTimerCreate("timerMQTT", pdMS_TO_TICKS(10000), pdTRUE, (void *)0, mqtt_app_start);
    xTimerStart(timerMQTT, 1);
}
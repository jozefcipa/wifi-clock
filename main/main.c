#include <esp_err.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>

#include "datetime.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vfd.h"
#include "wifi.h"

void task_time(void *params) {
  bool is_high_brightness = false;

  while (1) {
    char time[9];  // format 00:00:00 + '\0'
    datetime_timef(time, sizeof(time));
    vfd_write(0, time);

    // If time is 04:00:00, restart ESP as there is a tiny delay that grows over time
    // and the time will be off by a few seconds after a few days
    // This is a little hack but it should do the trick
    if (strcmp(time, "04:00:00") == 0) {
      ESP_LOGI("TIME", "Restarting ESP to reset time");
      esp_restart();
      return;
    }

    // Dim the display during the night
    if (datetime_is_night() && is_high_brightness) {
      is_high_brightness = false;
      vfd_config(VFD_CONF_BRIGHTNESS, 0x19);  // 10% brightness
    } else if (!datetime_is_night() && !is_high_brightness) {
      is_high_brightness = true;
      vfd_config(VFD_CONF_BRIGHTNESS, 0xFF);  // 100% brightness
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void on_wifi_start() {
  vfd_write_scroll("Connecting to WiFi");
}

void on_wifi_err() {
  // Reset wifi credentials and restart ESP to initialize provisioning process
  wifi_reset();
  esp_restart();
}

void app_main() {
  // Init storage
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW("WARNING", "NVS flash initialization failed, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Init default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Init WiFi
  wifi_init(on_wifi_start, on_wifi_err);

  // Init VFD display
  vfd_init();
  // test VFD by turning all pixels on
  vfd_cmd(VFD_CMD_ALL_PIXELS_ON);
  vTaskDelay(pdMS_TO_TICKS(500));

  // Check WiFi provisioning status
  if (!wifi_is_provisioned()) {
    wifi_init_provisioning();
    vfd_write_scroll("Setup WiFi");
  } else {
    on_wifi_start();
    wifi_connect();
  }

  // Wait for WiFi connection
  wifi_wait_for_conn();

  // Fetch current time
  vfd_write_scroll("Syncing time");
  datetime_init();

  // Display time
  xTaskCreate(task_time, "TimeTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

  // Wait a bit and turn off WiFi as it's no longer needed
  vTaskDelay(pdMS_TO_TICKS(5000));
  wifi_off();
}

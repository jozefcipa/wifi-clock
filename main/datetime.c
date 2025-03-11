#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "lwip/ip_addr.h"
#include "time.h"

static const char *TAG = "TIME";

void datetime_init(void) {
  ESP_LOGI(TAG, "Fetching current time over NTP.");

  // init NTP to fetch current time from the server
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);

  int retry = 0;
  const int retry_count = 15;
  while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
    ESP_LOGI(TAG, "Fetching current time... (%d/%d)", retry, retry_count);

    // Add a short delay to avoid running the loop too fast
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1-second delay
  }

  esp_netif_sntp_deinit();

  // set timezone
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);  // Prague with the daylight saving time config
  tzset();
}

void datetime_timef(char* buff, size_t buff_size) {
  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  strftime(buff, buff_size, "%H:%M:%S", &timeinfo);
}
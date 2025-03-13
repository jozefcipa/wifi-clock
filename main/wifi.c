#include "wifi.h"

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "WIFI";
static bool wifi_stopped = false;

// This salt,verifier has been generated for username = "wifiprov" and password = "abcd1234"
// IMPORTANT NOTE: For production cases, this must be unique to every device
// and should come from device manufacturing partition.
static const char sec2_salt[] = {
    0x03, 0x6e, 0xe0, 0xc7, 0xbc, 0xb9, 0xed, 0xa8, 0x4c, 0x9e, 0xac, 0x97, 0xd9, 0x3d, 0xec, 0xf4};

static const char sec2_verifier[] = {
    0x7c, 0x7c, 0x85, 0x47, 0x65, 0x08, 0x94, 0x6d, 0xd6, 0x36, 0xaf, 0x37, 0xd7, 0xe8, 0x91, 0x43,
    0x78, 0xcf, 0xfd, 0x61, 0x6c, 0x59, 0xd2, 0xf8, 0x39, 0x08, 0x12, 0x72, 0x38, 0xde, 0x9e, 0x24,
    0xa4, 0x70, 0x26, 0x1c, 0xdf, 0xa9, 0x03, 0xc2, 0xb2, 0x70, 0xe7, 0xb1, 0x32, 0x24, 0xda, 0x11,
    0x1d, 0x97, 0x18, 0xdc, 0x60, 0x72, 0x08, 0xcc, 0x9a, 0xc9, 0x0c, 0x48, 0x27, 0xe2, 0xae, 0x89,
    0xaa, 0x16, 0x25, 0xb8, 0x04, 0xd2, 0x1a, 0x9b, 0x3a, 0x8f, 0x37, 0xf6, 0xe4, 0x3a, 0x71, 0x2e,
    0xe1, 0x27, 0x86, 0x6e, 0xad, 0xce, 0x28, 0xff, 0x54, 0x46, 0x60, 0x1f, 0xb9, 0x96, 0x87, 0xdc,
    0x57, 0x40, 0xa7, 0xd4, 0x6c, 0xc9, 0x77, 0x54, 0xdc, 0x16, 0x82, 0xf0, 0xed, 0x35, 0x6a, 0xc4,
    0x70, 0xad, 0x3d, 0x90, 0xb5, 0x81, 0x94, 0x70, 0xd7, 0xbc, 0x65, 0xb2, 0xd5, 0x18, 0xe0, 0x2e,
    0xc3, 0xa5, 0xf9, 0x68, 0xdd, 0x64, 0x7b, 0xb8, 0xb7, 0x3c, 0x9c, 0xfc, 0x00, 0xd8, 0x71, 0x7e,
    0xb7, 0x9a, 0x7c, 0xb1, 0xb7, 0xc2, 0xc3, 0x18, 0x34, 0x29, 0x32, 0x43, 0x3e, 0x00, 0x99, 0xe9,
    0x82, 0x94, 0xe3, 0xd8, 0x2a, 0xb0, 0x96, 0x29, 0xb7, 0xdf, 0x0e, 0x5f, 0x08, 0x33, 0x40, 0x76,
    0x52, 0x91, 0x32, 0x00, 0x9f, 0x97, 0x2c, 0x89, 0x6c, 0x39, 0x1e, 0xc8, 0x28, 0x05, 0x44, 0x17,
    0x3f, 0x68, 0x02, 0x8a, 0x9f, 0x44, 0x61, 0xd1, 0xf5, 0xa1, 0x7e, 0x5a, 0x70, 0xd2, 0xc7, 0x23,
    0x81, 0xcb, 0x38, 0x68, 0xe4, 0x2c, 0x20, 0xbc, 0x40, 0x57, 0x76, 0x17, 0xbd, 0x08, 0xb8, 0x96,
    0xbc, 0x26, 0xeb, 0x32, 0x46, 0x69, 0x35, 0x05, 0x8c, 0x15, 0x70, 0xd9, 0x1b, 0xe9, 0xbe, 0xcc,
    0xa9, 0x38, 0xa6, 0x67, 0xf0, 0xad, 0x50, 0x13, 0x19, 0x72, 0x64, 0xbf, 0x52, 0xc2, 0x34, 0xe2,
    0x1b, 0x11, 0x79, 0x74, 0x72, 0xbd, 0x34, 0x5b, 0xb1, 0xe2, 0xfd, 0x66, 0x73, 0xfe, 0x71, 0x64,
    0x74, 0xd0, 0x4e, 0xbc, 0x51, 0x24, 0x19, 0x40, 0x87, 0x0e, 0x92, 0x40, 0xe6, 0x21, 0xe7, 0x2d,
    0x4e, 0x37, 0x76, 0x2f, 0x2e, 0xe2, 0x68, 0xc7, 0x89, 0xe8, 0x32, 0x13, 0x42, 0x06, 0x84, 0x84,
    0x53, 0x4a, 0xb3, 0x0c, 0x1b, 0x4c, 0x8d, 0x1c, 0x51, 0x97, 0x19, 0xab, 0xae, 0x77, 0xff, 0xdb,
    0xec, 0xf0, 0x10, 0x95, 0x34, 0x33, 0x6b, 0xcb, 0x3e, 0x84, 0x0f, 0xb9, 0xd8, 0x5f, 0xb8, 0xa0,
    0xb8, 0x55, 0x53, 0x3e, 0x70, 0xf7, 0x18, 0xf5, 0xce, 0x7b, 0x4e, 0xbf, 0x27, 0xce, 0xce, 0xa8,
    0xb3, 0xbe, 0x40, 0xc5, 0xc5, 0x32, 0x29, 0x3e, 0x71, 0x64, 0x9e, 0xde, 0x8c, 0xf6, 0x75, 0xa1,
    0xe6, 0xf6, 0x53, 0xc8, 0x31, 0xa8, 0x78, 0xde, 0x50, 0x40, 0xf7, 0x62, 0xde, 0x36, 0xb2, 0xba};

// This will set a custom 128 bit UUID which will be included in the BLE advertisement
// and will correspond to the primary GATT service that provides provisioning
// endpoints as GATT characteristics. Each GATT characteristic will be
// formed using the primary service UUID as base, with different auto assigned
// 12th and 13th bytes (assume counting starts from 0th byte). The client side
// applications must identify the endpoints by reading the User Characteristic
// Description descriptor (0x2901) for each characteristic, which contains the
// endpoint name of the characteristic
static uint8_t custom_service_uuid[] = {
    // LSB <------------------------------------------> MSB
    0xb4,
    0xdf,
    0x5a,
    0x1c,
    0x3f,
    0x6b,
    0xf4,
    0xbf,
    0xea,
    0x4a,
    0x82,
    0x03,
    0x04,
    0x90,
    0x1a,
    0x02,
};

// Signal Wi-Fi events on this event-group
static const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

static wifi_evt_handler on_wifi_start;
static wifi_evt_handler on_wifi_err;

static int wifi_failed_attempts = 0;

// Event handlers
static void on_network_prov_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  switch (event_id) {
    case NETWORK_PROV_START:
      ESP_LOGI(TAG, "Provisioning started");
      break;
    case NETWORK_PROV_WIFI_CRED_RECV: {
      wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
      ESP_LOGI(TAG,
               "Received Wi-Fi credentials\n\tSSID     : %s\n\tPassword : %s",
               (const char *)wifi_sta_cfg->ssid,
               (const char *)wifi_sta_cfg->password);

      on_wifi_start();

      break;
    }
    case NETWORK_PROV_WIFI_CRED_FAIL: {
      network_prov_wifi_sta_fail_reason_t *reason = (network_prov_wifi_sta_fail_reason_t *)event_data;
      ESP_LOGE(TAG,
               "Provisioning failed!\n\tReason : %s"
               "\n\tPlease reset to factory and retry provisioning",
               (*reason == NETWORK_PROV_WIFI_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

      break;
    }
    case NETWORK_PROV_WIFI_CRED_SUCCESS:
      ESP_LOGI(TAG, "Provisioning successful");
      break;
    case NETWORK_PROV_END:
      network_prov_mgr_deinit();
      break;
    default:
      break;
  }
}

static void on_wifi_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  switch (event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      break;
    // This event is called when wifi is disconnected while running
    // but also when booting up and cannot connect to the provisioned network
    case WIFI_EVENT_STA_DISCONNECTED:
      // don't try to reconnect when wifi has been turned off explicitly
      if (wifi_stopped) {
        break;
      }

      if (++wifi_failed_attempts > 3) {
        ESP_LOGI(TAG, "WiFi connection failed 3 times.");
        on_wifi_err();
      } else {
        ESP_LOGI(TAG, "WiFi connection failed. Retrying...");
        esp_wifi_connect();
      }

      break;
    default:
      break;
  }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Connected with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    
    // signal main application to continue execution
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
  }
}

static void on_protocomm_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
    switch (event_id) {
      case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
        ESP_LOGI(TAG, "BLE transport: Connected!");
        break;
      case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
        ESP_LOGI(TAG, "BLE transport: Disconnected!");
        break;
      default:
        break;
    }

  } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
    switch (event_id) {
      case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
        ESP_LOGI(TAG, "Secured session established!");
        break;
      case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
        ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
        break;
      case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
        ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
        break;
      default:
        break;
    }
  }
}

static void get_device_name(char *device_name, size_t max) {
  uint8_t eth_mac[6];
  const char *ssid_prefix = "PROV_";
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
  snprintf(device_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

void wifi_init(wifi_evt_handler on_start, wifi_evt_handler on_err) {
  // Init WiFi
  ESP_ERROR_CHECK(esp_netif_init());

  // Register event callbacks
  on_wifi_start = on_start;
  on_wifi_err = on_err;

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, on_network_prov_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, on_protocomm_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, on_protocomm_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_ip_event, NULL));

  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  esp_netif_set_hostname(sta_netif, "Wifi-Clock");

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_event_group = xEventGroupCreate();
}

void wifi_wait_for_conn() {
    // Wait for Wi-Fi connection
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);
}

bool wifi_is_provisioned(void) {
  bool provisioned = false;
  ESP_ERROR_CHECK(network_prov_mgr_is_wifi_provisioned(&provisioned));
  return provisioned;
}

void wifi_init_provisioning() {
  network_prov_mgr_config_t config = {
      .scheme = network_prov_scheme_ble,
      // after the provisioning is done, release the associated memory,
      // as we no longer use BT nor BLE in our application
      .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
  };

  ESP_LOGI(TAG, "Starting provisioning");
  ESP_ERROR_CHECK(network_prov_mgr_init(config));

  char device_name[12];
  get_device_name(device_name, sizeof(device_name));

  // The username must be the same one, which has been used in the generation of salt and verifier */
  // This pop field represents the password that will be used to generate salt and verifier.
  // The field is present here in order to generate the QR code containing password.
  const char *username = WIFI_PROV_SEC2_USERNAME;
  const char *pop = WIFI_PROV_SEC2_PWD;

  network_prov_security2_params_t sec_params = {
      .salt = sec2_salt,
      .salt_len = sizeof(sec2_salt),
      .verifier = sec2_verifier,
      .verifier_len = sizeof(sec2_verifier)};

  network_prov_scheme_ble_set_service_uuid(custom_service_uuid);
  ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(NETWORK_PROV_SECURITY_2, (const void *)&sec_params, device_name, NULL));

  char payload[150] = {0};
  snprintf(payload, sizeof(payload),
           "{\"ver\":\"%s\",\"name\":\"%s\""
           ",\"username\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
           PROV_QR_VERSION, device_name, username, pop, PROV_TRANSPORT_BLE);
  ESP_LOGI(TAG, "Generated WiFi provisioning payload: %s", payload);
}

void wifi_connect() {
  ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

  wifi_sta_config_t wifi_sta_cfg;
  ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, (wifi_config_t *) &wifi_sta_cfg));
  ESP_LOGI(TAG, "Connecting to WiFi %s", (const char *) wifi_sta_cfg.ssid);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_reset() {
  ESP_LOGI(TAG, "Resetting Wi-Fi credentials");
  wifi_config_t wifi_cfg_empty = {0};
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg_empty));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
}

void wifi_off() {
  ESP_LOGI(TAG, "Turning off WiFi");

  wifi_stopped = true;

  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());
}
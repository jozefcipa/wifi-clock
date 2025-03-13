#include <stdbool.h>
#include <stddef.h>

#define WIFI_PROV_SEC2_USERNAME "wifiprov"
#define WIFI_PROV_SEC2_PWD "abcd1234"
#define PROV_MGR_MAX_RETRY_CNT 3
#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"

typedef void (*wifi_evt_handler)(void);

void wifi_init(wifi_evt_handler on_start, wifi_evt_handler on_err);
bool wifi_is_provisioned();
void wifi_init_provisioning();
void wifi_connect();
void wifi_reset();
void wifi_wait_for_conn();
void wifi_off();
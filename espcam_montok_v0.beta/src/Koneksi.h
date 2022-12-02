#include "Type.h"

void KoneksiInit();
 
bool init_wifi();

void scan_wifi_networks();

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

void callback_show_ip(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

void disconnect_bluetooth();

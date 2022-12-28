#include "Type.h"

void KoneksiInit();

void BluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

void WifiInit();

char *extract_between(const char *str, const char *p1, const char *p2);

void WiFiQRParsing(const char *input, char **destSSID, char **destPASS);

bool WiFIQRCheck(const char *input);

int WiFiFormParsing(String str, char **p, int size, char **pdata);

void disconnect_bluetooth();

int CheckPingGateway();

void CheckPingServer();

void CheckPingSend();

void CheckPingBackground(void *parameter);

#include <tokenonline.h>
const char configFile[] = "/config.json";
char productName[] = "TOKEN-ONLINE_";

bool SettingsInit()
{
  int response;
  settings.readSettings(configFile);
  strcpy(idDevice, settings.getChar("device.id"));
  Serial.println(idDevice);
  Serial.println(settings.getChar("bluetooth.name"));

  if (strlen(settings.getChar("bluetooth.name")) == 0)
  {
    Serial.println(" ZERO BT NAME");
    response = settings.setChar("bluetooth.name", strcat(productName, idDevice));
    settings.writeSettings(configFile);
    ESP.restart();
  }
  else
    strcpy(bluetooth_name, settings.getChar("bluetooth.name"));

  Serial.println(idDevice);
  Serial.println(bluetooth_name);
  strcpy(wifissidbuff, settings.getChar("wifi.ssid"));
  strcpy(wifipassbuff, settings.getChar("wifi.pass"));
  Serial.println(wifissidbuff);
  Serial.println(wifipassbuff);

  return true;
}

bool SettingsLoad();
bool SettingsWifiSave(char *ssid, char *pass)
{
  settings.setChar("wifi.ssid", ssid);
  settings.setChar("wifi.pass", pass);

  Serial.println("---------- Save settings ----------");
  settings.writeSettings("/config.json");
  Serial.println("---------- Read settings from file ----------");
  settings.readSettings("/config.json");
  Serial.println("---------- New settings content ----------");

  return false;
}
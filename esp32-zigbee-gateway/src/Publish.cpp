#include "zigbee_gateway.h"

static String serverName = "192.168.1.1";
const char channelId = 'a';
const char customerID = 'b';
String mqttUser = "das";
String mqttPass = "mgi2022";
bool mqttConnectEth()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!mqttClientEth.connected())
  {
    if (serverName.length() <= 0)
      break;

    mqttClientEth.setServer(serverName.c_str(), 1883);
    // //server.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';

    // Setting mqtt gaes
    if (mqttClientEth.connect(clientId, mqttUser.c_str(), mqttPass.c_str()))
    {
      // //server.println("Established:" + String(clientId));
      return true;
    }
    else
    {
      if (!--retry)
        break;
      // delay(3000);
    }
  }
  return false;
}

void mqttPublish(String msg)
{
  String path = "aaa";
  mqttClientEth.publish(path.c_str(), msg.c_str());
}
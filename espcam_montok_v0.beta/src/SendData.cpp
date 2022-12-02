#include <montok.h>

IPAddress IpServer(203, 194, 112, 238);
const int MqttPort = 1883;
const String MqttUser = "das";
const String MqttPass = "mgi2022";
const String MqttChannel = "monTok/";
const char *ID = "monTok";   // Name of our device, must be unique
const char *TOPIC = "token"; // Topic to subcribe to

bool MqttConnect()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!Mqtt.connected())
  {
    if (MqttUser.length() <= 0)
      break;

    Mqtt.setServer(IpServer, MqttPort);
    Mqtt.setCallback(MqttCallback);
    Serial.println(String("Attempting MQTT broker:") + IpServer);

    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';
    // Setting mqtt gaes
    if (Mqtt.connect(clientId, MqttUser.c_str(), MqttPass.c_str()))
    {
      Serial.println("Established:" + String(clientId));
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

void MqttPublish(String msg)
{
  String path = MqttChannel + idDevice;
  Mqtt.publish(path.c_str(), msg.c_str());
}

void MqttCallback(char *topic, byte *payload, unsigned int length)
{

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }

  Serial.println();
  Serial.println("-----------------------");
}
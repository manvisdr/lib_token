#include <Type.h>

typedef uint8_t byte;
bool MqttConnect();
void MqttPublish(String msg);
void MqttCallback(char *topic, byte *payload, unsigned int length);
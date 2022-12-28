#include <Type.h>

typedef uint8_t byte;
void MqttInit();
bool MqttConnect();
void MqttPublish(String msg);
void MqttCustomPublish(char *path, String msg);
void MqttPublishStatus(String msg);
void MqttTopicInit();
void MqttReconnect();
String body_Nominal();
String body_Saldo();
String headerNominal(char *status, size_t length);
String headerSaldo(char *token, size_t length);
String sendImageViewKWH(char *sn);
String sendImageNominal(char *status, uint8_t *data_pic, size_t size_pic);
String sendImageNominal(char *status, char *filename);
String sendImageSaldo(char *token, uint8_t *data_pic, size_t size_pic);
String sendImageSaldo(char *token, char *filename);
String headerNotifSaldo(char *sn);
String sendNotifSaldo(char *sn);
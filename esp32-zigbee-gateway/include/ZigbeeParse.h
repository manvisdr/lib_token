#ifndef ZigbeeParse_H
#define ZigbeeParse_H
#include "Type.h"

void bytetoHexChar(uint8_t ubyte, uint8_t *uHexChar);
void bytestoHexChars(uint8_t *ubyte, int32_t len, uint8_t *uHexChar);
void hexChartoByte(uint8_t *uHexChar, uint8_t *ubyte);
void hexCharsToBytes(uint8_t *uHexChar, int32_t len, uint8_t *ubyte);
String convertToString(char *a, int size);
int zigbee_message_handler(zigbee_msg_t &zigbee_msg);

#endif
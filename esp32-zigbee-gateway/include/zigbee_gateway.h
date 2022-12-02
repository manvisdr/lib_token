#include <Arduino.h>
#include <stdint.h>
#include <U8g2lib.h>
#include <zb_znp.h>
#include <zb_zcl.h>
#include <SoftwareSerial.h>
#include <EthernetSPI2.h>
#include <PubSubClient.h>
#include <GEM_u8g2.h>
#include <OneButton.h>
#include "NavButton.h"
#include "Publish.h"
#include "LAN.h"
#include "Type.h"
#include "ZigbeeParse.h"

#define PIN_NAV_MID 32
#define PIN_NAV_RHT 25
#define PIN_NAV_LFT 26
#define PIN_NAV_DWN 29
#define PIN_NAV_UP 23

#define PIN_LED_PROC 11
#define PIN_LED_WARN 15

extern OneButton MIDButton;
extern EthernetClient ethClient;
extern PubSubClient mqttClientEth;

extern char zc_ieee_addr[17];
extern char zr_ieee_addr[17];
extern uint8_t control_switch_ieee[17];
extern uint8_t control_switch_cmd_seq;
extern uint16_t control_switch_address;

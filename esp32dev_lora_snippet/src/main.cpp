#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "LoraEncoder.h"

static const PROGMEM u1_t NWKSKEY[16] = {0x38, 0x75, 0xbd, 0x50, 0xd7, 0x60, 0x44, 0x49, 0x79, 0x19, 0xfc, 0x9d, 0x9e, 0xbc, 0xa1, 0xbc};
static const u1_t PROGMEM APPSKEY[16] = {0x86, 0x8b, 0x8d, 0xe3, 0x78, 0x71, 0x39, 0x1e, 0x88, 0xc1, 0x7e, 0xe3, 0x89, 0xdb, 0x13, 0xf8};
static const u4_t DEVADDR = 0x019d4f0f;

void os_getArtEui(u1_t *buf) {}
void os_getDevEui(u1_t *buf) {}
void os_getDevKey(u1_t *buf) {}

static uint8_t payload[5];
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 30;

const lmic_pinmap lmic_pins = {
    .nss = 5,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 26,
    .dio = {25, 32, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 8, // LBT cal for the Adafruit Feather M0 LoRa, in dB
    .spi_freq = 8000000,
};

void do_send(osjob_t *j)
{
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else
  {

    payload[0] = 0x01;
    payload[1] = 0x02;
    payload[2] = 0x03;
    payload[3] = 0x04;

    // prepare upstream data transmission at the next possible time.
    // transmit on port 1 (the first parameter); you can use any value from 1 to 223 (others are reserved).
    // don't request an ack (the last parameter, if not zero, requests an ack from the network).
    // Remember, acks consume a lot of network resources; don't ask for an ack unless you really need it.
    LMIC_setTxData2(1, payload, sizeof(payload) - 1, 1);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent(ev_t ev)
{
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev)
  {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED"));
    break;

  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack"));
    if (LMIC.dataLen)
    {
      Serial.println(F("Received "));
      Serial.println(LMIC.dataLen);
      Serial.println(F(" bytes of payload"));
    }
    // Schedule next transmission
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
    break;
  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;

  case EV_TXSTART:
    Serial.println(F("EV_TXSTART"));
    break;
  case EV_TXCANCELED:
    Serial.println(F("EV_TXCANCELED"));
    break;
  case EV_RXSTART:

    break;
  case EV_JOIN_TXCOMPLETE:
    Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
    break;
  default:
    Serial.print(F("Unknown event: "));
    Serial.println((unsigned)ev);
    break;
  }
}

byte buffer[4];
LoraEncoder encoder(buffer);

void setup()
{
  Serial.begin(115200);
  delay(5000);
  while (!Serial)
    ;
  Serial.begin(115200);
  delay(100);
  Serial.println(F("Starting"));

  // LMIC init
  os_init();
  LMIC_reset();
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession(0x13, DEVADDR, nwkskey, appskey);

  LMIC_selectSubBand(7);

  // LMIC_enableChannel(16);
  // Disable link check validation
  LMIC_setLinkCheckMode(0);

  // TTN uses SF9 for its RX2 window.
  // LMIC.dn2Dr = DR_SF9;

  // Set data rate and transmit power for uplink
  LMIC_setDrTxpow(DR_SF7, 14);

  // Start job
  do_send(&sendjob);

  // encoder.writeUnixtime(1467632413);
  // for (int i = 0; i < sizeof(buffer); i++)
  //   Serial.printf("%02x", buffer[i]);
}

void loop()
{
  os_runloop_once();
}

// #include <Arduino.h>
// #include <SPI.h>

// #include <hal/hal_io.h>
// #include <hal/print_debug.h>
// #include <keyhandler.h>
// #include <lmic.h>

// #define DEVICE_TESTESP32
// #include "lorakeys.h"

// void do_send();

// // Schedule TX every this many seconds (might become longer due to duty
// // cycle limitations).
// constexpr OsDeltaTime TX_INTERVAL = OsDeltaTime::from_sec(135);

// constexpr unsigned int BAUDRATE = 115200;
// // Pin mapping
// constexpr lmic_pinmap lmic_pins = {
//     .nss = 5,
//     .prepare_antenna_tx = nullptr,
//     .rst = 26,
//     .dio = {25, 32},
// };

// RadioSx1276 radio{lmic_pins};
// LmicUs915 LMIC{radio};

// OsTime nextSend;

// void onEvent(EventType ev)
// {
//   switch (ev)
//   {
//   case EventType::JOINING:
//     PRINT_DEBUG(2, F("EV_JOINING"));
//     //        LMIC.setDrJoin(0);
//     break;
//   case EventType::JOINED:
//     PRINT_DEBUG(2, F("EV_JOINED"));
//     // disable ADR because if will be mobile.
//     // LMIC.setLinkCheckMode(false);
//     break;
//   case EventType::JOIN_FAILED:
//     PRINT_DEBUG(2, F("EV_JOIN_FAILED"));
//     break;
//   case EventType::TXCOMPLETE:
//     PRINT_DEBUG(2, F("EV_TXCOMPLETE (includes waiting for RX windows)"));
//     if (LMIC.getTxRxFlags().test(TxRxStatus::ACK))
//     {
//       PRINT_DEBUG(1, F("Received ack"));
//     }
//     if (LMIC.getDataLen())
//     {
//       PRINT_DEBUG(1, F("Received %d bytes of payload"), LMIC.getDataLen());
//       auto data = LMIC.getData();
//       if (data)
//       {
//         uint8_t port = LMIC.getPort();
//       }
//     }
//     break;
//   case EventType::RESET:
//     PRINT_DEBUG(2, F("EV_RESET"));
//     break;
//   case EventType::LINK_DEAD:
//     PRINT_DEBUG(2, F("EV_LINK_DEAD"));
//     break;
//   case EventType::LINK_ALIVE:
//     PRINT_DEBUG(2, F("EV_LINK_ALIVE"));
//     break;
//   default:
//     PRINT_DEBUG(2, F("Unknown event"));
//     break;
//   }
// }

// void do_send()
// {
//   // Some analog value
//   // val = analogRead(A1) >> 4;
//   uint8_t val = temperatureRead();
//   // Prepare upstream data transmission at the next possible time.
//   LMIC.setTxData2(2, &val, 1, false);
//   PRINT_DEBUG(1, F("Packet queued"));
//   nextSend = os_getTime() + TX_INTERVAL;
// }

// void setup()
// {

//   if (debugLevel > 0)
//   {
//     Serial.begin(BAUDRATE);
//   }

//   SPI.begin();
//   // LMIC init
//   os_init();
//   LMIC.init();
//   // Reset the MAC state. Session and pending data transfers will be discarded.
//   LMIC.reset();

//   const uint32_t TTN_NET_ID = 0x000013;
//   const uint32_t DEV_ADRESS = 0x00000001;
//   const uint8_t NET_KEY[] = {0x38, 0x75, 0xbd, 0x50, 0xd7, 0x60, 0x44, 0x49, 0x79, 0x19, 0xfc, 0x9d, 0x9e, 0xbc, 0xa1, 0xbc};
//   const uint8_t APP_KEY[] = {0x86, 0x8b, 0x8d, 0xe3, 0x78, 0x71, 0x39, 0x1e, 0x88, 0xc1, 0x7e, 0xe3, 0x89, 0xdb, 0x13, 0xf8};

//   AesKey appkey;
//   std::copy(APP_KEY, APP_KEY + 16, appkey.begin());
//   AesKey netkey;
//   std::copy(NET_KEY, NET_KEY + 16, netkey.begin());
//   LMIC.setSession(TTN_NET_ID, DEV_ADRESS, netkey, appkey);

//   LMIC.setDrTx(5);
//   LMIC.setEventCallBack(onEvent);
//   LMIC.selectSubBand(4);

//   // set clock error to allow good connection.
//   LMIC.setClockError(MAX_CLOCK_ERROR * 3 / 100);
//   LMIC.setAntennaPowerAdjustment(-14);

//   LMIC.setRx2Parameter(923300000, SF12);
//   // Start job (sending automatically starts OTAA too)
//   nextSend = os_getTime();
// }

// void loop()
// {

//   OsDeltaTime freeTimeBeforeNextCall = LMIC.run();

//   if (freeTimeBeforeNextCall > OsDeltaTime::from_ms(10))
//   {
//     // we have more than 10 ms to do some work.
//     // the test must be adapted from the time spend in other task
//     if (nextSend < os_getTime())
//     {
//       if (LMIC.getOpMode().test(OpState::TXRXPEND))
//       {
//         PRINT_DEBUG(1, F("OpState::TXRXPEND, not sending"));
//       }
//       else
//       {
//         do_send();
//       }
//     }
//     else
//     {
//       OsDeltaTime freeTimeBeforeSend = nextSend - os_getTime();
//       OsDeltaTime to_wait =
//           std::min(freeTimeBeforeNextCall, freeTimeBeforeSend);
//       delay(to_wait.to_ms() / 2);
//     }
//   }
// }

// #include <lorawan.h>

// // ABP Credentials
// const char *devAddr = "019d4f0f";
// const char *nwkSKey = "3875bd50d76044497919fc9d9ebca1bc";
// const char *appSKey = "868b8de37871391e88c17ee389db13f8";

// const unsigned long interval = 5000; // 10 s interval to send message
// unsigned long previousMillis = 0;    // will store last time message sent
// unsigned int counter = 0;            // message counter

// char myStr[50];
// char outStr[255];
// byte recvStatus = 0;

// const sRFM_pins RFM_pins = {
//     .CS = 5,
//     .RST = 26,
//     .DIO0 = 25,
//     .DIO1 = 32,
// }; // DEF PIN LORA

// void setup()
// {
//   // Setup loraid access
//   Serial.begin(115200);
//   delay(5000);
//   Serial.println("Start..");
//   if (!lora.init())
//   {
//     Serial.println("RFM95 not detected");
//     delay(5000);
//     return;
//   }

//   // Set LoRaWAN Class change CLASS_A or CLASS_C
//   lora.setDeviceClass(CLASS_C);

//   // Set Data Rate
//   lora.setDataRate(SF7BW125);

//   // set channel to random
//   lora.setChannel(MULTI);

//   // Put ABP Key and DevAddress here
//   lora.setNwkSKey(nwkSKey);
//   lora.setAppSKey(appSKey);
//   lora.setDevAddr(devAddr);
// }

// void loop()
// {

//   // Check interval overflow
//   if (millis() - previousMillis > interval)
//   {
//     previousMillis = millis();

//     sprintf(myStr, "Counter-%d", counter);

//     Serial.print("Sending: ");
//     Serial.println(myStr);

//     lora.sendUplink(myStr, strlen(myStr), 1, 1);
//     counter++;
//   }

//   bool ACKStat = lora.readAck();
//   if (ACKStat)
//     Serial.println("ACK DOWNLINK");

//   // Check Lora RX
//   lora.update();
// }
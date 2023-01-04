#include <tokenonline.h>

// bool DetectionLowToken()
// {
//   int cntDetectLong;
//   bool timerStart;
//   bool lowToken;
//   long millisNow = millis();
//   long millisDetectLong;
//   long millisNotDetect;
//   if (SoundDetectLong() and !timerStart)
//   {
//     millisDetectLong = millisNow;
//     timerStart = true; // so start value is not updated next time around
//     if (millisNow - millisDetectLong >= 1000)
//     {
//       cntDetectLong++;
//       if (cntDetectLong > 5)
//       {
//         cntDetectLong = 0;
//         timerStart = false;
//       }
//     }
//   }
//   else
//   {
//     millisNotDetect = millisNow;
//     if (millisNow - millisNotDetect > 5000)
//     {
//       cntDetectLong = 0;
//       timerStart = false;
//       lowToken = false;
//     }
//   }
//   return timerStart;
// }
Timeout timerLowSaldo;

void ProccessInit()
{
  timerLowSaldo.prepare(5000);
  timerSolenoid.prepare(2000);
}

int counterLowSaldo;
bool DetecLowToken()
{
  bool detect;
  if (SoundDetectLong())
  {
    timerLowSaldo.start();
    counterLowSaldo++;
    detect = true;
  }
  else if (timerLowSaldo.time_over())
  {
    // Serial.print("TIMEOUT...");
    // Serial.println("RESET");
    counterLowSaldo = 0;
    detect = false;
  }

  if (/* timer.time_over()  and */ counterLowSaldo > 30)
  {
    Serial.println("DETECK LOW TOKEN");
    Serial.println("SEND NOTIFICATION");
    MqttPublishStatus("DETECTING LOW TOKEN");

    sendNotifSaldo(idDevice);
    counterLowSaldo = 0;
    detect = false;
  }
  return detect;
}

void GetKwhProcess()
{
  if (received_msg and strcmp(received_topic, topicREQView) == 0 /* and strcmp(received_payload, "view") == 0*/)
  {
    MqttPublishStatus("REQUESTING BALANCE");
    Serial.println("GET_KWH");
    Serial.println(sendImageViewKWH(idDevice));
    received_msg = false;
  }
}

void TokenProcess(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQToken) == 0) and received_length == 20)
  {
    MqttPublishStatus("EXECUTING TOKEN");
    for (int i = 0; i < received_length; i++)
    {
      Serial.printf("%d ", i);
      char chars = received_payload[i];
      switch (chars)
      {
      case '0':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(10);
        delay(1000);
        break;

      case '1':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(0);
        delay(1000);
        break;

      case '2':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        ////MechanicTyping(1);
        delay(1000);
        break;
      case '3':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(2);
        delay(1000);
        break;
      case '4':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(3);
        delay(1000);
        break;
      case '5':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(4);
        delay(1000);
        break;
      case '6':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(5);
        delay(1000);
        break;
      case '7':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(6);
        delay(1000);
        break;
      case '8':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(7);
        delay(1000);
        break;
      case '9':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(8);
        delay(1000);
        break;
      default:
        break;
      }
    }

    // MechanicEnter();

    ProccessSendSaldo(received_payload);

    delay(5000);

    ProccessSendNominal("success");

    // if (SoundDetectLong())
    // {

    //   fb = esp_camera_fb_get();
    //   if (!fb)
    //   {
    //     Serial.println("Camera capture failed");
    //     delay(1000);
    //     ESP.restart();
    //   }
    //   esp_camera_fb_return(fb);
    //   delay(4000);

    //   ProccessSendNominal("success", fb->buf, fb->len);
    // }
    // else
    //   ProccessSendNominal("gagal", fb->buf, fb->len);

    received_msg = false;
  }
}

void ProccessSendSaldo(char *payload)
{
  Serial.println(payload);
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  MqttPublishStatus("SEND IMAGE SALDO");
  // res = sendImageNominal("success", "/dummy.jpg"); //dummy
  String res = sendImageSaldo(received_payload, fb->buf, fb->len);
  Serial.println(res);
  esp_camera_fb_return(fb);
}

void ProccessSendNominal(char *status)
{
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  // MqttPublishStatus("SEND IMAGE NOMINAL");
  // res = sendImageNominal("success", "/dummy.jpg"); //dummy
  String res = sendImageNominal(status, fb->buf, fb->len);
  Serial.println(res);
  esp_camera_fb_return(fb);
}
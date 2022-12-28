#include <tokenonline.h>

#define MECHANIC_TIMEOUT 5000

bool MechanicTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= MECHANIC_TIMEOUT)
    return true;
  else
    return false;
} // CheckTimeout

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);
  for (int i = 0; i < 12; i++)
  {
    mcp.pinMode(i, OUTPUT); // MCP23017 pins are set for inputs
    // delay(10);
  }
  for (int i = 0; i < 12; i++)
  {
    MechanicTyping(i);
    // delay(10);
  }
}

bool MechanicTyping(int pin)
{
  mcp.digitalWrite(pin, HIGH);
  delay(150);
  mcp.digitalWrite(pin, LOW);
  return true;
}

bool MechanicTyping(char *pin)
{
  MechanicTyping(atoi(pin));
  return true;
}

void MechanicSelectNoSound(char chars)
{
  char buff[2];
  unsigned long timeNow;
  unsigned long timeStart;
  bool state = false;
  switch (chars)
  {
  case '0':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(10);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }
  case '1':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(0);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '2':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(1);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '3':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(2);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '4':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(3);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '5':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(4);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '6':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(5);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '7':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(6);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '8':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(7);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  case '9':

    Serial.printf("SELENOID %c ON\n", chars);
    MqttPublishStatus("SELENOID " + String(chars) + " ON");
    MechanicTyping(8);
    // timerSolenoid.start();
    // while (!state)
    // {

    //   if (timerSolenoid.time_over())
    //   {
    //     Serial.printf("SELENOID %c OFF\n", chars);
    //     state = true;
    //     break;
    //   }
    // }

  default:
    break;
  }
}

void MechanicSelect(char chars)
{
  char buff[2];
  unsigned long timeNow;
  unsigned long timeStart;
  switch (chars)
  {
  case '0':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();
      MechanicTyping(chars);

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
  case '1':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();
      MechanicTyping(SOL_1);

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '2':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();
      MechanicTyping(SOL_2);

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '3':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '4':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '5':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '6':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '7':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '8':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  case '9':
    timeStart = millis();
    Serial.printf("SELENOID %c ON\n", chars);
    while (!SoundDetectShort())
    {
      timeNow = millis();

      if (MechanicTimeout(&timeNow, &timeStart))
      {
        Serial.printf("SELENOID %c OFF\n", chars);
        break;
      }
    }
    break;
  default:
    break;
  }
}

// void MechanicSelect(char chars)
// {
//   char buff[2];
//   unsigned long timeNow;
//   unsigned long timeStart;
//   switch (chars)
//   {
//   case '0':
//     MechanicTyping(0);
//     break;
//   case '1':
//     MechanicTyping(1);
//     break;
//   case '2':
//     MechanicTyping(2);
//     break;
//   case '3':
//     MechanicTyping(3);
//     break;
//   case '4':
//     MechanicTyping(4);
//     break;
//   case '5':
//     MechanicTyping(5);
//     break;
//   case '6':
//     MechanicTyping(6);
//     break;
//   case '7':
//     MechanicTyping(7);
//     break;
//   case '8':
//     MechanicTyping(8);
//     break;
//   case '9':
//     MechanicTyping(9);
//     break;
//   default:
//     break;
//   }
// }

void MechanicEnter()
{
  MechanicTyping(11);
}

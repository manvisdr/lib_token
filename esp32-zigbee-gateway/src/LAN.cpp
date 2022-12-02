#include "zigbee_gateway.h"

IPAddress IPDummy(192, 168, 1, 1);
static byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
boolean checkDHCP = true;

void sys_LanInit()
{
  if (checkDHCP)
  {
    if (Ethernet.begin(mac) == 0)
    {
      if (Ethernet.hardwareStatus() == EthernetNoHardware)
      {
        while (true)
        {
          delay(1); // do nothing, no point running without Ethernet hardware
        }
      }
      else if (Ethernet.linkStatus() == LinkOFF)
      {
        while (true)
        {
          delay(1);
        }
      }
    }
    // server.print(F("      DHCP IP : "));
    // server.println(Ethernet.localIP());
  }
  else
  {
    Ethernet.begin(mac, IPDummy, IPDummy, IPDummy, IPDummy);
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      while (true)
      {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
      while (true)
      {
        delay(1);
      }
    }
  }
}

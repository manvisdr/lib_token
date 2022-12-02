#include <montok.h>

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);

  mcp.pinMode(0, INPUT);
}
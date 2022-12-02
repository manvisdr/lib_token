#include "main.h"

void Draw_Slow_Bitmap_Resize(int x, int y, uint8_t *bitmap, int w1, int h1, int w2, int h2)
{
  uint8_t color = Disp.getDrawColor();
  // Serial.print("颜色");
  // Serial.println(color);
  float mw = (float)w2 / w1;
  float mh = (float)h2 / h1;
  uint8_t cmw = ceil(mw);
  uint8_t cmh = ceil(mh);
  int xi, yi, byteWidth = (w1 + 7) / 8;
  for (yi = 0; yi < h1; yi++)
  {
    for (xi = 0; xi < w1; xi++)
    {
      if (pgm_read_byte(bitmap + yi * byteWidth + xi / 8) & (1 << (7 - (xi & 7))))
      {
        Disp.drawBox(x + xi * mw, y + yi * mh, cmw, cmh);
      }
      else if (color != 2)
      {
        Disp.setDrawColor(0);
        Disp.drawBox(x + xi * mw, y + yi * mh, cmw, cmh);
        Disp.setDrawColor(color);
      }
    }
  }
}

void Draw_Utf(int x, int y, const char *s)
{
  // Disp.setCursor(x,y + 1);
  // Disp.print(s);
  Disp.drawUTF8(x, y + 1, s);
}

/*
@ 作用：抖动1
*/
void Blur(int sx, int sy, int ex, int ey, int f, int delayMs)
{
  for (int i = 0; i < f; i++)
  {
    for (int y = 0; y < ey; y++)
    {
      for (int x = 0; x < ex; x++)
      {
        if (x % 2 == y % 2 && x % 2 == 0 && x >= sx && x <= ex && y >= sy && y <= ey)
          Disp.drawPixel(x + (i > 0 && i < 3), y + (i > 1));
        // else Disp.drawPixel(x + (i > 0 && i < 3), y + (i > 1), 0);
      }
    }
    if (delayMs)
    {
      Disp.sendBuffer();
      delay(delayMs);
    }
  }
}
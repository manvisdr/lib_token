#include "main.h"

// const char *txtMENU[] = {
//     "DEVICE",
//     "SERIAL",
//     "ETHERNET",
//     "SERVER",
//     "END DEVICE",
//     "RF",
// };

// const char *dscMENU[] = {
//     "GATEWAY SETTING",
//     "SERIAL COMM",
//     "LAN COMM",
//     "FRONTEND SERVER",
//     "MODIFYING NODE DEVICE",
//     "MODIFYING RADIO",

// };

void Page_Footnotes(int a, int b)
{
  char buffer[20];
  uint8_t w = (Get_Dec_Deep(a) + Get_Dec_Deep(b) + 3) * 6;
  uint8_t x = SCREEN_COLUMN - 8 - w;
  uint8_t y = SCREEN_ROW - 12;

  // if (millis() < pages_Tip_Display_timer + pages_Tip_Display_Timeout)
  // {
  //绘制白色底色块
  Disp.setDrawColor(1);
  Disp.drawRBox(x + 1, y - 1, w, 13, 1);
  //绘制下标文字
  Disp.setDrawColor(0);
  Disp.setCursor(x + 1, y + 9);
  sprintf(buffer, "[%d/%d]", a, b);
  Disp.print(buffer);
  // }
  //恢复颜色设置
  Disp.setDrawColor(1);
}

void Pop_Windows(const char *s)
{
  // Disp.setCursor(0, 0);
  // Disp.print(s);
  // Display();
  // Set_Font_Size(2);
  int w = Get_UTF8_Ascii_Pix_Len(1, s) + 2;
  int h = 12;
  // for (int i = 5;i > 0;i--) {
  //     //Set_Font_Size(i);
  //     w = CNSize * Get_Max_Line_Len(s) * Get_Font_Size() / 2;
  //     //h = CNSize * Get_Font_Size() * Get_Str_Next_Line_O(s);
  //     if (w < SCREEN_COLUMN && h < SCREEN_ROW) break;
  // }
  int x = (SCREEN_COLUMN - w) / 2;
  int y = (SCREEN_ROW - h) / 2;

  Disp.setDrawColor(0);
  Blur(0, 0, SCREEN_COLUMN, SCREEN_ROW, 3, 66 * 100); //<=15FPS以便人眼察觉细节变化

  int ix = 0;
  for (int i = 1; i <= 10; i++)
  {
    //震荡动画
    // if (*Switch_space[SwitchSpace_SmoothAnimation])
    ix = (10 * cos((i * 3.14) / 2.0)) / i;

    Disp.setDrawColor(0);
    Blur(0, 0, SCREEN_COLUMN, SCREEN_ROW, 3, 0);
    Disp.drawFrame(x - 1 + ix, y - 3, w + 1, h + 3);
    Disp.setDrawColor(1);
    Disp.drawRBox(x + ix, y - 2, w, h + 2, 2);
    Disp.setDrawColor(0);
    Draw_Utf(x + 1 + ix, y + 9, s);
    Disp.setDrawColor(1);
    // Display();
    delay(20 * 100);
  }
  // Set_Font_Size(1);
}
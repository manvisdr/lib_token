#include <Arduino.h>
#include <GEM_u8g2.h>
#include "type.h"
#include "ExternFunc.h"
#include "ExternDisp.h"
#include "Menu.h"
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C Disp;

#define SCREEN_COLUMN 128
#define SCREEN_ROW 64
#define SCREEN_PAGE_NUM 8
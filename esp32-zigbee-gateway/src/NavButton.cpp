#include "zigbee_gateway.h"

static double Count = 0;
static double CountLast = 0;
static double Count_min = 0;
static double Count_max = 0;
static double Count_step = 0;
static uint8_t CounterChanged = 0;

void sys_NavInit(void)
{
  //初始化GPIO
  pinMode(PIN_NAV_DWN, INPUT_PULLUP);
  pinMode(PIN_NAV_LFT, INPUT_PULLUP);
  pinMode(PIN_NAV_MID, INPUT_PULLUP);
  pinMode(PIN_NAV_RHT, INPUT_PULLUP);
  pinMode(PIN_NAV_UP, INPUT_PULLUP);

  //初始化按键事件检测
  MIDButton.attachClick(sys_Counter_click);
  MIDButton.attachDoubleClick(sys_Counter_doubleclick);
  MIDButton.attachLongPressStart(sys_Counter_longclick);
  MIDButton.setDebounceTicks(25);
  MIDButton.setClickTicks(30);
  MIDButton.setPressTicks(300);
}

/**
 * @name  sys_Counter_Set
 * @brief 设置计数器参数
 * @param {double} c       计数器初始值
 * @param {double} min     计数器最小值
 * @param {double} max     计数器最大值
 * @param {double} step    计数器增量步进
 * @return {*}
 */

//按键FIFO循环大小
#define MIDButton_FIFO_Size 10
//按键FIFO读写指针
static uint8_t MIDButton_FIFO_pwrite = 0;
static uint8_t MIDButton_FIFO_pread = 0;
//按键FIFO缓存区
static uint8_t MIDButton_FIFO[MIDButton_FIFO_Size];
//按键FIFO缓存区有效数据大小
static uint8_t MIDButton_FIFO_BufferSize = 0;

void Clear_MIDButton_FIFO(void)
{
  MIDButton_FIFO_pread = MIDButton_FIFO_pwrite;
  memset(MIDButton_FIFO, 0, MIDButton_FIFO_Size);
}
/***
 * @description: 写按键FIFO
 * @param uint8_t State 要写入FIFO的按键状态值
 * @return 无
 */
//
static void Write_MIDButton_FIFO(uint8_t State)
{
  //重置事件计时器
  // TimerUpdateEvent();

  MIDButton_FIFO[MIDButton_FIFO_pwrite] = State;
  printf("FIFO写入[%d]=%d\n", MIDButton_FIFO_pwrite, State);
  //写指针移位
  MIDButton_FIFO_pwrite++;
  //按键缓冲区数据大小+1
  if (MIDButton_FIFO_BufferSize < MIDButton_FIFO_Size)
    MIDButton_FIFO_BufferSize++;
  //循环写
  if (MIDButton_FIFO_pwrite >= MIDButton_FIFO_Size)
    MIDButton_FIFO_pwrite = 0;
  printf("FIFO缓存区大小:%d\n\n", MIDButton_FIFO_BufferSize);
}
/***
 * @description: 读按键FIFO
 * @param {*}
 * @return 按键State
 */
//读按键FIFO
static uint8_t Read_MIDButton_FIFO(void)
{
  //判断当前按键缓冲区是否有数据
  if (MIDButton_FIFO_BufferSize == 0)
    return 0;

  //从按键FIFO缓存读取数据
  uint8_t res = MIDButton_FIFO[MIDButton_FIFO_pread];
  printf("FIFO读取[%d]=%d\n", MIDButton_FIFO_pread, res);
  //读指针移位
  MIDButton_FIFO_pread++;
  MIDButton_FIFO_BufferSize--;
  //循环写
  if (MIDButton_FIFO_pread >= MIDButton_FIFO_Size)
    MIDButton_FIFO_pread = 0;
  printf("FIFO缓存区大小:%d\n\n", MIDButton_FIFO_BufferSize);
  return res;
}
/***
 * @description: 按键单击回调函数
 * @param {*}
 * @return {*}
 */
void sys_Counter_click(void)
{
  printf("触发单击事件\n");
  Write_MIDButton_FIFO(1);
}
/***
 * @description: 按键长按回调函数
 * @param {*}
 * @return {*}
 */
void sys_Counter_longclick(void)
{
  printf("触发长按事件\n");
  Write_MIDButton_FIFO(2);
}
/***
 * @description: 按键双击回调函数
 * @param {*}
 * @return {*}
 */
void sys_Counter_doubleclick(void)
{
  printf("触发双击事件\n");
  Write_MIDButton_FIFO(3);
}

//编码器按键按下 + 软件滤波
uint8_t SYSKey = 0;
uint8_t sys_KeyProcess(void)
{
  MIDButton.tick();
  SYSKey = Read_MIDButton_FIFO();
  return SYSKey;
}
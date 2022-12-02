#ifndef NavButton_H
#define NavButton_H

void sys_NavInit(void);
void Clear_MIDButton_FIFO(void);
static void Write_MIDButton_FIFO(uint8_t State);
static uint8_t Read_MIDButton_FIFO(void);
void sys_Counter_click(void);
void sys_Counter_longclick(void);
void sys_Counter_doubleclick(void);
uint8_t sys_KeyProcess(void);

#endif
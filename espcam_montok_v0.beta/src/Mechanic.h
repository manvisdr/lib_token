#include "Type.h"

#define SOL_0 0
#define SOL_1 1
#define SOL_2 2
#define SOL_3 3
#define SOL_3 3
#define SOL_4 4
#define SOL_5 5
#define SOL_6 6
#define SOL_7 7
#define SOL_8 8
#define SOL_9 9
#define SOL_BACK 10
#define SOL_ENTER 11

bool MechanicTimeout(unsigned long *timeNow, unsigned long *timeStart);

void MechanicInit();

bool MechanicTyping(int pin);
bool MechanicTyping(char *pin);

void MechanicSelect(char chars);

void MechanicSelectNoSound(char chars);

void MechanicEnter();
#ifndef GUARD_TIME_H
#define GUARD_TIME_H

#include "global.h"

void InGameTimeCounter_Reset(void);
void InGameTimeCounter_Start(void);
void InGameTimeCounter_Stop(void);
void InGameTimeCounter_Set(u8 dayOfWeek, u8 hour, u8 minute);
void InGameTimeCounter_Update(void);

#endif // GUARD_TIME_H

#include "time.h"

static bool8 sInGameTimeCounterActive;

void InGameTimeCounter_Reset(void)
{
    sInGameTimeCounterActive = FALSE;
    gSaveBlock2Ptr->inGameTime.dayOfWeek = 0;
    gSaveBlock2Ptr->inGameTime.hours = 0;
    gSaveBlock2Ptr->inGameTime.minutes = 0;
    gSaveBlock2Ptr->inGameTime.seconds = 0;
    gSaveBlock2Ptr->inGameTime.vblanks = 0;
}

void InGameTimeCounter_Start(void)
{
    sInGameTimeCounterActive = TRUE;
}

void InGameTimeCounter_Stop(void)
{
    sInGameTimeCounterActive = FALSE;
}

void InGameTimeCounter_Set(u8 dayOfWeek, u8 hour, u8 minute)
{
    gSaveBlock2Ptr->inGameTime.dayOfWeek = dayOfWeek;
    gSaveBlock2Ptr->inGameTime.hours = hour;
    gSaveBlock2Ptr->inGameTime.minutes = minute;
    gSaveBlock2Ptr->inGameTime.seconds = 0;
    gSaveBlock2Ptr->inGameTime.vblanks = 0;
}

void InGameTimeCounter_Update(void)
{
    if (sInGameTimeCounterActive == TRUE)
    {
        gSaveBlock2Ptr->inGameTime.vblanks++;

        // 20 vblanks for a 8 hour cycle
        if (gSaveBlock2Ptr->inGameTime.vblanks < 10)
            return;

        gSaveBlock2Ptr->inGameTime.vblanks = 0;
        gSaveBlock2Ptr->inGameTime.seconds++;

        if (gSaveBlock2Ptr->inGameTime.seconds < 60)
            return;

        gSaveBlock2Ptr->inGameTime.seconds = 0;
        gSaveBlock2Ptr->inGameTime.minutes++;

        if (gSaveBlock2Ptr->inGameTime.minutes < 60)
            return;

        gSaveBlock2Ptr->inGameTime.minutes = 0;
        gSaveBlock2Ptr->inGameTime.hours++;

        if (gSaveBlock2Ptr->inGameTime.hours < 24)
            return;

        gSaveBlock2Ptr->inGameTime.hours = 0;
        gSaveBlock2Ptr->inGameTime.dayOfWeek++;

        if (gSaveBlock2Ptr->inGameTime.dayOfWeek < 7)
            return;

        gSaveBlock2Ptr->inGameTime.dayOfWeek = 0;
    }
}

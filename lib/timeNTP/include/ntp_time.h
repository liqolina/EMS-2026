#pragma once
#include <Arduino.h>
#include <time.h>

namespace NTPTime
{
    void begin(const char* tz, const char* ntp1, const char* ntp2);

    bool isReady();

    void sync();   // force resync

    bool getDateTime(struct tm &outTime);

    time_t now();  // epoch realtime

    String getDateTimeString();
    String getDateString();
    String getTimeString();
}
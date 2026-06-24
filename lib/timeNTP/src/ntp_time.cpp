#include "ntp_time.h"

static const char* g_tz = nullptr;
static const char* g_ntp1 = nullptr;
static const char* g_ntp2 = nullptr;

static bool ready = false;
static time_t lastEpoch = 0;

namespace NTPTime
{
    void begin(const char* tz, const char* ntp1, const char* ntp2)
    {
        g_tz = tz;
        g_ntp1 = ntp1;
        g_ntp2 = ntp2;

        configTzTime(g_tz, g_ntp1, g_ntp2);

        sync();
    }

    void sync()
    {
        struct tm timeinfo;
        int retry = 0;

        while (!getLocalTime(&timeinfo) && retry < 10)
        {
            delay(500);
            retry++;
        }

        if (retry < 10)
        {
            ready = true;
            lastEpoch = time(nullptr);
        }
    }

    bool isReady()
    {
        return ready;
    }

    time_t now()
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
            return lastEpoch;

        return time(nullptr);
    }

    bool getDateTime(struct tm &outTime)
    {
        return getLocalTime(&outTime);
    }

    String getDateTimeString()
    {
        struct tm t;
        if (!getLocalTime(&t)) return "NO_TIME";

        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
        return String(buf);
    }

    String getDateString()
    {
        struct tm t;
        if (!getLocalTime(&t)) return "NO_DATE";

        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
        return String(buf);
    }

    String getTimeString()
    {
        struct tm t;
        if (!getLocalTime(&t)) return "NO_TIME";

        char buf[16];
        strftime(buf, sizeof(buf), "%H:%M:%S", &t);
        return String(buf);
    }
}
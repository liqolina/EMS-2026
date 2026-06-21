#ifndef MUTEX_GLOBAL_H
#define MUTEX_GLOBAL_H

#include <Arduino.h>
#include <atomic>
#include <mutex>

extern std::mutex wifi_mutex;
extern std::mutex info_mutex;
extern std::mutex sensor_mutex;
extern std::mutex sensor_calib_mutex;
extern std::mutex act_mutex;
extern std::mutex act_calib_mutex;
extern std::mutex statusnews_mutex;

extern std::atomic<bool> wifiSTA_running;
extern std::atomic<bool> wifiAP_running;
extern std::atomic<bool> wifiStatus_running;

#endif
#ifndef MUTEX_GLOBAL_H
#define MUTEX_GLOBAL_H

#include <Arduino.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>

std::mutex wifi_mutex;
std::mutex info_mutex;
std::mutex sensor_mutex;
std::mutex sensor_calib_mutex;
std::mutex act_mutex;
std::mutex act_calib_mutex;
std::mutex statusnews_mutex;

std::atomic<bool> wifiSTA_running{false}; 
std::atomic<bool> wifiAP_running{false}; 
std::atomic<bool> wifiStatus_running{false}; 

#endif
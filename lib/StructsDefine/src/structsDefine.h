#ifndef STRUCTS_DEFINE_H
#define STRUCTS_DEFINE_H

#include <Arduino.h>

constexpr size_t DEVICE_NAME_SIZE = 36;
constexpr size_t LOCATION_SIZE    = 64;
constexpr size_t IP_ADDRESS_SIZE  = 16;
constexpr size_t MAC_ADDRESS_SIZE = 18;
constexpr size_t STATUS_SIZE      = 40;
constexpr size_t NEWS_SIZE        = 170;

struct DeviceInfo {
    char deviceId;

    char deviceName[DEVICE_NAME_SIZE];
    char location[LOCATION_SIZE];
    char ipAddress[IP_ADDRESS_SIZE];
    char macAddress[MAC_ADDRESS_SIZE];

    uint32_t timestamps;
};

struct DataSensor {
    uint64_t deviceId;
    uint64_t moduleId;

    float value_1;
    float value_2;
    float value_3;
    float value_4;
    float value_5;
    float value_6;
    float value_7;
};

struct CalibrateSensor {
    uint64_t deviceId;
    uint64_t moduleId;

    float calib_1;
    float calib_2;
    float calib_3;
    float calib_4;
    float calib_5;
    float calib_6;
    float calib_7;
};

struct CommandAct {
    uint64_t deviceId;
    uint64_t moduleId;

    bool command;
};

struct CalibrateAct {
    uint64_t deviceId;
    uint64_t moduleId;

    float calib_1;
    float calib_2;
};

struct StatusNews{
    uint64_t deviceId;
    uint64_t moduleId;
    
    char status[STATUS_SIZE];
    char news[NEWS_SIZE];
};

#endif
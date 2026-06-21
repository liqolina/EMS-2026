#ifndef VARIABLE_GLOBAL_H
#define VARIABLE_GLOBAL_H

#include <Arduino.h>

constexpr size_t ID_SIZE        = 36;
constexpr size_t NAME_ID_SIZE   = 36;
constexpr size_t LOC_SIZE       = 64;
constexpr size_t IP_SIZE        = 16;
constexpr size_t MAC_SIZE       = 18;

constexpr size_t ID_SENSOR_SIZE = 36;
constexpr size_t ID_ACT_SIZE    = 36;

constexpr size_t STATUS_SIZE    = 40;
constexpr size_t NEWS_SIZE      = 170;

struct __attribute__((packed)) InfoESP {
    char id_esp[ID_SIZE];

    char name_esp[NAME_ID_SIZE];
    char loc_esp[LOC_SIZE];
};

struct __attribute__((packed)) InfoWifiESP {
    char ip_sta[IP_SIZE];
    char mac_sta[MAC_SIZE];
    char ip_ap[IP_SIZE];
    char mac_ap[MAC_SIZE];
};

struct __attribute__((packed)) ValueSensor {
    char id_sensor[ID_SENSOR_SIZE];

    float val_sensor_1;
    float val_sensor_2;
    float val_sensor_3;
    float val_sensor_4;
    float val_sensor_5;
    float val_sensor_6;
    float val_sensor_7;
    float val_sensor_8;
};

struct __attribute__((packed)) CalibSensor {
    char id_sensor[ID_SENSOR_SIZE];

    float calib_sensor_1;
    float calib_sensor_2;
    float calib_sensor_3;
    float calib_sensor_4;
    float calib_sensor_5;
    float calib_sensor_6;
    float calib_sensor_7;
    float calib_sensor_8;
};

struct __attribute__((packed)) CmdAct {
    char id_act[ID_ACT_SIZE];

    bool val_cmd_1;
    bool val_cmd_2;
    bool val_cmd_3;
    bool val_cmd_4;
    bool val_cmd_5;
    bool val_cmd_6;
    bool val_cmd_7;
    bool val_cmd_8;
};

struct __attribute__((packed)) CalibAct {
    char id_act[ID_ACT_SIZE];

    float calib_cmd_1;
    float calib_cmd_2;
    float calib_cmd_3;
    float calib_cmd_4;
    float calib_cmd_5;
    float calib_cmd_6;
    float calib_cmd_7;
    float calib_cmd_8;
};

struct __attribute__((packed)) StatusNews{
    char id[ID_SIZE];
    
    char status[STATUS_SIZE];
    char news[NEWS_SIZE];

    uint32_t timestamps;
};

// extern InfoESP g_InfoESP;
// extern InfoWifiESP g_InfoWifiESP;
// extern ValueSensor g_ValueSensor;
// extern CalibSensor g_CalibSensor;
// extern CmdAct g_CmdAct;
// extern CalibAct g_CalibAct;
// extern StatusNews g_StatusNews;

#endif
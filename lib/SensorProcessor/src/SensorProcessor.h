#ifndef SENSOR_PROCESSOR_H
#define SENSOR_PROCESSOR_H

#include <Arduino.h>

#define MAX_SENSORS 10
#define MAX_SAMPLES 20 

// PascalCase untuk Struct
struct SensorBuffer {
    char sensorId[16];
    float sampleValues[MAX_SAMPLES];
    float filteredValue = 0.0f;
    float calibratedValue = 0.0f;
    uint8_t currentSampleSize = 0;
};

// PascalCase untuk Class
class SensorProcessor {
private:
    SensorBuffer _sensors[MAX_SENSORS];
    uint8_t _sensorCount = 0;

    int findSensorIndex(const char* sensorId);
    float calculateAverage(const float* data, uint8_t size);
    float calculateMedian(float* data, uint8_t size);

public:
    SensorProcessor();
    
    bool addSample(const char* sensorId, uint8_t requiredSamples, float value);
    bool processData(const char* sensorId, uint8_t requiredSamples, bool useMedian);
    void calibrate(const char* sensorId, float gain, float offset);
    float getOutput(const char* sensorId);
};

#endif
#include "SensorProcessor.h"
#include <algorithm>
#include <string.h>

SensorProcessor::SensorProcessor() : _sensorCount(0) {}

int SensorProcessor::findSensorIndex(const char* sensorId) {
    for (uint8_t i = 0; i < _sensorCount; i++) {
        if (strcmp(_sensors[i].sensorId, sensorId) == 0) return i;
    }
    
    if (_sensorCount < MAX_SENSORS) {
        strlcpy(_sensors[_sensorCount].sensorId, sensorId, sizeof(_sensors[_sensorCount].sensorId));
        return _sensorCount++;
    }
    return -1; 
}

bool SensorProcessor::addSample(const char* sensorId, uint8_t requiredSamples, float value) {
    int idx = findSensorIndex(sensorId);
    if (idx == -1) return false;

    SensorBuffer &s = _sensors[idx];
    if (s.currentSampleSize <= requiredSamples) {
        s.sampleValues[s.currentSampleSize] = value;
        s.currentSampleSize++;
        return false;
    }
    return true;
}

bool SensorProcessor::processData(const char* sensorId, uint8_t requiredSamples, bool useMedian) {
    int idx = findSensorIndex(sensorId);
    if (idx == -1) return false;

    SensorBuffer &s = _sensors[idx];

    if ((s.currentSampleSize >= requiredSamples) && (requiredSamples > 0)) {
        if (useMedian) {
            s.filteredValue = calculateMedian(s.sampleValues, s.currentSampleSize);
        } else {
            s.filteredValue = calculateAverage(s.sampleValues, s.currentSampleSize);
        }
        s.currentSampleSize = 0;
        return false;
    }
    return true;
}

void SensorProcessor::calibrate(const char* sensorId, float gain, float offset) {
    int idx = findSensorIndex(sensorId);
    if (idx != -1) {
        SensorBuffer &s = _sensors[idx];
        float safeGain = (gain < 0) ? 1.0f : gain;
        s.calibratedValue = (s.filteredValue * safeGain) + offset;
    }
}

float SensorProcessor::getOutput(const char* sensorId) {
    int idx = findSensorIndex(sensorId);
    return (idx != -1) ? _sensors[idx].calibratedValue : 0.0f;
}

float SensorProcessor::calculateAverage(const float* data, uint8_t size) {
    if (size == 0) return 0.0f;
    float total = 0.0f;
    for (uint8_t i = 0; i < size; i++) total += data[i];
    return total / (float)size;
}

float SensorProcessor::calculateMedian(float* data, uint8_t size) {
    if (size == 0) return 0.0f;
    
    std::sort(data, data + size);

    if (size % 2 == 0)
        return (data[size / 2 - 1] + data[size / 2]) / 2.0f;
    else
        return data[size / 2];
}
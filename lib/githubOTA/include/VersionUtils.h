#ifndef VERSION_UTILS_H
#define VERSION_UTILS_H

#include <Arduino.h>

namespace VersionUtils {
String normalize(const String &version);
int compare(const String &currentVersion, const String &latestVersion);
}

#endif
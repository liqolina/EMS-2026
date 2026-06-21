#include "VersionUtils.h"

namespace {
long popVersionPart(String &version) {
  if (version.isEmpty()) {
    return 0;
  }

  const int dotPos = version.indexOf('.');
  String token = (dotPos >= 0) ? version.substring(0, dotPos) : version;
  token.trim();

  const long value = token.toInt();

  if (dotPos >= 0) {
    version.remove(0, dotPos + 1);
  } else {
    version = "";
  }

  return value;
}
}  // namespace

namespace VersionUtils {

String normalize(const String &rawVersion) {
  String version = rawVersion;
  version.trim();

  if (version.startsWith("v") || version.startsWith("V")) {
    version.remove(0, 1);
  }

  const int dashPos = version.indexOf('-');
  if (dashPos >= 0) {
    version = version.substring(0, dashPos);
  }

  const int plusPos = version.indexOf('+');
  if (plusPos >= 0) {
    version = version.substring(0, plusPos);
  }

  return version;
}

int compare(const String &currentVersion, const String &latestVersion) {
  String current = normalize(currentVersion);
  String latest = normalize(latestVersion);

  for (int i = 0; i < 8; ++i) {
    const long a = popVersionPart(current);
    const long b = popVersionPart(latest);

    if (a < b) {
      return -1;
    }

    if (a > b) {
      return 1;
    }

    if (current.isEmpty() && latest.isEmpty()) {
      break;
    }
  }

  return 0;
}

}  // namespace VersionUtils

#include "GitHubOtaUpdater.h"
#include "VersionUtils.h"

#include <new>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <Network.h>
#include <NetworkClientSecure.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "esp_crt_bundle.h"

constexpr const char* TAG = "TASK_0A";

GitHubOtaUpdater::GitHubOtaUpdater(const GitHubOtaConfig &config)
    : _config(config), _lastCheckMs(0) {}

void GitHubOtaUpdater::begin() {
  Serial.println();
  Serial.println(F("=== ESP32 GitHub Release OTA ==="));

#ifndef FW_VERSION
  Serial.println(F("[APP] FW_VERSION not defined"));
#else
  Serial.printf("[APP] Current FW_VERSION: %s\n", FW_VERSION);
#endif

  confirmPendingFirmwareIfNeeded();
  _lastCheckMs = millis();

  if (_config.checkOnBoot) {
    checkNow();
  }
}

void GitHubOtaUpdater::loop() {
  if (millis() - _lastCheckMs >= _config.checkIntervalMs) {
    _lastCheckMs = millis();
    checkNow();
  }

  if (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == 'u' || c == 'U') {
      Serial.println(F("[OTA] Manual update check requested"));
      checkNow();
    }
  }
}

bool GitHubOtaUpdater::checkNow() {
  if (!isNetworkReady()) {
    Serial.println(F("[OTA] Network is not ready; OTA check skipped"));
    return false;
  }

  ReleaseInfo release{};
  if (!fetchLatestRelease(release)) {
    Serial.println(F("[OTA] Failed to fetch latest release"));
    return false;
  }

  Serial.printf("[OTA] Latest tag   : %s\n", release.tagName.c_str());
  Serial.printf("[OTA] Published at : %s\n", release.publishedAt.c_str());
  Serial.printf("[OTA] Asset        : %s\n", release.assetName.c_str());
  Serial.printf("[OTA] Size         : %u bytes\n", static_cast<unsigned>(release.sizeBytes));

#ifdef FW_VERSION
  const String currentVersion = VersionUtils::normalize(FW_VERSION);
#else
  const String currentVersion = "0.0.0";
#endif
  const String latestVersion = VersionUtils::normalize(release.tagName);

  if (VersionUtils::compare(currentVersion, latestVersion) >= 0) {
    Serial.println(F("[OTA] No update available"));
    return true;
  }

  Serial.printf("[OTA] Update available: %s -> %s\n",
                currentVersion.c_str(),
                latestVersion.c_str());

  if (!downloadAndInstall(release)) {
    Serial.println(F("[OTA] OTA installation failed"));
    _enableInsecure = true;
    return false;
  }

  Serial.println(F("[OTA] OTA installation completed"));
  if (_config.autoRebootAfterUpdate) {
    Serial.println(F("[OTA] Rebooting in 2 seconds..."));
    delay(2000);
    ESP.restart();
  }

  return true;
}

bool GitHubOtaUpdater::isNetworkReady() const {
  return _config.networkReadyCheck != nullptr ? _config.networkReadyCheck() : WiFi.status() == WL_CONNECTED;
}

void GitHubOtaUpdater::confirmPendingFirmwareIfNeeded() {
  if (!_config.enableRollbackConfirm) return;

  const esp_partition_t *runningPartition = esp_ota_get_running_partition();
  if (runningPartition == nullptr) return;

  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (esp_ota_get_state_partition(runningPartition, &state) != ESP_OK) return;
  if (state != ESP_OTA_IMG_PENDING_VERIFY) return;

  Serial.println(F("[OTA] Firmware is in PENDING_VERIFY state"));
  if (runSelfTest()) {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      Serial.println(F("[OTA] Firmware marked VALID"));
    } else {
      Serial.println(F("[OTA] Failed to mark firmware VALID"));
    }
  } else {
    Serial.println(F("[OTA] Self-test failed, rolling back"));
    esp_ota_mark_app_invalid_rollback_and_reboot();
  }
}

bool GitHubOtaUpdater::runSelfTest() const {
  return _config.selfTestCheck != nullptr ? _config.selfTestCheck() : true;
}

bool GitHubOtaUpdater::fetchLatestRelease(ReleaseInfo &release) {
  release = ReleaseInfo{};
  release.sizeBytes = 0;
  release.found = false;

  const String url = String("https://api.github.com/repos/") + _config.githubOwner + "/" + _config.githubRepo + "/releases/latest";

  NetworkClientSecure client;
  HTTPClient http;
  if (!beginHttp(http, client, url, true, false)) return false;

  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] latest release GET failed, status=%d\n", statusCode);
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(_config.jsonDocCapacity);
  const DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("[JSON] parse failed: %s\n", error.c_str());
    return false;
  }

  release.tagName = String(doc["tag_name"] | "");
  release.publishedAt = String(doc["published_at"] | "");

  JsonArray assets = doc["assets"].as<JsonArray>();
  if (assets.isNull() || assets.size() == 0) {
    Serial.println(F("[OTA] Release exists, but no assets were found"));
    return false;
  }

  for (JsonObject asset : assets) {
    const String name = String(asset["name"] | "");
    const String state = String(asset["state"] | "");
    if (state != "uploaded" || !isMatchingAsset(name)) continue;

    release.assetName = name;
    release.assetApiUrl = String(asset["url"] | "");
    release.browserDownloadUrl = String(asset["browser_download_url"] | "");
    release.sizeBytes = static_cast<size_t>(asset["size"] | 0);

    String digest = String(asset["digest"] | "");
    digest.trim();
    if (digest.startsWith("sha256:")) digest.remove(0, 7);
    digest.toLowerCase();
    release.sha256 = digest;
    release.found = true;
    break;
  }

  if (!release.found) {
    Serial.printf("[OTA] Asset '%s' was not found in the latest release\n", _config.firmwareAssetName);
    return false;
  }

  return true;
}

bool GitHubOtaUpdater::downloadAndInstall(const ReleaseInfo &release) {
  String currentUrl = release.assetApiUrl.length() > 0 ? release.assetApiUrl : release.browserDownloadUrl;

  for (uint8_t redirectCount = 0; redirectCount <= _config.maxRedirects; ++redirectCount) {
    NetworkClientSecure client;
    HTTPClient http;
    const char *headerKeys[] = {"Location"};

    if (!beginHttp(http, client, currentUrl, false, true)) return false;

    http.collectHeaders(headerKeys, 1);
    Serial.printf("[OTA] Download URL: %s\n", currentUrl.c_str());
    const int statusCode = http.GET();

    if (statusCode == HTTP_CODE_OK) {
      const bool success = installFromHttp(http, release);
      http.end();
      return success;
    }

    if (statusCode == HTTP_CODE_MOVED_PERMANENTLY || statusCode == HTTP_CODE_FOUND || statusCode == HTTP_CODE_SEE_OTHER || statusCode == 307 || statusCode == 308) {
      const String location = http.header("Location");
      http.end();
      if (location.isEmpty()) {
        Serial.println(F("[HTTP] Redirect without a Location header"));
        return false;
      }
      Serial.printf("[HTTP] Redirect -> %s\n", location.c_str());
      currentUrl = location;
      continue;
    }

    Serial.printf("[HTTP] Firmware download failed, status=%d\n", statusCode);
    http.end();
    return false;
  }

  Serial.println(F("[HTTP] Too many redirects"));
  return false;
}

bool GitHubOtaUpdater::installFromHttp(HTTPClient &http, const ReleaseInfo &release) {
  size_t expectedSize = release.sizeBytes;
  const int responseLength = http.getSize();
  if (expectedSize == 0 && responseLength > 0) expectedSize = static_cast<size_t>(responseLength);

  if (!Update.begin(expectedSize > 0 ? expectedSize : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
    Serial.printf("[OTA] Update.begin failed: %s\n", Update.errorString());
    return false;
  }

  auto *stream = http.getStreamPtr();
  if (stream == nullptr || _config.downloadBufferSize == 0) {
    Serial.println(F("[OTA] Invalid stream or download buffer size"));
    Update.abort();
    return false;
  }

  uint8_t *buffer = new uint8_t[_config.downloadBufferSize];
  if (buffer == nullptr) {
    Serial.println(F("[OTA] Failed to allocate download buffer"));
    Update.abort();
    return false;
  }

  size_t written = 0;
  int lastPercent = -1;
  uint32_t lastDataMs = millis();
  mbedtls_sha256_context shaContext;
  mbedtls_sha256_init(&shaContext);

  if (mbedtls_sha256_starts(&shaContext, 0) != 0) {
    Serial.println(F("[SHA256] starts failed"));
    delete[] buffer;
    Update.abort();
    mbedtls_sha256_free(&shaContext);
    return false;
  }

  bool ok = true;
  while (http.connected() || stream->available()) {
    size_t availableBytes = stream->available();
    if (availableBytes == 0) {
      if ((millis() - lastDataMs) > _config.httpReadTimeoutMs) {
        Serial.println(F("[HTTP] Read timeout"));
        ok = false;
        break;
      }
      delay(1);
      continue;
    }

    if (availableBytes > _config.downloadBufferSize) availableBytes = _config.downloadBufferSize;

    const int readLen = stream->readBytes(reinterpret_cast<char *>(buffer), availableBytes);
    if (readLen <= 0) {
      delay(1);
      continue;
    }

    lastDataMs = millis();
    if (mbedtls_sha256_update(&shaContext, buffer, static_cast<size_t>(readLen)) != 0) {
      Serial.println(F("[SHA256] update failed"));
      ok = false;
      break;
    }

    const size_t justWritten = Update.write(buffer, static_cast<size_t>(readLen));
    if (justWritten != static_cast<size_t>(readLen)) {
      Serial.printf("[OTA] Update.write failed: %s\n", Update.errorString());
      ok = false;
      break;
    }

    written += justWritten;
    if (expectedSize > 0) {
      const int percent = static_cast<int>((written * 100U) / expectedSize);
      if (percent != lastPercent) {
        lastPercent = percent;
        Serial.printf("[OTA] Progress: %d%% (%u/%u)\n", percent, static_cast<unsigned>(written), static_cast<unsigned>(expectedSize));
      }
      if (written >= expectedSize) break;
    }
  }

  uint8_t digestRaw[32] = {0};
  if (ok && mbedtls_sha256_finish(&shaContext, digestRaw) != 0) {
    Serial.println(F("[SHA256] finish failed"));
    ok = false;
  }
  mbedtls_sha256_free(&shaContext);
  delete[] buffer;

  if (!ok) {
    Update.abort();
    return false;
  }

  if (expectedSize > 0 && written != expectedSize) {
    Serial.printf("[OTA] Incomplete firmware image: %u / %u bytes\n", static_cast<unsigned>(written), static_cast<unsigned>(expectedSize));
    Update.abort();
    return false;
  }

  const String actualSha256 = bytesToHex(digestRaw, sizeof(digestRaw));
  if (release.sha256.length() > 0 && !actualSha256.equalsIgnoreCase(release.sha256)) {
    Serial.println(F("[OTA] SHA-256 mismatch"));
    Serial.printf("[OTA] Expected: %s\n", release.sha256.c_str());
    Serial.printf("[OTA] Actual  : %s\n", actualSha256.c_str());
    Update.abort();
    return false;
  }

  if (release.sha256.length() > 0) {
    Serial.println(F("[OTA] SHA-256 verified"));
  } else {
    Serial.println(F("[OTA] Warning: release metadata did not include a SHA-256 digest"));
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Update.end failed: %s\n", Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println(F("[OTA] Update did not finish cleanly"));
    return false;
  }

  Serial.println(F("[OTA] Firmware successfully written"));
  return true;
}

bool GitHubOtaUpdater::beginHttp(HTTPClient &http,
                                 NetworkClientSecure &client,
                                 const String &url,
                                 bool acceptJson,
                                 bool acceptOctetStream) {
  if (!configureSecureClient(client, url)) return false;

  http.setConnectTimeout(_config.httpConnectTimeoutMs);
  http.setTimeout(_config.httpReadTimeoutMs);
  http.useHTTP10(true);

  if (!http.begin(client, url)) {
    Serial.printf("[HTTP] begin() failed: %s\n", url.c_str());
    return false;
  }

  http.setUserAgent("ESP32-GitHub-Release-OTA/1.0");
  if (acceptJson) {
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("X-GitHub-Api-Version", _config.githubApiVersion);
  }
  if (acceptOctetStream) {
    http.addHeader("Accept", "application/octet-stream");
    http.addHeader("X-GitHub-Api-Version", _config.githubApiVersion);
  }
  if (_config.githubToken != nullptr && strlen(_config.githubToken) > 0) {
    http.addHeader("Authorization", String("Bearer ") + _config.githubToken);
  }
  return true;
}

bool GitHubOtaUpdater::configureSecureClient(NetworkClientSecure &client, const String &url) {
  client.setTimeout(_config.httpReadTimeoutMs / 1000);

  if (_config.strictTls != nullptr && strcmp(_config.strictTls, "TESTING_INSECURE_TLS") == 0) {
    client.setInsecure();
    Serial.println(F("[TLS] Insecure mode enabled (Testing Only)"));
    return true;
  }

  if (_config.strictTls != nullptr && strcmp(_config.strictTls, "AUTO_CA_CERT_BUNDLE") == 0) {
#if defined(ESP_IDF_VERSION_MAJOR)
    client.setCACertBundle();
    Serial.println(F("[TLS] Native CA bundle enabled"));
    return true;
#else
    Serial.println(F("[TLS] Native CA bundle not supported in this build"));
    return false;
#endif
  }

  if (_config.strictTls != nullptr && strcmp(_config.strictTls, "MANUAL_CA_CERT") == 0) {
    const char *ca = pickCaForUrl(url);
    if (ca != nullptr && strlen(ca) > 0) {
      client.setCACert(ca);
      Serial.println(F("[TLS] Manual CA (high security)"));
      return true;
    }
  }

  if (_enableInsecure) {
    client.setInsecure();
    Serial.println(F("[TLS] Insecure mode enabled (CA configuration missing/failed)"));
    return true;
  }

  _enableInsecure = true;
  Serial.printf("[TLS] Critical Error: Missing CA or Bundle configuration for URL: %s\n", url.c_str());
  return false;
}

const char *GitHubOtaUpdater::pickCaForUrl(const String &url) const {
  if (url.indexOf("api.github.com") >= 0) return _config.githubApiCa;
  if (url.indexOf("githubusercontent.com") >= 0 || url.indexOf("release-assets.githubusercontent.com") >= 0 || url.indexOf("objects.githubusercontent.com") >= 0) return _config.githubAssetCa;
  if (url.indexOf("github.com") >= 0) return _config.githubWebCa;
  return nullptr;
}

bool GitHubOtaUpdater::isMatchingAsset(const String &name) const {
  return name.equals(_config.firmwareAssetName);
}

String GitHubOtaUpdater::bytesToHex(const uint8_t *data, const size_t length) {
  static const char hex[] = "0123456789abcdef";
  String out;
  out.reserve(length * 2);
  for (size_t i = 0; i < length; ++i) {
    out += hex[(data[i] >> 4) & 0x0F];
    out += hex[data[i] & 0x0F];
  }
  return out;
}

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
#include "esp_log.h"

/*
  =====================================================
  TAG SERIAL MONITOR
  =====================================================
*/

constexpr const char* TAG = "OTA-GitHub";

// PENTING: Tambahkan external reference ini di bagian atas file .cpp Anda
// Pastikan Anda sudah menambahkan `board_build.embed_files` di platformio.ini seperti langkah sebelumnya
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");
extern const uint8_t rootca_crt_bundle_end[]   asm("_binary_data_cert_x509_crt_bundle_bin_end");

GitHubOtaUpdater::GitHubOtaUpdater(const GitHubOtaConfig &config)
    : _config(config), _lastCheckMs(0) {}

void GitHubOtaUpdater::begin() {
  ESP_LOGI(TAG, "ESP32 GitHub Release OTA");

#ifndef FW_VERSION
ESP_LOGW(TAG, "FW_VERSION not defined");
#else
ESP_LOGI(TAG, "Current FW_VERSION: %s", FW_VERSION);
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
      ESP_LOGI(TAG, "Manual update check requested");
      checkNow();
    }
  }
}

bool GitHubOtaUpdater::checkNow() {
  if (!isNetworkReady()) {
    ESP_LOGW(TAG, "Network is not ready; OTA check skipped");
    return false;
  }

  ReleaseInfo release{};
  if (!fetchLatestRelease(release)) {
    _tlsFailCount++;
    if (_tlsFailCount >= 2) {
      Serial.println(F("[TLS] switching to emergency insecure mode"));
      _enableInsecure = true;
    }

    Serial.println(F("[OTA] Failed to fetch latest release"));
    return false;
  }

  _tlsFailCount = 0;

  ESP_LOGI(TAG, "Latest tag   : %s", release.tagName.c_str());
  ESP_LOGI(TAG, "Published at : %s", release.publishedAt.c_str());
  ESP_LOGI(TAG, "Asset        : %s", release.assetName.c_str());
  ESP_LOGI(TAG, "Size         : %u bytes", (unsigned)release.sizeBytes);

#ifdef FW_VERSION
  const String currentVersion = VersionUtils::normalize(FW_VERSION);
#else
  const String currentVersion = "0.0.0";
#endif
  const String latestVersion = VersionUtils::normalize(release.tagName);

  const int versionCompare = VersionUtils::compare(currentVersion, latestVersion);
  if (versionCompare >= 0) {
    ESP_LOGI(TAG, "No update available");
    return true;
  }

  ESP_LOGI(TAG, "Update available: %s -> %s",
          currentVersion.c_str(),
          latestVersion.c_str());

  const bool installed = downloadAndInstall(release);
  if (!installed) {
    ESP_LOGE(TAG, "OTA installation failed");
    _enableInsecure = true;
    return false;
  }

  ESP_LOGI(TAG, "OTA installation completed");
  _enableInsecure = false;
  _tlsFailCount = 0;

  if (_config.autoRebootAfterUpdate) {
    ESP_LOGI(TAG, "Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();
  }

  return true;
}

bool GitHubOtaUpdater::isNetworkReady() const {
  if (_config.networkReadyCheck != nullptr) {
    ESP_LOGD(TAG, "Checking network status...");
    return _config.networkReadyCheck();
  }

  return WiFi.status() == WL_CONNECTED;
}

void GitHubOtaUpdater::confirmPendingFirmwareIfNeeded() {
  if (!_config.enableRollbackConfirm) {
    return;
  }

  const esp_partition_t *runningPartition = esp_ota_get_running_partition();
  if (runningPartition == nullptr) {
    return;
  }

  esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
  if (esp_ota_get_state_partition(runningPartition, &state) != ESP_OK) {
    return;
  }

  if (state != ESP_OTA_IMG_PENDING_VERIFY) {
    return;
  }

  ESP_LOGI(TAG, "Firmware is in PENDING_VERIFY state");

  if (runSelfTest()) {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      ESP_LOGI(TAG, "Firmware marked VALID");
    } else {
      ESP_LOGE(TAG, "Failed to mark firmware VALID");
    }
  } else {
    ESP_LOGW(TAG, "Self-test failed, rolling back");
    esp_ota_mark_app_invalid_rollback_and_reboot();
  }
}

bool GitHubOtaUpdater::runSelfTest() const {
  if (_config.selfTestCheck != nullptr) {
    return _config.selfTestCheck();
  }

  // Safe default: do not rollback only because no custom self-test is provided.
  return true;
}

bool GitHubOtaUpdater::fetchLatestRelease(ReleaseInfo &release) {
  release = ReleaseInfo{};
  release.sizeBytes = 0;
  release.found = false;

  const String url =
      String("https://api.github.com/repos/") +
      _config.githubOwner + "/" +
      _config.githubRepo + "/releases/latest";

  NetworkClientSecure client;
  HTTPClient http;

  if (!beginHttp(http, client, url, true, false)) {
    return false;
  }

  const int statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    ESP_LOGE(TAG, "Latest release GET failed, status=%d", statusCode);
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(_config.jsonDocCapacity);
  const DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    ESP_LOGE(TAG, "parse failed: %s", error.c_str());
    return false;
  }

  const char *tagName = doc["tag_name"] | "";
  const char *publishedAt = doc["published_at"] | "";
  release.tagName = String(tagName);
  release.publishedAt = String(publishedAt);

  JsonArray assets = doc["assets"].as<JsonArray>();
  if (assets.isNull() || assets.size() == 0) {
    ESP_LOGW(TAG, "Release exists, but no assets were found");
    return false;
  }

  for (JsonObject asset : assets) {
    const char *assetName = asset["name"] | "";
    const char *assetState = asset["state"] | "";
    const String name = String(assetName);
    const String state = String(assetState);

    if (state != "uploaded") {
      continue;
    }

    if (!isMatchingAsset(name)) {
      continue;
    }

    release.assetName = name;
    const char *assetUrl = asset["url"] | "";
    const char *browserUrl = asset["browser_download_url"] | "";
    release.assetApiUrl = String(assetUrl);
    release.browserDownloadUrl = String(browserUrl);
    release.sizeBytes = static_cast<size_t>(asset["size"] | 0);

    const char *assetDigest = asset["digest"] | "";
    String digest = String(assetDigest);
    digest.trim();
    if (digest.startsWith("sha256:")) {
      digest.remove(0, 7);
    }
    digest.toLowerCase();
    release.sha256 = digest;

    release.found = true;
    break;
  }

  if (!release.found) {
    ESP_LOGW(TAG, "Asset '%s' was not found in the latest release", _config.firmwareAssetName);
    return false;
  }

  return true;
}

bool GitHubOtaUpdater::downloadAndInstall(const ReleaseInfo &release) {
  String currentUrl = release.assetApiUrl.length() > 0
                          ? release.assetApiUrl
                          : release.browserDownloadUrl;

  for (uint8_t redirectCount = 0; redirectCount <= _config.maxRedirects; ++redirectCount) {
    NetworkClientSecure client;
    HTTPClient http;
    const char *headerKeys[] = {"Location"};

    if (!beginHttp(http, client, currentUrl, false, true)) {
      return false;
    }

    http.collectHeaders(headerKeys, 1);

    ESP_LOGI(TAG, "Download URL: %s", currentUrl.c_str());
    const int statusCode = http.GET();

    if (statusCode == HTTP_CODE_OK) {
      const bool success = installFromHttp(http, release);
      http.end();
      return success;
    }

    if (statusCode == HTTP_CODE_MOVED_PERMANENTLY ||
        statusCode == HTTP_CODE_FOUND ||
        statusCode == HTTP_CODE_SEE_OTHER ||
        statusCode == 307 ||
        statusCode == 308) {
      const String location = http.header("Location");
      http.end();

      if (location.isEmpty()) {
        ESP_LOGE(TAG, "Redirect without a Location header");
        return false;
      }

      Serial.printf("[HTTP] Redirect -> %s\n", location.c_str());
      currentUrl = location;
      continue;
    }

    ESP_LOGE(TAG, "Firmware download failed, status=%d", statusCode);
    http.end();
    return false;
  }

  ESP_LOGE(TAG, "Too many redirects");
  return false;
}

bool GitHubOtaUpdater::installFromHttp(HTTPClient &http, const ReleaseInfo &release) {
  size_t expectedSize = release.sizeBytes;
  const int responseLength = http.getSize();
  if (expectedSize == 0 && responseLength > 0) {
    expectedSize = static_cast<size_t>(responseLength);
  }

  if (!Update.begin(expectedSize > 0 ? expectedSize : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
    ESP_LOGE(TAG, "Update.begin failed: %s", Update.errorString());
    return false;
  }

  auto *stream = http.getStreamPtr();
  if (stream == nullptr) {
    ESP_LOGE(TAG, "Stream pointer is null");
    Update.abort();
    return false;
  }

  if (_config.downloadBufferSize == 0) {
    Serial.println(F("[OTA] Download buffer size must be greater than zero"));
    Update.abort();
    return false;
  }

  uint8_t *buffer = new uint8_t[_config.downloadBufferSize];
  if (buffer == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate download buffer");
    Update.abort();
    return false;
  }

  size_t written = 0;
  int lastPercent = -1;
  uint32_t lastDataMs = millis();

  mbedtls_sha256_context shaContext;
  mbedtls_sha256_init(&shaContext);

  if (mbedtls_sha256_starts(&shaContext, 0) != 0) {
    ESP_LOGE(TAG, "SHA256 starts failed");
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
        ESP_LOGE(TAG, "Read timeout");
        ok = false;
        break;
      }

      delay(1);
      continue;
    }

    if (availableBytes > _config.downloadBufferSize) {
      availableBytes = _config.downloadBufferSize;
    }

    const int readLen = stream->readBytes(reinterpret_cast<char *>(buffer), availableBytes);
    if (readLen <= 0) {
      delay(1);
      continue;
    }

    lastDataMs = millis();

    if (mbedtls_sha256_update(&shaContext, buffer, static_cast<size_t>(readLen)) != 0) {
      ESP_LOGE(TAG, "SHA256 update failed");
      ok = false;
      break;
    }

    const size_t justWritten = Update.write(buffer, static_cast<size_t>(readLen));
    if (justWritten != static_cast<size_t>(readLen)) {
      ESP_LOGE(TAG, "Update.write failed: %s", Update.errorString());
      ok = false;
      break;
    }

    written += justWritten;

    if (expectedSize > 0) {
      const int percent = static_cast<int>((written * 100U) / expectedSize);
      if (percent != lastPercent) {
        lastPercent = percent;
        ESP_LOGI(TAG, "Progress: %d%% (%u/%u)",
                percent,
                (unsigned)written,
                (unsigned)expectedSize);
      }

      if (written >= expectedSize) {
        break;
      }
    }
  }

  uint8_t digestRaw[32] = {0};
  if (ok && mbedtls_sha256_finish(&shaContext, digestRaw) != 0) {
    ESP_LOGE(TAG, "SHA256 finish failed");
    ok = false;
  }
  mbedtls_sha256_free(&shaContext);

  delete[] buffer;

  if (!ok) {
    Update.abort();
    return false;
  }

  if (expectedSize > 0 && written != expectedSize) {
    ESP_LOGE(TAG, "Incomplete firmware image: %u / %u bytes",
            (unsigned)written,
            (unsigned)expectedSize);
    Update.abort();
    return false;
  }

  const String actualSha256 = bytesToHex(digestRaw, sizeof(digestRaw));
  if (release.sha256.length() > 0) {
    if (!actualSha256.equalsIgnoreCase(release.sha256)) {
      ESP_LOGE(TAG, "SHA-256 mismatch");
      Serial.printf("[OTA] Expected: %s\n", release.sha256.c_str());
      Serial.printf("[OTA] Actual  : %s\n", actualSha256.c_str());
      Update.abort();
      return false;
    }

    ESP_LOGI(TAG, "SHA-256 verified");
  } else {
    ESP_LOGW(TAG, "Warning: release metadata did not include SHA-256 digest");
  }

  if (!Update.end()) {
    ESP_LOGE(TAG, "Update.end failed: %s", Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    ESP_LOGE(TAG, "Update did not finish cleanly");
    return false;
  }

  ESP_LOGI(TAG, "Firmware successfully written");
  return true;
}

bool GitHubOtaUpdater::beginHttp(HTTPClient &http,
                                 NetworkClientSecure &client,
                                 const String &url,
                                 bool acceptJson,
                                 bool acceptOctetStream) {
  if (!configureSecureClient(client, url)) {
    return false;
  }

  http.setConnectTimeout(_config.httpConnectTimeoutMs);
  http.setTimeout(_config.httpReadTimeoutMs);
  http.useHTTP10(true);

  if (!http.begin(client, url)) {
    ESP_LOGE(TAG, "HTTP begin() failed: %s", url.c_str());
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
  client.setHandshakeTimeout(30);
  client.setTimeout(_config.httpReadTimeoutMs / 1000);

  if (_enableInsecure && _config.allowInsecureTlsForEmergency) { 
    client.setInsecure();
    ESP_LOGI(TAG, "Emergency mode enabled (CA configuration missing/failed)");
    return true; // WAJIB return true agar proses handshake HTTPS berjalan
  }

  if (_config.strictTls == TESTING_INSECURE_TLS) {
    client.setInsecure();
    ESP_LOGI(TAG, "Insecure mode enabled (Testing Only)");
    return true;
  }

  // Khusus ESP-IDF
  // if(_config.strictTls == AUTO_CA_CERT_BUNDLE) {
  //   client.setCACertBundle(esp_crt_bundle_attach, nullptr);
  //   ESP_LOGI(TAG, "Bundle CA Cert mode enabled (Native Cert Bundle)");
  //   return true;
  // }

  if(_config.strictTls == CUSTOM_CA_CERT_BUNDLE) {
    size_t bundle_size = rootca_crt_bundle_end - rootca_crt_bundle_start;
    if (bundle_size > 0) {
      client.setCACertBundle(rootca_crt_bundle_start, bundle_size);
      ESP_LOGI(TAG, "CA Cert Bundle (Fallback Custom Bin)");
      return true;
    }
  }

  if(_config.strictTls == CUSTOM_CA_CERT) {
    const char *ca = pickCaForUrl(url);
    // 2. Mode Strict Tinggi - Menggunakan Hardcoded/Manual CA (.setCACert)
    if (ca != nullptr && strlen(ca) > 0) {
      client.setCACert(ca);
      ESP_LOGI(TAG, "Manual CA (high security)");
      return true;
    } 
  }

  // Jika strictTls = true, CA kosong, dan file bundle .bin tidak di-embed
  _enableInsecure = true;
  ESP_LOGE(TAG, "Critical Error: Missing CA or Bundle configuration for URL: %s", url.c_str());
  return false;
}

const char *GitHubOtaUpdater::pickCaForUrl(const String &url) const {

  // API GitHub
  if (url.indexOf("api.github.com") >= 0) {
    return _config.githubApiCa;   // DIGICERT G2
  }

  // RAW ASSETS / CDN
  if (url.indexOf("githubusercontent.com") >= 0 ||
      url.indexOf("release-assets.githubusercontent.com") >= 0 ||
      url.indexOf("objects.githubusercontent.com") >= 0) {
    return _config.githubAssetCa;  // ISRG X1 (OK)
  }

  // WEB GitHub
  if (url.indexOf("github.com") >= 0) {
    return _config.githubWebCa;    // DIGICERT G2
  }

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

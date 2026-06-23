#ifndef GITHUB_OTA_H
#define GITHUB_OTA_H

#include <Arduino.h>

using OtaCheckCallback = bool (*)();

struct GitHubOtaConfig {
  // GitHub release
  const char *githubOwner;
  const char *githubRepo;
  const char *firmwareAssetName;
  const char *githubToken;
  const char *githubApiVersion;

  // Behavior
  bool checkOnBoot;
  bool autoRebootAfterUpdate;
  bool enableRollbackConfirm;
  uint32_t checkIntervalMs;
  uint32_t httpConnectTimeoutMs;
  uint32_t httpReadTimeoutMs;
  uint8_t maxRedirects;
  size_t jsonDocCapacity;
  size_t downloadBufferSize;

  // Optional callbacks managed by the application
  // - networkReadyCheck: return true only when network is ready for HTTPS.
  // - selfTestCheck: return true when the newly booted firmware is healthy.
  OtaCheckCallback networkReadyCheck;
  OtaCheckCallback selfTestCheck;

  // TLS
  const char *strictTls;
  bool allowInsecureTlsForTesting;
  const char *githubApiCa;
  const char *githubWebCa;
  const char *githubAssetCa;
};

struct ReleaseInfo {
  String tagName;
  String publishedAt;
  String assetName;
  String assetApiUrl;
  String browserDownloadUrl;
  String sha256;
  size_t sizeBytes;
  bool found;
};

class GitHubOtaUpdater {
 public:
  explicit GitHubOtaUpdater(const GitHubOtaConfig &config);

  void begin();
  void loop();
  bool checkNow();

 private:
  bool isNetworkReady() const;
  void confirmPendingFirmwareIfNeeded();
  bool runSelfTest() const;

  bool fetchLatestRelease(ReleaseInfo &release);
  bool downloadAndInstall(const ReleaseInfo &release);
  bool installFromHttp(class HTTPClient &http, const ReleaseInfo &release);

  bool beginHttp(class HTTPClient &http,
                 class NetworkClientSecure &client,
                 const String &url,
                 bool acceptJson,
                 bool acceptOctetStream);

  bool configureSecureClient(class NetworkClientSecure &client, const String &url);
  const char *pickCaForUrl(const String &url) const;
  bool isMatchingAsset(const String &name) const;

  static String bytesToHex(const uint8_t *data, size_t length);

  const GitHubOtaConfig &_config;
  uint32_t _lastCheckMs;

  bool _enableInsecure = false;
};

#endif
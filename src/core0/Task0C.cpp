#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"
#include "environment.h"

#include "GitHubOtaUpdater.h"

#ifndef FW_VERSION
#define FW_VERSION "1.0.0"
#endif

/*
  =====================================================
  KONFIGURASI GITHUB
  =====================================================
*/

// constexpr char GITHUB_OWNER[] = "owner-anda";
// constexpr char GITHUB_REPO[] = "repo-anda";
// constexpr char FIRMWARE_ASSET_NAME[] = "firmware-esp32.bin";

// // Kosongkan untuk repo publik.
// // Untuk repo privat, isi token dengan permission Contents: read.
// constexpr char GITHUB_TOKEN[] = "";

// constexpr char GITHUB_API_VERSION[] = "2026-03-10";

// constexpr bool CHECK_ON_BOOT = true;
// constexpr bool AUTO_REBOOT_AFTER_UPDATE = true;
// constexpr bool ENABLE_ROLLBACK_CONFIRM = true;

// constexpr uint32_t CHECK_INTERVAL_MS = 5UL * 60UL * 1000UL; // 6UL * 60UL * 60UL * 1000UL
// constexpr uint32_t HTTP_CONNECT_TIMEOUT_MS = 15000;
// constexpr uint32_t HTTP_READ_TIMEOUT_MS = 30000;

// constexpr uint8_t MAX_REDIRECTS = 5;
// constexpr size_t JSON_DOC_CAPACITY = 24 * 1024;
// constexpr size_t DOWNLOAD_BUFFER_SIZE = 4096;

// constexpr bool STRICT_TLS = true;
// constexpr bool ALLOW_INSECURE_TLS_FOR_EMERGENCY = false;

// // Isi CA PEM untuk produksi.
// const char GITHUB_API_CA[] PROGMEM = R"EOF()EOF";

// const char GITHUB_WEB_CA[] PROGMEM = R"EOF()EOF";

// const char GITHUB_ASSET_CA[] PROGMEM = R"EOF()EOF";

/*
  =====================================================
  CHECK STATUS
  =====================================================
*/

// Ganti isi fungsi ini bila Anda punya cara sendiri untuk mengecek konektivitas.
bool isOtaNetworkReady() {
    return (wifiSTA_running.load() && wifiStatus_running.load());
}

// Ganti dengan health-check aplikasi yang benar-benar relevan.
// Jangan rollback hanya karena Wi-Fi belum sempat reconnect.
bool otaSelfTest() {
    // return true  -> firmware baru valid
    // return false -> firmware baru gagal dan rollback
    return true;
}

/*
  =====================================================
  KONFIGURASI GITHUB
  =====================================================
*/

GitHubOtaConfig buildOtaConfig() {
    GitHubOtaConfig config{};
    config.githubOwner = GITHUB_OWNER;
    config.githubRepo = GITHUB_REPO;
    config.firmwareAssetName = FIRMWARE_ASSET_NAME;
    config.githubToken = GITHUB_TOKEN;
    config.githubApiVersion = GITHUB_API_VERSION;
    config.checkOnBoot = CHECK_ON_BOOT;
    config.autoRebootAfterUpdate = AUTO_REBOOT_AFTER_UPDATE;
    config.enableRollbackConfirm = ENABLE_ROLLBACK_CONFIRM;
    config.checkIntervalMs = CHECK_INTERVAL_MS;
    config.httpConnectTimeoutMs = HTTP_CONNECT_TIMEOUT_MS;
    config.httpReadTimeoutMs = HTTP_READ_TIMEOUT_MS;
    config.maxRedirects = MAX_REDIRECTS;
    config.jsonDocCapacity = JSON_DOC_CAPACITY;
    config.downloadBufferSize = DOWNLOAD_BUFFER_SIZE;
    config.networkReadyCheck = isOtaNetworkReady;
    config.selfTestCheck = otaSelfTest;
    config.strictTls = STRICT_TLS;
    config.allowInsecureTlsForEmergency = ALLOW_INSECURE_TLS_FOR_EMERGENCY;
    config.githubApiCa = GITHUB_API_CA;
    config.githubWebCa = GITHUB_WEB_CA;
    config.githubAssetCa = GITHUB_ASSET_CA;
    return config;
}

GitHubOtaConfig otaConfig = buildOtaConfig();
GitHubOtaUpdater otaUpdater(otaConfig);

/*
  =====================================================
  TASK 0C
  =====================================================
*/

void Task0C(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(15000));
    otaUpdater.begin();

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(1000UL);

    for (;;) {
        otaUpdater.loop();

        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}

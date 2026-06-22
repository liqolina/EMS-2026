#include "core0Task.h"
#include "mutex_global.h"
#include "variable_global.h"

#include "GitHubOtaUpdater.h"

#ifndef FW_VERSION
#define FW_VERSION "1.0.0"
#endif

namespace AppConfig {
constexpr char GITHUB_OWNER[] = "owner-anda";
constexpr char GITHUB_REPO[] = "repo-anda";
constexpr char FIRMWARE_ASSET_NAME[] = "firmware-esp32.bin";

// Kosongkan untuk repo publik.
// Untuk repo privat, isi token dengan permission Contents: read.
constexpr char GITHUB_TOKEN[] = "";

constexpr char GITHUB_API_VERSION[] = "2026-03-10";

constexpr bool CHECK_ON_BOOT = true;
constexpr bool AUTO_REBOOT_AFTER_UPDATE = true;
constexpr bool ENABLE_ROLLBACK_CONFIRM = true;

constexpr uint32_t CHECK_INTERVAL_MS = 5UL * 60UL * 1000UL; // 6UL * 60UL * 60UL * 1000UL
constexpr uint32_t HTTP_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t HTTP_READ_TIMEOUT_MS = 30000;

constexpr uint8_t MAX_REDIRECTS = 5;
constexpr size_t JSON_DOC_CAPACITY = 24 * 1024;
constexpr size_t DOWNLOAD_BUFFER_SIZE = 4096;

constexpr bool STRICT_TLS = true;
constexpr bool ALLOW_INSECURE_TLS_FOR_TESTING = false;

// Isi CA PEM untuk produksi.
const char GITHUB_API_CA[] PROGMEM = R"EOF(
)EOF";

const char GITHUB_WEB_CA[] PROGMEM = R"EOF(
)EOF";

const char GITHUB_ASSET_CA[] PROGMEM = R"EOF(
)EOF";
}  // namespace AppConfig

// Wi-Fi sudah dikelola di modul lain.
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

GitHubOtaConfig buildOtaConfig() {
    GitHubOtaConfig config{};
    config.githubOwner = AppConfig::GITHUB_OWNER;
    config.githubRepo = AppConfig::GITHUB_REPO;
    config.firmwareAssetName = AppConfig::FIRMWARE_ASSET_NAME;
    config.githubToken = AppConfig::GITHUB_TOKEN;
    config.githubApiVersion = AppConfig::GITHUB_API_VERSION;
    config.checkOnBoot = AppConfig::CHECK_ON_BOOT;
    config.autoRebootAfterUpdate = AppConfig::AUTO_REBOOT_AFTER_UPDATE;
    config.enableRollbackConfirm = AppConfig::ENABLE_ROLLBACK_CONFIRM;
    config.checkIntervalMs = AppConfig::CHECK_INTERVAL_MS;
    config.httpConnectTimeoutMs = AppConfig::HTTP_CONNECT_TIMEOUT_MS;
    config.httpReadTimeoutMs = AppConfig::HTTP_READ_TIMEOUT_MS;
    config.maxRedirects = AppConfig::MAX_REDIRECTS;
    config.jsonDocCapacity = AppConfig::JSON_DOC_CAPACITY;
    config.downloadBufferSize = AppConfig::DOWNLOAD_BUFFER_SIZE;
    config.networkReadyCheck = isOtaNetworkReady;
    config.selfTestCheck = otaSelfTest;
    config.strictTls = AppConfig::STRICT_TLS;
    config.allowInsecureTlsForTesting = AppConfig::ALLOW_INSECURE_TLS_FOR_TESTING;
    config.githubApiCa = AppConfig::GITHUB_API_CA;
    config.githubWebCa = AppConfig::GITHUB_WEB_CA;
    config.githubAssetCa = AppConfig::GITHUB_ASSET_CA;
    return config;
}

GitHubOtaConfig otaConfig = buildOtaConfig();
GitHubOtaUpdater otaUpdater(otaConfig);

void Task0C(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(7000));
    otaUpdater.begin();

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t samplingInterval = pdMS_TO_TICKS(5UL * 60UL * 1000UL);

    for (;;) {
        otaUpdater.loop();

        vTaskDelayUntil(&lastWakeTime, samplingInterval);
    }
}

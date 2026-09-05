#include "ota_manager.h"
#include "config.h"
#include "time_sync.h"
#include "ui.h"
#include "display_config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_ota_ops.h>

static WebServer server(80);
static bool otaServicesInitialized = false;
static bool otaActive = false;

// HTML page for the root dashboard
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-E E-Paper Clock</title>
    <style>
        :root {
            --bg: #0f172a;
            --card-bg: #1e293b;
            --card-border: #334155;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --accent: #38bdf8;
            --accent-hover: #0ea5e9;
            --success: #22c55e;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text-main);
            margin: 0;
            padding: 24px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            box-sizing: border-box;
        }
        .container {
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            border-radius: 16px;
            padding: 32px;
            max-width: 540px;
            width: 100%;
            box-shadow: 0 10px 25px -5px rgba(0,0,0,0.5);
        }
        h1 {
            margin: 0 0 8px 0;
            font-size: 1.5rem;
            color: var(--accent);
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .subtitle {
            color: var(--text-muted);
            margin-bottom: 24px;
            font-size: 0.9rem;
        }
        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-bottom: 24px;
        }
        .stat-box {
            background: rgba(15, 23, 42, 0.6);
            border: 1px solid var(--card-border);
            border-radius: 8px;
            padding: 12px;
        }
        .stat-label {
            font-size: 0.75rem;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
        .stat-value {
            font-size: 1.1rem;
            font-weight: 600;
            margin-top: 4px;
        }
        .btn-group {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        .btn {
            display: inline-block;
            text-align: center;
            background: var(--accent);
            color: #0f172a;
            font-weight: 600;
            padding: 12px 20px;
            border-radius: 8px;
            text-decoration: none;
            transition: all 0.2s ease;
            border: none;
            cursor: pointer;
        }
        .btn:hover {
            background: var(--accent-hover);
        }
        .btn-secondary {
            background: transparent;
            border: 1px solid var(--card-border);
            color: var(--text-main);
        }
        .btn-secondary:hover {
            background: rgba(255,255,255,0.05);
        }
        .badge {
            display: inline-block;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 0.75rem;
            background: rgba(34, 197, 94, 0.2);
            color: var(--success);
            border: 1px solid rgba(34, 197, 94, 0.4);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32-E E-Paper Clock <span class="badge">OTA ACTIVE</span></h1>
        <div class="subtitle">FireBeetle 2 ESP32-E &bull; Waveshare 7.5" V2 Panel</div>

        <div class="grid">
            <div class="stat-box">
                <div class="stat-label">IP Address</div>
                <div class="stat-value">%IP_ADDR%</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Wi-Fi RSSI</div>
                <div class="stat-value">%WIFI_RSSI% dBm</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Active Partition</div>
                <div class="stat-value">%ACTIVE_PART%</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Free Heap</div>
                <div class="stat-value">%FREE_HEAP% KB</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Flash Size</div>
                <div class="stat-value">%FLASH_SIZE% MB</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Uptime</div>
                <div class="stat-value">%UPTIME% s</div>
            </div>
        </div>

        <div class="btn-group">
            <a href="/update" class="btn">Update Firmware via Browser</a>
            <button onclick="reboot()" class="btn btn-secondary">Restart Clock</button>
        </div>
    </div>
    <script>
        function reboot() {
            if (confirm("Restart clock now?")) {
                fetch('/restart').then(() => alert("Rebooting ESP32..."));
            }
        }
    </script>
</body>
</html>
)rawliteral";

// HTML page for firmware file upload with live progress bar
static const char UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Firmware Update - ESP32-E Clock</title>
    <style>
        :root {
            --bg: #0f172a;
            --card-bg: #1e293b;
            --card-border: #334155;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --accent: #38bdf8;
            --accent-hover: #0ea5e9;
            --success: #22c55e;
            --error: #ef4444;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text-main);
            margin: 0;
            padding: 24px;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            box-sizing: border-box;
        }
        .container {
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            border-radius: 16px;
            padding: 32px;
            max-width: 500px;
            width: 100%;
            box-shadow: 0 10px 25px -5px rgba(0,0,0,0.5);
        }
        h1 {
            margin: 0 0 8px 0;
            font-size: 1.4rem;
            color: var(--accent);
        }
        .subtitle {
            color: var(--text-muted);
            margin-bottom: 24px;
            font-size: 0.9rem;
        }
        .upload-card {
            border: 2px dashed var(--card-border);
            border-radius: 12px;
            padding: 28px 16px;
            text-align: center;
            margin-bottom: 20px;
            background: rgba(15, 23, 42, 0.4);
            cursor: pointer;
            transition: all 0.2s ease;
        }
        .upload-card:hover {
            border-color: var(--accent);
        }
        input[type="file"] {
            display: none;
        }
        .file-label {
            font-size: 1rem;
            color: var(--text-main);
            cursor: pointer;
            display: block;
        }
        .file-hint {
            color: var(--text-muted);
            font-size: 0.8rem;
            margin-top: 6px;
        }
        .selected-file {
            margin-top: 10px;
            font-size: 0.9rem;
            font-weight: 600;
            color: var(--accent);
        }
        .progress-container {
            display: none;
            margin-bottom: 20px;
        }
        .progress-bar-bg {
            background: rgba(15, 23, 42, 0.8);
            border: 1px solid var(--card-border);
            border-radius: 10px;
            height: 20px;
            overflow: hidden;
            position: relative;
        }
        .progress-bar-fill {
            background: linear-gradient(90deg, #38bdf8, #0ea5e9);
            height: 100%;
            width: 0%;
            transition: width 0.15s ease;
        }
        .progress-text {
            margin-top: 8px;
            font-size: 0.85rem;
            color: var(--text-muted);
            text-align: center;
        }
        .btn {
            display: block;
            width: 100%;
            background: var(--accent);
            color: #0f172a;
            font-weight: 600;
            padding: 12px 20px;
            border-radius: 8px;
            border: none;
            cursor: pointer;
            font-size: 1rem;
            transition: all 0.2s ease;
            box-sizing: border-box;
        }
        .btn:hover:not(:disabled) {
            background: var(--accent-hover);
        }
        .btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .back-link {
            display: block;
            text-align: center;
            margin-top: 16px;
            color: var(--text-muted);
            text-decoration: none;
            font-size: 0.85rem;
        }
        .back-link:hover {
            color: var(--accent);
        }
        .status-msg {
            margin-top: 16px;
            padding: 12px;
            border-radius: 8px;
            font-size: 0.9rem;
            text-align: center;
            display: none;
        }
        .status-success {
            background: rgba(34, 197, 94, 0.2);
            color: var(--success);
            border: 1px solid rgba(34, 197, 94, 0.4);
            display: block;
        }
        .status-error {
            background: rgba(239, 68, 68, 0.2);
            color: var(--error);
            border: 1px solid rgba(239, 68, 68, 0.4);
            display: block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Wireless Firmware Update</h1>
        <div class="subtitle">Select firmware.bin compiled for ESP32-E</div>

        <form id="upload-form">
            <div class="upload-card" onclick="document.getElementById('firmware-file').click()">
                <label class="file-label" id="file-label-text">Click to choose firmware file</label>
                <div class="file-hint">Accepts .bin compiled binaries</div>
                <div class="selected-file" id="selected-file-name"></div>
                <input type="file" id="firmware-file" name="update" accept=".bin" onchange="onFileSelected()">
            </div>

            <div class="progress-container" id="progress-container">
                <div class="progress-bar-bg">
                    <div class="progress-bar-fill" id="progress-fill"></div>
                </div>
                <div class="progress-text" id="progress-text">0%</div>
            </div>

            <button type="button" id="upload-btn" class="btn" onclick="startUpload()" disabled>Upload & Flash</button>
            <div id="status-msg" class="status-msg"></div>
            <a href="/" class="back-link">&larr; Back to Dashboard</a>
        </form>
    </div>

    <script>
        const fileInput = document.getElementById('firmware-file');
        const fileLabel = document.getElementById('file-label-text');
        const fileNameDiv = document.getElementById('selected-file-name');
        const uploadBtn = document.getElementById('upload-btn');
        const progressContainer = document.getElementById('progress-container');
        const progressFill = document.getElementById('progress-fill');
        const progressText = document.getElementById('progress-text');
        const statusMsg = document.getElementById('status-msg');

        function onFileSelected() {
            if (fileInput.files.length > 0) {
                const file = fileInput.files[0];
                fileLabel.innerText = "Selected File:";
                fileNameDiv.innerText = file.name + " (" + (file.size / 1024).toFixed(1) + " KB)";
                uploadBtn.disabled = false;
            }
        }

        function startUpload() {
            if (!fileInput.files.length) return;
            const file = fileInput.files[0];

            uploadBtn.disabled = true;
            fileInput.disabled = true;
            progressContainer.style.display = 'block';
            statusMsg.className = 'status-msg';
            statusMsg.style.display = 'none';

            const xhr = new XMLHttpRequest();
            const formData = new FormData();
            formData.append('update', file);

            xhr.upload.addEventListener('progress', (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    progressFill.style.width = percent + '%';
                    progressText.innerText = 'Uploading: ' + percent + '% (' + (e.loaded / 1024).toFixed(0) + ' / ' + (e.total / 1024).toFixed(0) + ' KB)';
                    if (percent >= 100) {
                        progressText.innerText = 'Flashing into partition... Please wait.';
                    }
                }
            });

            xhr.onload = function() {
                if (xhr.status === 200) {
                    statusMsg.innerText = "Update Successful! Clock is rebooting now...";
                    statusMsg.className = "status-msg status-success";
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 8000);
                } else {
                    statusMsg.innerText = "Update Failed (HTTP " + xhr.status + "): " + xhr.responseText;
                    statusMsg.className = "status-msg status-error";
                    uploadBtn.disabled = false;
                    fileInput.disabled = false;
                }
            };

            xhr.onerror = function() {
                statusMsg.innerText = "Upload failed due to network error.";
                statusMsg.className = "status-msg status-error";
                uploadBtn.disabled = false;
                fileInput.disabled = false;
            };

            xhr.open('POST', '/update?size=' + file.size);
            xhr.send(formData);
        }
    </script>
</body>
</html>
)rawliteral";

static String generateIndexHtml() {
    String html = FPSTR(INDEX_HTML);
    html.replace("%IP_ADDR%", WiFi.localIP().toString());
    html.replace("%WIFI_RSSI%", String(WiFi.RSSI()));
    
    const esp_partition_t* running = esp_ota_get_running_partition();
    html.replace("%ACTIVE_PART%", running ? String(running->label) : "Unknown");
    
    html.replace("%FREE_HEAP%", String(ESP.getFreeHeap() / 1024));
    html.replace("%FLASH_SIZE%", String(ESP.getFlashChipSize() / (1024 * 1024)));
    html.replace("%UPTIME%", String(millis() / 1000));
    return html;
}

void initOtaServices() {
    if (otaServicesInitialized) {
        return;
    }

    log_i("Initializing OTA services (mDNS, ArduinoOTA, WebServer)...");

    // 1. Initialize mDNS
    if (!MDNS.begin(OTA_HOSTNAME)) {
        log_w("mDNS responder failed to start.");
    } else {
        log_i("mDNS responder started: http://%s.local", OTA_HOSTNAME);
        MDNS.addService("http", "tcp", 80);
    }

    // 2. Initialize ArduinoOTA (for PlatformIO CLI / IDE uploads)
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(OTA_HOSTNAME);

    ArduinoOTA.onStart([]() {
        otaActive = true;
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "Filesystem";
        log_i("[ArduinoOTA] Start updating %s...", type.c_str());
        char sub[64];
        snprintf(sub, sizeof(sub), "Receiving %s via port %d...", type.c_str(), OTA_PORT);
        showOtaScreen("ARDUINOTA UPDATE", sub);
    });

    ArduinoOTA.onEnd([]() {
        log_i("[ArduinoOTA] Update completed successfully! Rebooting...");
        updateOtaProgress(100, 100, 100);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static uint32_t lastArduOtaMs = 0;
        static unsigned int lastPercent = 0;
        unsigned int percent = (total > 0) ? (progress * 100 / total) : 0;
        if (percent % 10 == 0 && percent != lastPercent) {
            log_i("[ArduinoOTA] Progress: %u%%", percent);
            lastPercent = percent;
        }

        uint32_t now = millis();
        if (now - lastArduOtaMs >= 2000 || percent >= 100) {
            lastArduOtaMs = now;
            updateOtaProgress(percent, progress, total);
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        otaActive = false;
        log_e("[ArduinoOTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) log_e("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) log_e("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) log_e("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) log_e("Receive Failed");
        else if (error == OTA_END_ERROR) log_e("End Failed");
    });

    ArduinoOTA.begin();

    // 3. Initialize WebServer routes
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", generateIndexHtml());
    });

    server.on("/update", HTTP_GET, []() {
        server.send_P(200, "text/html", UPDATE_HTML);
    });

    server.on("/restart", HTTP_GET, []() {
        server.send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });

    // POST /update handler for browser multipart uploads
    static size_t webOtaTotalSize = 0;
    static uint32_t webOtaLastDisplayMs = 0;

    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        if (Update.hasError()) {
            server.send(500, "text/plain", "Update Failed!");
        } else {
            server.send(200, "text/plain", "OK");
            delay(1000);
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            otaActive = true;
            log_i("[WebOTA] Upload started: %s", upload.filename.c_str());

            webOtaTotalSize = 0;
            if (server.hasArg("size")) {
                webOtaTotalSize = (size_t)server.arg("size").toInt();
            }

            char sub[96];
            if (webOtaTotalSize > 0) {
                snprintf(sub, sizeof(sub), "File: %s (%u KB)", 
                         upload.filename.c_str(), (unsigned int)(webOtaTotalSize / 1024));
            } else {
                snprintf(sub, sizeof(sub), "File: %s", upload.filename.c_str());
            }
            showOtaScreen("WEB BROWSER UPDATE", sub);
            webOtaLastDisplayMs = millis();

            size_t updateSize = (webOtaTotalSize > 0) ? webOtaTotalSize : UPDATE_SIZE_UNKNOWN;
            if (!Update.begin(updateSize)) {
                Update.printError(Serial);
                otaActive = false;
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            } else {
                uint32_t now = millis();
                if (now - webOtaLastDisplayMs >= 2000) {
                    webOtaLastDisplayMs = now;
                    int pct = (webOtaTotalSize > 0) ? ((upload.totalSize * 100) / webOtaTotalSize) : 0;
                    if (pct > 99) pct = 99;
                    updateOtaProgress(pct, upload.totalSize, webOtaTotalSize);
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                log_i("[WebOTA] Update success: %u bytes written. Rebooting clock...", upload.totalSize);
                updateOtaProgress(100, upload.totalSize, (webOtaTotalSize > 0 ? webOtaTotalSize : upload.totalSize));
            } else {
                Update.printError(Serial);
                otaActive = false;
            }
        }
    });

    server.begin();
    otaServicesInitialized = true;
    log_i("OTA services ready. Web: http://%s/update | ArduinoOTA port: %d", 
          WiFi.localIP().toString().c_str(), OTA_PORT);
}

bool runOtaWindow(uint32_t timeoutSeconds) {
    if (!ensureWiFiConnected()) {
        log_w("Cannot open OTA window: Wi-Fi connection failed.");
        return false;
    }

    initOtaServices();

    log_i("=================================================");
    log_i("OTA Update Window Active (Timeout: %u seconds)", timeoutSeconds);
    log_i("Web Browser: http://%s/update", WiFi.localIP().toString().c_str());
    log_i("mDNS URL:    http://%s.local/update", OTA_HOSTNAME);
    log_i("PlatformIO:  pio run -e firebeetle2_esp32e_ota -t upload");
    log_i("=================================================");

    uint32_t windowStart = millis();
    uint32_t lastLogMs = 0;

    while (true) {
        ArduinoOTA.handle();
        server.handleClient();

        if (otaActive) {
            // Reset timeout if upload has begun so it never cancels mid-flash
            windowStart = millis();
        } else {
            uint32_t elapsedSec = (millis() - windowStart) / 1000;
            if (elapsedSec >= timeoutSeconds) {
                log_i("OTA window timeout reached (%u s). Closing OTA and shutting down Wi-Fi...", timeoutSeconds);
                break;
            }

            if (millis() - lastLogMs >= 5000) {
                lastLogMs = millis();
                log_i("OTA listening... %u seconds remaining (http://%s/update)", 
                      timeoutSeconds - elapsedSec, WiFi.localIP().toString().c_str());
            }
        }

        delay(5);
    }

    // Clean up OTA services and shut down Wi-Fi radio
    server.stop();
    ArduinoOTA.end();
    MDNS.end();
    otaServicesInitialized = false;

    disconnectWiFi();
    return false;
}

bool isOtaInProgress() {
    return otaActive;
}

String getOtaIpAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "";
}

String getOtaWebUrl() {
    if (WiFi.status() == WL_CONNECTED) {
        return "http://" + WiFi.localIP().toString() + "/update";
    }
    return "";
}

bool isVersionNewer(const char* remoteVer, const char* localVer) {
    if (!remoteVer || !localVer) return false;

    // Skip leading 'v' or 'V' if present
    if (*remoteVer == 'v' || *remoteVer == 'V') remoteVer++;
    if (*localVer == 'v' || *localVer == 'V') localVer++;

    int rMaj = 0, rMin = 0, rPatch = 0;
    int lMaj = 0, lMin = 0, lPatch = 0;

    sscanf(remoteVer, "%d.%d.%d", &rMaj, &rMin, &rPatch);
    sscanf(localVer, "%d.%d.%d", &lMaj, &lMin, &lPatch);

    if (rMaj > lMaj) return true;
    if (rMaj < lMaj) return false;

    if (rMin > lMin) return true;
    if (rMin < lMin) return false;

    return (rPatch > lPatch);
}

bool checkAndApplyGithubOta() {
    if (WiFi.status() != WL_CONNECTED) {
        log_w("[GitHub OTA] Wi-Fi not connected. Skipping update check.");
        return false;
    }

    log_i("[GitHub OTA] Checking for firmware updates from GitHub: %s", GITHUB_VERSION_URL);

    WiFiClientSecure client;
    client.setInsecure(); // Disable TLS validation for IoT microcontroller

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(8000); // 8-second timeout

    if (!http.begin(client, GITHUB_VERSION_URL)) {
        log_w("[GitHub OTA] Failed to connect to version URL.");
        return false;
    }

    http.addHeader("User-Agent", "ESP32-EinkClock/" FIRMWARE_VERSION);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        log_w("[GitHub OTA] HTTP GET returned code %d (%s)", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    log_i("[GitHub OTA] Version JSON: %s", payload.c_str());

    // Extract "version" value
    int vIdx = payload.indexOf("\"version\"");
    if (vIdx < 0) {
        log_w("[GitHub OTA] Could not find 'version' field in response.");
        return false;
    }

    int colonIdx = payload.indexOf(':', vIdx);
    int q1 = payload.indexOf('"', colonIdx);
    int q2 = payload.indexOf('"', q1 + 1);
    if (q1 < 0 || q2 <= q1) {
        log_w("[GitHub OTA] Malformed version string in JSON.");
        return false;
    }

    String remoteVersion = payload.substring(q1 + 1, q2);
    remoteVersion.trim();

    // Check optional custom "url" field
    String firmwareUrl = GITHUB_FIRMWARE_URL;
    int uIdx = payload.indexOf("\"url\"");
    if (uIdx >= 0) {
        int uColon = payload.indexOf(':', uIdx);
        int uQ1 = payload.indexOf('"', uColon);
        int uQ2 = payload.indexOf('"', uQ1 + 1);
        if (uQ1 >= 0 && uQ2 > uQ1) {
            firmwareUrl = payload.substring(uQ1 + 1, uQ2);
            firmwareUrl.trim();
        }
    }

    log_i("[GitHub OTA] Installed Version: v%s | Remote Version: v%s", FIRMWARE_VERSION, remoteVersion.c_str());

    if (!isVersionNewer(remoteVersion.c_str(), FIRMWARE_VERSION)) {
        log_i("[GitHub OTA] Firmware is up to date (v%s).", FIRMWARE_VERSION);
        return false;
    }

    log_i("=================================================");
    log_i("[GitHub OTA] NEW VERSION DETECTED: v%s", remoteVersion.c_str());
    log_i("[GitHub OTA] Downloading binary from: %s", firmwareUrl.c_str());
    log_i("=================================================");

    // Render clean full-screen update dialog on e-paper screen (full hardware init wipes ghosting)
    char msg[96];
    snprintf(msg, sizeof(msg), "Downloading v%s from GitHub Releases...", remoteVersion.c_str());
    showOtaScreen("GITHUB OTA UPDATE", msg);

    // Setup HTTPUpdate with redirect support
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.rebootOnUpdate(true);

    httpUpdate.onProgress([](int cur, int total) {
        static uint32_t lastDisplayUpdateMs = 0;
        static int lastLoggedPct = -1;
        int pct = (total > 0) ? (cur * 100) / total : 0;
        if (pct > 100) pct = 100;

        if (pct % 10 == 0 && pct != lastLoggedPct) {
            log_i("[GitHub OTA] Downloading: %d%% (%d / %d bytes)", pct, cur, total);
            lastLoggedPct = pct;
        }

        uint32_t now = millis();
        if (now - lastDisplayUpdateMs >= 2000 || pct == 100) {
            lastDisplayUpdateMs = now;
            updateOtaProgress(pct, (uint32_t)cur, (uint32_t)total);
        }
    });

    // Stream download and flash directly into alternate OTA partition
    t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

    if (ret == HTTP_UPDATE_FAILED) {
        log_e("[GitHub OTA] Update failed! Error (%d): %s", 
              httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        return false;
    } else if (ret == HTTP_UPDATE_NO_UPDATES) {
        log_i("[GitHub OTA] No updates available.");
        return false;
    } else if (ret == HTTP_UPDATE_OK) {
        log_i("[GitHub OTA] Update completed successfully! Rebooting clock...");
        return true;
    }

    return false;
}

#pragma once

#include <Arduino.h>
#include "config.h"

// Initialize OTA services (ArduinoOTA, mDNS, WebServer)
void initOtaServices();

// Run the OTA update window for a specified duration (in seconds).
// Blocks while servicing ArduinoOTA and WebServer until timeout or until flash completes/reboots.
// If an upload starts, timeout is canceled and it stays alive until flashing finishes.
// Returns true if update was applied, false if timed out without update.
bool runOtaWindow(uint32_t timeoutSeconds);

// Check if an OTA transfer is currently active
bool isOtaInProgress();

// Get the local IP address as a String
String getOtaIpAddress();

// Get the OTA Web URL (e.g. "http://192.168.1.98/update")
String getOtaWebUrl();

// Check GitHub Releases for new firmware and perform update if available.
// Returns true if update started (will reboot on completion), false if up to date or failed.
bool checkAndApplyGithubOta();

// Compare semantic versions (e.g. "1.0.1" > "1.0.0")
bool isVersionNewer(const char* remoteVer, const char* localVer);

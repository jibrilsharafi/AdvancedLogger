/*
 * File: basicServer.ino
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 21/03/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example covers the addition of a simple web server to the basicUsage, which allows
 * the user to explore the log and configuration files remotely.
 * 
 * All the other advanced usage features are reported in the basicUsage example.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "AdvancedLogger.h"

const char *customLogPath = "/customPath/log.txt";

AsyncWebServer server(80);

const int timeZone = 0; // UTC. In milliseconds
const int daylightOffset = 0; // No daylight saving time. In milliseconds
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.windows.com";

// **** CHANGE THESE TO YOUR SSID AND PASSWORD ****
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

static const char* TAG = "main";

void setup()
{
    // Initialize Serial and LittleFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    AdvancedLogger::begin(customLogPath);

    AdvancedLogger::debug("AdvancedLogger setup done!", TAG);
    
    // Connect to WiFi
    // --------------------
    // Connect to the specified SSID
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        AdvancedLogger::info("Connecting to WiFi... SSID: %s | Password: ***", TAG, ssid);
    }
    
    AdvancedLogger::info(("IP address: " + WiFi.localIP().toString()).c_str(), TAG);

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);

    // Serve a simple webpage with a button that sends the user to the page /log and /config
    // --------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<button onclick=\"window.location.href='/log'\">Explore the logs</button>"); });
    
    server.serveStatic("/log", LittleFS, customLogPath);
    
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });
    server.begin();
    
    AdvancedLogger::info("Server started!", TAG);

    AdvancedLogger::info("Setup done!", TAG);
}

void loop()
{
    AdvancedLogger::debug("This is a debug message!", TAG);
    delay(500);
    AdvancedLogger::info("This is an info message!!", TAG);
    delay(500);
    AdvancedLogger::warning("This is a warning message!!!", TAG);
    delay(500);
    AdvancedLogger::error("This is a error message!!!!", TAG);
    delay(500);
    AdvancedLogger::fatal("This is a fatal message!!!!!", TAG);
    delay(500);
    AdvancedLogger::info("This is an info message!!", TAG, true);
    delay(1000);;

    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        AdvancedLogger::info("Current number of log lines: %d", TAG, AdvancedLogger::getLogLines());
        AdvancedLogger::clearLog();
        AdvancedLogger::setDefaultConfig();
        AdvancedLogger::warning("Log cleared!", TAG);

        lastMillisLogClear = millis();
    }
}
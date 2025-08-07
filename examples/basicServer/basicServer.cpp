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
const char *ssid = "SSID";
const char *password = "PASSWORD";

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

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

    LOG_DEBUG("AdvancedLogger setup done!");
    
    // Connect to WiFi
    // --------------------
    // Connect to the specified SSID
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        LOG_INFO("Connecting to WiFi... SSID: %s | Password: ***", ssid);
    }

    LOG_INFO(("IP address: " + WiFi.localIP().toString()).c_str());

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);

    // Serve a simple webpage with a button that sends the user to the page /log and /config
    // --------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<button onclick=\"window.location.href='/log'\">Explore the logs</button>"); });
    
    server.serveStatic("/log", LittleFS, customLogPath);
    
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });
    server.begin();
    
    LOG_DEBUG("Server started!");

    LOG_INFO("Setup done!");
}

void loop()
{
    LOG_DEBUG("This is a debug message!");
    delay(500);
    LOG_INFO("This is an info message!!");
    delay(500);
    LOG_WARNING("This is a warning message!!!");
    delay(500);
    LOG_ERROR("This is a error message!!!!");
    delay(500);
    LOG_FATAL("This is a fatal message!!!!!");
    delay(500);
    LOG_INFO("This is an info message!!", true);
    delay(1000);

    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        LOG_INFO("Current number of log lines: %d", AdvancedLogger::getLogLines());
        AdvancedLogger::clearLog();
        AdvancedLogger::setDefaultConfig();
        LOG_WARNING("Log cleared!");

        lastMillisLogClear = millis();
    }
}
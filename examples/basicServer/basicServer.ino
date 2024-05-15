/*
 * File: basicServer.ino
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 07/04/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example is a simple web server that serves a webpage with a button
 * that sends the user to the /log page, where the logs are displayed, and
 * another button that sends the user to the /config page, where the configuration
 * is displayed.
 *
 * Remember to change the ssid and password to your own.
 *
 * Tested only on the ESP32. For other boards, you may need to change the
 * WebServer library.
 *
 * Possible logging levels:
 * - ADVANCEDLOGGER_VERBOSE
 * - ADVANCEDLOGGER_DEBUG
 * - ADVANCEDLOGGER_INFO
 * - ADVANCEDLOGGER_WARNING
 * - ADVANCEDLOGGER_ERROR
 * - ADVANCEDLOGGER_FATAL
 */
#include <Arduino.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "AdvancedLogger.h"

String customLogPath = "/customPath/log.txt";
String customConfigPath = "/customPath/config.txt";
String customTimestampFormat = "%A %B %Y | %H:%M:%S";

AdvancedLogger logger(SPIFFS, customLogPath.c_str(), customConfigPath.c_str(), customTimestampFormat.c_str()); // Leave empty for default paths

AsyncWebServer server(80);

String printLevel;
String saveLevel;

long lastMillisLogDump = 0;
const long intervalLogDump = 10000;

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

int maxLogLines = 100; // Low value for testing purposes

// **** CHANGE THESE TO YOUR SSID AND PASSWORD ****
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

void setup()
{
    // Initialize Serial and SPIFFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) // Setting to true will format the SPIFFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    logger.begin();

    // Setting the print and save levels is not mandatory.
    // If you don't set them, the default levels are first taken
    // from the SPIFFS file, and if it doesn't exist, the default
    // levels are used (DEBUG for print and INFO for save).
    logger.setPrintLevel(ADVANCEDLOGGER_DEBUG);
    logger.setSaveLevel(ADVANCEDLOGGER_INFO);
    // Set the maximum number of log lines before the log is cleared
    // If you don't set this, the default is used
    logger.setMaxLogLines(maxLogLines);
    logger.debug("AdvancedLogger setup done!", "basicServer::setup");
    
    // Connect to WiFi
    // --------------------
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        logger.info("Connecting to WiFi..", "basicServer::setup");
    }
    logger.info(("IP address: " + WiFi.localIP().toString()).c_str(), "basicServer::setup");

    // Serve a simple webpage with a button that sends the user to the page /log and /config
    // --------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<button onclick=\"window.location.href='/log'\">Explore the logs</button><br><br><button onclick=\"window.location.href='/config'\">Explore the configuration</button>"); });
    server.serveStatic("/log", SPIFFS, customLogPath.c_str());
    server.serveStatic("/config", SPIFFS, customConfigPath.c_str());
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });
    server.begin();
    logger.info("Server started!", "basicServer::setup");

    lastMillisLogDump = millis();
    lastMillisLogClear = millis();
    logger.info("Setup done!", "basicServer::setup");
}

void loop()
{
    logger.debug("This is a debug message!", "basicServer::loop");
    delay(500);
    logger.info("This is an info message!!", "basicServer::loop");
    delay(500);
    logger.warning("This is a warning message!!!", "basicServer::loop");
    delay(500);
    logger.error("This is a error message!!!!", "basicServer::loop");
    delay(500);
    logger.fatal("This is a fatal message!!!!!", "basicServer::loop");
    delay(500);
    logger.info("This is an info message!!", "basicServer::loop", true);
    delay(1000);

    printLevel = logger.getPrintLevel();
    saveLevel = logger.getSaveLevel();

    if (millis() - lastMillisLogDump > intervalLogDump)
    {
        logger.dumpToSerial();

        lastMillisLogDump = millis();
    }
    
    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        logger.info(
            ("Current number of log lines: " + String(logger.getLogLines())).c_str(),
            "basicServer::loop"
        );
        logger.clearLog();
        logger.setDefaultLogLevels();
        logger.warning("Log cleared!", "basicServer::loop");

        lastMillisLogClear = millis();
    }
}

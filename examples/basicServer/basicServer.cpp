/*
 * File: basicServer.cpp
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

#include "advancedLogger.h"

AdvancedLogger logger;

AsyncWebServer server(80);

String printLevel;
String saveLevel;

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
    logger.log("AdvancedLogger setup done!", "basicServer::setup", ADVANCEDLOGGER_INFO);

    // Connect to WiFi
    // --------------------
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        logger.log("Connecting to WiFi..", "basicServer::setup", ADVANCEDLOGGER_INFO);
    }
    logger.log(("IP address: " + WiFi.localIP().toString()).c_str(), "basicServer::setup", ADVANCEDLOGGER_INFO);

    // Serve a simple webpage with a button that sends the user to the page /log and /config
    // --------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<button onclick=\"window.location.href='/log'\">Explore the logs</button><br><br><button onclick=\"window.location.href='/config'\">Explore the configuration</button>"); });
    server.serveStatic("/log", SPIFFS, "/AdvancedLogger/log.txt");
    server.serveStatic("/config", SPIFFS, "/AdvancedLogger/config.json");
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not found"); });
    server.begin();
    logger.log("Server started!", "basicServer::setup", ADVANCEDLOGGER_INFO);

    logger.log("Setup done!", "basicServer::setup", ADVANCEDLOGGER_INFO);
}

void loop()
{
    logger.log("This is an debug message!", "basicServer::loop", ADVANCEDLOGGER_DEBUG);
    logger.log("This is an info message!!", "basicServer::loop", ADVANCEDLOGGER_INFO);
    logger.log("This is an warning message!!!", "basicServer::loop", ADVANCEDLOGGER_WARNING);
    logger.log("This is an error message!!!!", "basicServer::loop", ADVANCEDLOGGER_ERROR);
    logger.log("This is an fatal message!!!!!", "basicServer::loop", ADVANCEDLOGGER_FATAL);
    delay(1000);
    logger.logOnly("This is an info message (logOnly)!!", "basicServer::loop", ADVANCEDLOGGER_INFO);

    printLevel = logger.getPrintLevel();
    saveLevel = logger.getSaveLevel();

    if (millis() > 60000)
    {
        logger.clearLog();
        logger.setDefaultLogLevels();
        logger.log("Log cleared!", "basicServer::loop", ADVANCEDLOGGER_WARNING);
    }
}
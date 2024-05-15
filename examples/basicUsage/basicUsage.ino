/*
 * File: basicUsage.ino
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 12/05/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * The AdvancedLogger library provides advanced logging for the ESP32.
 * It allows you to log messages to the console and to a file on the SPIFFS.
 * You can set the print level and save level, and the library will only log
 * messages accordingly.
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

#include "AdvancedLogger.h"

String customLogPath = "/customPath/log.txt";
String customConfigPath = "/customPath/config.txt";
String customTimestampFormat = "%A %B %Y | %H:%M:%S";

AdvancedLogger logger(SPIFFS, customLogPath.c_str(), customConfigPath.c_str(), customTimestampFormat.c_str()); // Leave empty for default paths

String printLevel;
String saveLevel;

long lastMillisLogDump = 0;
const long intervalLogDump = 10000;

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

int maxLogLines = 10; // Low value for testing purposes

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
    logger.debug("AdvancedLogger setup done!", "basicUsage::setup");
   
    lastMillisLogDump = millis();
    lastMillisLogClear = millis();
    logger.info("Setup done!", "basicUsage::setup");
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
    logger.info("This is an info message (logOnly)!!", "basicServer::loop", true);
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
    }
}

/*
 * File: basicUsage.cpp
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 11/05/2024
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

#include "advancedLogger.h"

AdvancedLogger logger;

String printLevel;
String saveLevel;

long lastMillisLogDump = 0;
const long intervalLogDump = 10000;

long lastMillisLogClear = 0;
const long intervalLogClear = 30000;

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

    lastMillisLogDump = millis();
    lastMillisLogClear = millis();
    logger.log("Setup done!", "basicUsage::setup", ADVANCEDLOGGER_INFO);
}

void loop()
{
    logger.log("This is an debug message!", "basicUsage::loop", ADVANCEDLOGGER_DEBUG);
    logger.log("This is an info message!!", "basicUsage::loop", ADVANCEDLOGGER_INFO);
    logger.log("This is an warning message!!!", "basicUsage::loop", ADVANCEDLOGGER_WARNING);
    logger.log("This is an error message!!!!", "basicUsage::loop", ADVANCEDLOGGER_ERROR);
    logger.log("This is an fatal message!!!!!", "basicUsage::loop", ADVANCEDLOGGER_FATAL);
    delay(1000);
    logger.logOnly("This is an info message (logOnly)!!", "basicUsage::loop", ADVANCEDLOGGER_INFO);

    printLevel = logger.getPrintLevel();
    saveLevel = logger.getSaveLevel();

    if (millis() - lastMillisLogDump > intervalLogDump)
    {
        logger.dumpToSerial();

        lastMillisLogDump = millis();
    }
    
    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        logger.dumpToSerial();
        logger.clearLog();
        logger.setDefaultLogLevels();
        logger.log("Log cleared!", "basicServer::loop", ADVANCEDLOGGER_WARNING);

        lastMillisLogClear = millis();
    }
}
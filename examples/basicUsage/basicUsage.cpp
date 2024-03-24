/*
 * File: basicUsage.cpp
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 21/03/2024
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
 */
#include <Arduino.h>
#include <SPIFFS.h>

#include "advancedLogger.h"

AdvancedLogger logger;

String printLevel;
String saveLevel;

void setup()
{
    Serial.begin(9600);

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    logger.begin();

    logger.setPrintLevel(ADVANCEDLOGGER_VERBOSE);
    logger.setSaveLevel(ADVANCEDLOGGER_INFO);

    logger.log("Setup done!", "simpleExample::setup", ADVANCEDLOGGER_INFO);
}

void loop()
{
    logger.log("This is an debug message!", "simpleExample::loop", ADVANCEDLOGGER_VERBOSE);
    logger.log("This is an info message!!", "simpleExample::loop", ADVANCEDLOGGER_INFO);
    logger.log("This is an warning message!!!", "simpleExample::loop", ADVANCEDLOGGER_WARNING);
    logger.log("This is an error message!!!!", "simpleExample::loop", ADVANCEDLOGGER_ERROR);
    logger.log("This is an fatal message!!!!!", "simpleExample::loop", ADVANCEDLOGGER_FATAL);
    delay(1000);
    logger.logOnly("This is an info message (logOnly)!!", "simpleExample::loop", ADVANCEDLOGGER_INFO);

    printLevel = logger.getPrintLevel();
    saveLevel = logger.getSaveLevel();

    if (millis() > 60000)
    {
        logger.clearLog();
        logger.setDefaultLogLevels();
        logger.log("Log cleared!", "simpleExample::loop", ADVANCEDLOGGER_WARNING);
    }
}
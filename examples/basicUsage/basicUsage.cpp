/*
 * File: basicUsage.ino
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 21/03/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example covers the basic usage of the AdvancedLogger library:
 * - Initializing the logger
 * - Setting the print and save levels
 * - Setting the maximum number of log lines before the log is cleared
 * - Logging messages
 * - Dumping the log
 * - Clearing the log
 * - Getting the current print and save levels
 * - Getting the current number of log lines
 * - Setting the default configuration
 */

#include <Arduino.h>
#include <LittleFS.h>

#include "AdvancedLogger.h"

const char *customLogPath = "/customPath/log.txt";

// Set the custom print and save levels
LogLevel printLevel = LogLevel::INFO;
LogLevel saveLevel = LogLevel::WARNING;

// Set the maximum number of log lines before the log is cleared
int maxLogLines = 100; // Low value for testing purposes

// Variables for logging and clearing the log
long lastMillisLogDump = 0;
const long intervalLogDump = 10000;
const char *logDumpPath = "/logDump.txt";

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

    // Initialize the logger
    AdvancedLogger::begin(customLogPath);
    
    // Setting the print and save levels (optional)
    AdvancedLogger::setPrintLevel(printLevel);
    AdvancedLogger::setSaveLevel(saveLevel);

    // Set the maximum number of log lines before the log is cleared (optional)
    AdvancedLogger::setMaxLogLines(maxLogLines);

    AdvancedLogger::debug("AdvancedLogger setup done!", TAG);
   
    lastMillisLogDump = millis();
    lastMillisLogClear = millis();

    AdvancedLogger::info("Setup done!", TAG);
}

void loop()
{
    AdvancedLogger::verbose("This is a verbose message", TAG);
    delay(500);
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

    AdvancedLogger::info("Testing printf functionality: %d, %f, %s", TAG, 1, 2.0, "three");
    delay(500);
    
    // Get the current print and save levels
    String printLevel = AdvancedLogger::logLevelToString(AdvancedLogger::getPrintLevel());
    String saveLevel = AdvancedLogger::logLevelToString(AdvancedLogger::getSaveLevel());

    if (millis() - lastMillisLogDump > intervalLogDump)
    {
        // Print the current number of log lines
        AdvancedLogger::info("Current number of log lines: %d", TAG, AdvancedLogger::getLogLines());

        // Dump the log to Serial
        AdvancedLogger::info("Dumping log to Serial...", TAG);
        AdvancedLogger::dump(Serial);
        AdvancedLogger::info("Log dumped!", TAG);

        // Dump the log to another file
        AdvancedLogger::info("Dumping log to file...", TAG);
        File tempFile = LittleFS.open(logDumpPath, "w");
        AdvancedLogger::dump(tempFile);
        tempFile.close();
        AdvancedLogger::info("Log dumped!", TAG);

        // Ensure the log has been dumped correctly
        AdvancedLogger::info("Printing the temporary log dump file...", TAG);
        tempFile = LittleFS.open(logDumpPath, "r");
        while (tempFile.available())
        {
            Serial.write(tempFile.read());
        }
        tempFile.close();
        AdvancedLogger::info("Log dump file printed!", TAG);

        lastMillisLogDump = millis();
    }

    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        // Clear the log and set the default configuration
        AdvancedLogger::clearLogKeepLatestXPercent(50);
        // If you want to clear the log without keeping the latest X percent of the log, use:
        // AdvancedLogger::clearLog();
        AdvancedLogger::setDefaultConfig();

        AdvancedLogger::info("Log cleared and default configuration set!", TAG);

        lastMillisLogClear = millis();
    }
}
/*
 * File: basicUsage.ino
 * --------------------
 * This file provides a simple example to show how to use the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 21/03/2024
 * Last modified: 24/08/2024
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
#include <SPIFFS.h>

#include "AdvancedLogger.h"

const char *customLogPath = "/customPath/log.txt";
const char *customConfigPath = "/customPath/config.txt";
// For more info on formatting, see https://www.cplusplus.com/reference/ctime/strftime/
const char *customTimestampFormat = "%Y-%m-%d %H:%M:%S"; 

AdvancedLogger logger(
    customLogPath,
    customConfigPath,
    customTimestampFormat);
// If you don't want to set custom paths and timestamp format, you can 
// just use the default constructor:
// AdvancedLogger logger;

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
    // Initialize Serial and SPIFFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) // Setting to true will format the SPIFFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    // Initialize the logger
    logger.begin();
    
    // Setting the print and save levels (optional)
    logger.setPrintLevel(printLevel);
    logger.setSaveLevel(saveLevel);

    // Set the maximum number of log lines before the log is cleared (optional)
    logger.setMaxLogLines(maxLogLines);

    logger.debug("AdvancedLogger setup done!", TAG);
   
    lastMillisLogDump = millis();
    lastMillisLogClear = millis();

    logger.info("Setup done!", TAG);
}

void loop()
{
    logger.verbose("This is a verbose message", TAG);
    delay(500);
    logger.debug("This is a debug message!", TAG);
    delay(500);
    logger.info("This is an info message!!", TAG);
    delay(500);
    logger.warning("This is a warning message!!!", TAG);
    delay(500);
    logger.error("This is a error message!!!!", TAG);
    delay(500);
    logger.fatal("This is a fatal message!!!!!", TAG);
    delay(500);

    logger.info("Testing printf functionality: %d, %f, %s", TAG, 1, 2.0, "three");
    delay(500);
    
    // Get the current print and save levels
    String printLevel = logger.logLevelToString(logger.getPrintLevel());
    String saveLevel = logger.logLevelToString(logger.getSaveLevel());

    if (millis() - lastMillisLogDump > intervalLogDump)
    {
        // Print the current number of log lines
        logger.info("Current number of log lines: %d", TAG, logger.getLogLines());

        // Dump the log to Serial
        logger.info("Dumping log to Serial...", TAG);
        logger.dump(Serial);
        logger.info("Log dumped!", TAG);

        // Dump the log to another file
        logger.info("Dumping log to file...", TAG);
        File tempFile = SPIFFS.open(logDumpPath, "w");
        logger.dump(tempFile);
        tempFile.close();
        logger.info("Log dumped!", TAG);

        // Ensure the log has been dumped correctly
        logger.info("Printing the temporary log dump file...", TAG);
        tempFile = SPIFFS.open(logDumpPath, "r");
        while (tempFile.available())
        {
            Serial.write(tempFile.read());
        }
        tempFile.close();
        logger.info("Log dump file printed!", TAG);

        lastMillisLogDump = millis();
    }

    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        // Clear the log and set the default configuration
        logger.clearLogKeepLatestXPercent(50);
        // If you want to clear the log without keeping the latest X percent of the log, use:
        // logger.clearLog();
        logger.setDefaultConfig();

        logger.info("Log cleared and default configuration set!", TAG);

        lastMillisLogClear = millis();
    }
}
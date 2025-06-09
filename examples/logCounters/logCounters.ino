/*
 * File: logCounters.ino
 * ---------------------
 * This file demonstrates the log counter functionality of the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 08/06/2025
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example covers the log counter features:
 * - Tracking the number of logs per level
 * - Getting individual log level counts
 * - Getting total log count
 * - Resetting log counters
 */

#include <Arduino.h>
#include <SPIFFS.h>

#include "AdvancedLogger.h"

AdvancedLogger logger;

static const char* TAG = "main";

void setup()
{
    // Initialize Serial and SPIFFS (mandatory for the AdvancedLogger library)
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) // Setting to true will format the SPIFFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    // Initialize the logger
    logger.begin();
    
    // Set print level to VERBOSE to see all messages
    logger.setPrintLevel(LogLevel::DEBUG);
    
    logger.info("Log counter example started!", TAG);
}

void loop()
{
    // Generate some logs of different levels
    logger.verbose("This is a verbose message", TAG);
    delay(100);
    
    logger.debug("This is a debug message", TAG);
    delay(100);
    
    logger.info("This is an info message", TAG);
    delay(100);
    
    logger.warning("This is a warning message", TAG);
    delay(100);
    
    logger.error("This is an error message", TAG);
    delay(100);

    // Set 5 random logs
    for (int i = 0; i < 5; ++i) {
        int randomLevel = random(0, 6); // Random log level from 0 to 5
        switch (randomLevel) {
            case 0: logger.verbose("Random verbose log %d", TAG, i); break;
            case 1: logger.debug("Random debug log %d", TAG, i); break;
            case 2: logger.info("Random info log %d", TAG, i); break;
            case 3: logger.warning("Random warning log %d", TAG, i); break;
            case 4: logger.error("Random error log %d", TAG, i); break;
            case 5: logger.fatal("Random fatal log %d", TAG, i); break;
        }
        delay(200);
    }

    // Try a burst of 1000 verbose logs and time it
    unsigned long startTime = millis();
    logger.info("Starting burst of 1000 verbose logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        logger.verbose("Burst verbose log %d", TAG, i);
    }
    unsigned long elapsedTime = millis() - startTime;
    logger.info("Burst of 1000 verbose logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 debug logs and time it
    startTime = millis();
    logger.info("Starting burst of 1000 debug logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        logger.debug("Burst debug log %d", TAG, i);
    }
    elapsedTime = millis() - startTime;
    logger.info("Burst of 1000 debug logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 fatal logs and time it
    startTime = millis();
    logger.info("Starting burst of 1000 fatal logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        logger.fatal("Burst fatal log %d", TAG, i);
    }
    elapsedTime = millis() - startTime;
    logger.info("Burst of 1000 fatal logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Every 10 seconds, display the log counters
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay > 10000) {
        lastDisplay = millis();
        
        logger.info("=== LOG STATISTICS ===", TAG);
        logger.info("Verbose logs: %lu", TAG, logger.getVerboseCount());
        logger.info("Debug logs: %lu", TAG, logger.getDebugCount());
        logger.info("Info logs: %lu", TAG, logger.getInfoCount());
        logger.info("Warning logs: %lu", TAG, logger.getWarningCount());
        logger.info("Error logs: %lu", TAG, logger.getErrorCount());
        logger.info("Fatal logs: %lu", TAG, logger.getFatalCount());
        logger.info("Total logs: %lu", TAG, logger.getTotalLogCount());
        logger.info("====================", TAG);
    }
    
    // Every 30 seconds, reset counters
    static unsigned long lastReset = 0;
    if (millis() - lastReset > 30000) {
        lastReset = millis();
        
        logger.warning("Resetting log counters...", TAG);
        logger.resetLogCounters();
        logger.info("Log counters reset! Starting fresh count.", TAG);
    }
    
    delay(1000);
}

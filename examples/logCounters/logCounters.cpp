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
#include <LittleFS.h>

#include "AdvancedLogger.h"

static const char* TAG = "main";

void setup()
{
    // Initialize Serial and LittleFS (mandatory for the AdvancedLogger library)
    Serial.begin(115200);

    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    // Initialize the logger
    AdvancedLogger::begin();
    
    // Set print level to VERBOSE to see all messages
    AdvancedLogger::setPrintLevel(LogLevel::DEBUG);
    
    AdvancedLogger::info("Log counter example started!", TAG);
}

void loop()
{
    // Generate some logs of different levels
    AdvancedLogger::verbose("This is a verbose message", TAG);
    delay(100);
    
    AdvancedLogger::debug("This is a debug message", TAG);
    delay(100);
    
    AdvancedLogger::info("This is an info message", TAG);
    delay(100);
    
    AdvancedLogger::warning("This is a warning message", TAG);
    delay(100);
    
    AdvancedLogger::error("This is an error message", TAG);
    delay(100);

    // Set 5 random logs
    for (int i = 0; i < 5; ++i) {
        int randomLevel = random(0, 6); // Random log level from 0 to 5
        switch (randomLevel) {
            case 0: AdvancedLogger::verbose("Random verbose log %d", TAG, i); break;
            case 1: AdvancedLogger::debug("Random debug log %d", TAG, i); break;
            case 2: AdvancedLogger::info("Random info log %d", TAG, i); break;
            case 3: AdvancedLogger::warning("Random warning log %d", TAG, i); break;
            case 4: AdvancedLogger::error("Random error log %d", TAG, i); break;
            case 5: AdvancedLogger::fatal("Random fatal log %d", TAG, i); break;
        }
        delay(200);
    }

    // Try a burst of 1000 verbose logs and time it
    unsigned long startTime = millis();
    AdvancedLogger::info("Starting burst of 1000 verbose logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        AdvancedLogger::verbose("Burst verbose log %d", TAG, i);
    }
    unsigned long elapsedTime = millis() - startTime;
    AdvancedLogger::info("Burst of 1000 verbose logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 debug logs and time it
    startTime = millis();
    AdvancedLogger::info("Starting burst of 1000 debug logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        AdvancedLogger::debug("Burst debug log %d", TAG, i);
    }
    elapsedTime = millis() - startTime;
    AdvancedLogger::info("Burst of 1000 debug logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 fatal logs and time it
    startTime = millis();
    AdvancedLogger::info("Starting burst of 1000 fatal logs...", TAG);
    for (int i = 0; i < 1000; ++i) {
        AdvancedLogger::fatal("Burst fatal log %d", TAG, i);
    }
    elapsedTime = millis() - startTime;
    AdvancedLogger::info("Burst of 1000 fatal logs completed in %lu ms (average %.2f us per log)", 
                TAG, elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Every 10 seconds, display the log counters
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay > 10000) {
        lastDisplay = millis();
        
        AdvancedLogger::info("=== LOG STATISTICS ===", TAG);
        AdvancedLogger::info("Verbose logs: %lu", TAG, AdvancedLogger::getVerboseCount());
        AdvancedLogger::info("Debug logs: %lu", TAG, AdvancedLogger::getDebugCount());
        AdvancedLogger::info("Info logs: %lu", TAG, AdvancedLogger::getInfoCount());
        AdvancedLogger::info("Warning logs: %lu", TAG, AdvancedLogger::getWarningCount());
        AdvancedLogger::info("Error logs: %lu", TAG, AdvancedLogger::getErrorCount());
        AdvancedLogger::info("Fatal logs: %lu", TAG, AdvancedLogger::getFatalCount());
        AdvancedLogger::info("Total logs: %lu", TAG, AdvancedLogger::getTotalLogCount());
        AdvancedLogger::info("====================", TAG);
    }
    
    // Every 30 seconds, reset counters
    static unsigned long lastReset = 0;
    if (millis() - lastReset > 30000) {
        lastReset = millis();
        
        AdvancedLogger::warning("Resetting log counters...", TAG);
        AdvancedLogger::resetLogCounters();
        AdvancedLogger::info("Log counters reset! Starting fresh count.", TAG);
    }
    
    delay(1000);
}

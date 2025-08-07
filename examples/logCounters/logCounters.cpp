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
    
    LOG_INFO("Log counter example started!");
}

void loop()
{
    // Generate some logs of different levels
    LOG_VERBOSE("This is a verbose message");
    delay(100);
    
    LOG_DEBUG("This is a debug message");
    delay(100);
    
    LOG_INFO("This is an info message");
    delay(100);
    
    LOG_WARNING("This is a warning message");
    delay(100);
    
    LOG_ERROR("This is an error message");
    delay(100);

    // Set 5 random logs
    for (int i = 0; i < 5; ++i) {
        int randomLevel = random(0, 6); // Random log level from 0 to 5
        switch (randomLevel) {
            case 0: LOG_VERBOSE("Random verbose log %d", i); break;
            case 1: LOG_DEBUG("Random debug log %d", i); break;
            case 2: LOG_INFO("Random info log %d", i); break;
            case 3: LOG_WARNING("Random warning log %d", i); break;
            case 4: LOG_ERROR("Random error log %d", i); break;
            case 5: LOG_FATAL("Random fatal log %d", i); break;
        }
        delay(200);
    }

    // Try a burst of 1000 verbose logs and time it
    unsigned long startTime = millis();
    LOG_INFO("Starting burst of 1000 verbose logs...");
    for (int i = 0; i < 1000; ++i) {
        LOG_VERBOSE("Burst verbose log %d", i);
    }
    unsigned long elapsedTime = millis() - startTime;
    LOG_INFO("Burst of 1000 verbose logs completed in %lu ms (average %.2f us per log)", 
                elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 debug logs and time it
    startTime = millis();
    LOG_INFO("Starting burst of 1000 debug logs...");
    for (int i = 0; i < 1000; ++i) {
        LOG_DEBUG("Burst debug log %d", i);
    }
    elapsedTime = millis() - startTime;
    LOG_INFO("Burst of 1000 debug logs completed in %lu ms (average %.2f us per log)", 
                elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Try a burst of 1000 fatal logs and time it
    startTime = millis();
    LOG_INFO("Starting burst of 1000 fatal logs...");
    for (int i = 0; i < 1000; ++i) {
        LOG_FATAL("Burst fatal log %d", i);
    }
    elapsedTime = millis() - startTime;
    LOG_INFO("Burst of 1000 fatal logs completed in %lu ms (average %.2f us per log)", 
                elapsedTime, elapsedTime / 1000.0 * 1000.0);

    delay(10000);

    // Every 10 seconds, display the log counters
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay > 10000) {
        lastDisplay = millis();
        
        LOG_INFO("=== LOG STATISTICS ===");
        LOG_INFO("Verbose logs: %lu", AdvancedLogger::getVerboseCount());
        LOG_INFO("Debug logs: %lu", AdvancedLogger::getDebugCount());
        LOG_INFO("Info logs: %lu", AdvancedLogger::getInfoCount());
        LOG_INFO("Warning logs: %lu", AdvancedLogger::getWarningCount());
        LOG_INFO("Error logs: %lu", AdvancedLogger::getErrorCount());
        LOG_INFO("Fatal logs: %lu", AdvancedLogger::getFatalCount());
        LOG_INFO("Total logs: %lu", AdvancedLogger::getTotalLogCount());
        LOG_INFO("====================");
    }
    
    // Every 30 seconds, reset counters
    static unsigned long lastReset = 0;
    if (millis() - lastReset > 30000) {
        lastReset = millis();
        
        LOG_WARNING("Resetting log counters...");
        AdvancedLogger::resetLogCounters();
        LOG_INFO("Log counters reset! Starting fresh count.");
    }
    
    delay(1000);
}

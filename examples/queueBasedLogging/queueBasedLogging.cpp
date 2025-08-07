/*
 * File: queueBasedLoggingExample.cpp
 * ---------------------
 * This file demonstrates the queue-based logging functionality of the AdvancedLogger library.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 07/08/2025
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example covers the log queue features:
 * - Initializing the logger with queue-based logging
 * - Rapid logging without blocking the main thread
 * - Checking queue status (available spaces, messages waiting, dropped messages)
 * - Demonstrating the efficiency of queue-based logging for high-frequency log generation
 */

#include <Arduino.h>
#include <LittleFS.h>

#include "AdvancedLogger.h"

// Optional: Configure queue parameters before including the library
// #define ADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE 12000 // Amount of heap memory allocated for the log queue. The queue size is calculated based on this value.
// #define ADVANCED_LOGGER_TASK_STACK_SIZE 4096 // Stack size for the log processing task.
// #define ADVANCED_LOGGER_TASK_PRIORITY 2 // Priority for the log processing task.
// #define ADVANCED_LOGGER_TASK_CORE 1 // Core ID for the log processing task.
// #define ADVANCED_LOGGER_MAX_MESSAGE_LENGTH 512 // Maximum length of log messages.

void setup() {
    Serial.begin(115200);
    delay(1000);
        
    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }
    
    // Initialize the advanced logger with queue-based logging
    AdvancedLogger::begin("/logs/app.log");
    
    LOG_INFO("AdvancedLogger with queue-based logging started");
    LOG_DEBUG("Queue spaces available: %lu", AdvancedLogger::getQueueSpacesAvailable());
    LOG_DEBUG("Queue messages waiting: %lu", AdvancedLogger::getQueueMessagesWaiting());
    
    // Demonstrate rapid logging that would benefit from queue-based processing
    LOG_INFO("Starting rapid logging test...");
    
    unsigned long startTime = millis();
    for (int i = 0; i < 100; i++) {
        LOG_VERBOSE("Rapid log message #%d - timestamp: %lu", i, millis());
        LOG_DEBUG("Rapid log message #%d - timestamp: %lu", i, millis());
        LOG_INFO("Rapid log message #%d - timestamp: %lu", i, millis());
        LOG_WARNING("Rapid log message #%d - timestamp: %lu", i, millis());
        LOG_ERROR("Rapid log message #%d - timestamp: %lu", i, millis());
        LOG_FATAL("Rapid log message #%d - timestamp: %lu", i, millis());

        // Check queue status every 5 messages
        if (i % 5 == 0) {
            LOG_INFO("Queue status - Available: %lu, Waiting: %lu, Dropped: %lu", 
                    AdvancedLogger::getQueueSpacesAvailable(),
                    AdvancedLogger::getQueueMessagesWaiting(),
                    AdvancedLogger::getDroppedCount());
        }

        delay(i * 5); // Simulate variable delay to mimic real-world logging scenarios
    }
    unsigned long endTime = millis();
    
    LOG_INFO("Rapid logging test completed in %lu ms", endTime - startTime);
    LOG_INFO("Final queue status - Available: %lu, Waiting: %lu", 
            AdvancedLogger::getQueueSpacesAvailable(),
            AdvancedLogger::getQueueMessagesWaiting());
}

void loop() {
    // Log some periodic information
    static unsigned long lastLog = 0;
    static int counter = 0;
    
    if (millis() - lastLog >= 5000) {
        lastLog = millis();
        counter++;
        
        LOG_INFO("Periodic log #%d - Free heap: %d bytes", counter, ESP.getFreeHeap());
        LOG_DEBUG("Queue status - Available: %lu, Waiting: %lu", 
                AdvancedLogger::getQueueSpacesAvailable(),
                AdvancedLogger::getQueueMessagesWaiting());
        
        // Log counters every 20 seconds
        if (counter % 4 == 0) {
            LOG_INFO("Log counters - DEBUG: %lu, INFO: %lu, WARNING: %lu, ERROR: %lu, TOTAL: %lu, DROPPED: %lu",
                    AdvancedLogger::getDebugCount(),
                    AdvancedLogger::getInfoCount(),
                    AdvancedLogger::getWarningCount(),
                    AdvancedLogger::getErrorCount(),
                    AdvancedLogger::getTotalLogCount(),
                    AdvancedLogger::getDroppedCount());
        }
    }
    
    // Simulate some work
    delay(100);
}

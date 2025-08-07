#include <Arduino.h>
#include <WiFi.h>
#include "AdvancedLogger.h"

// Optional: Configure queue parameters before including the library
// #define ADVANCED_LOGGER_QUEUE_SIZE 64
// #define ADVANCED_LOGGER_TASK_STACK_SIZE 8192
// #define ADVANCED_LOGGER_TASK_PRIORITY 2
// #define ADVANCED_LOGGER_TASK_CORE tskNO_AFFINITY
// #define ADVANCED_LOGGER_MAX_MESSAGE_LENGTH 512

void setup() {
    Serial.begin(115200);
    delay(1000);
    
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

        delay(i * 10);
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

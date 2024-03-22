/*
  * This is a simple example to show how to use the AdvancedLogger library.
  * 
  * The library is designed to be easy to use and to be able to log messages
  * to the console and to a file on the SPIFFS.
  * 
  * You can set the print level and save level, and the library will only log
  * messages accordingly.
*/
#include <Arduino.h>
#include <SPIFFS.h>

#include "advancedLogger.h"

AdvancedLogger logger;

void setup() {
    Serial.begin(9600);

    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    logger.begin();

    logger.setPrintLevel(ADVANCEDLOGGER_VERBOSE);
    logger.setSaveLevel(ADVANCEDLOGGER_INFO);

    logger.log("Setup done!", "simpleExample::setup", ADVANCEDLOGGER_INFO);
}

void loop() {
    logger.log("This is an debug message!", "simpleExample::loop", ADVANCEDLOGGER_VERBOSE);
    logger.log("This is an info message!!", "simpleExample::loop", ADVANCEDLOGGER_INFO);
    logger.log("This is an warning message!!!", "simpleExample::loop", ADVANCEDLOGGER_WARNING);
    logger.log("This is an error message!!!!", "simpleExample::loop", ADVANCEDLOGGER_ERROR);
    logger.log("This is an fatal message!!!!!", "simpleExample::loop", ADVANCEDLOGGER_FATAL);
    delay(1000);

    if (millis() > 60000) {
      logger.clearLog();
      logger.setDefaultLogLevels();
    }
}
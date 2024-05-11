/*
 * File: AdvancedLogger.h
 * ----------------------
 * This file exports the class AdvancedLogger, which provides
 * advanced logging for the ESP32.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 11/05/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This library provides advanced logging for the ESP32.
 * It allows you to log messages to the console and to a file on the SPIFFS.
 * You can set the print level and save level, and the library will only log
 * messages accordingly.
*/

#ifndef ADVANCEDLOGGER_H
#define ADVANCEDLOGGER_H

#define ADVANCEDLOGGER_VERBOSE 0
#define ADVANCEDLOGGER_DEBUG 1
#define ADVANCEDLOGGER_INFO 2
#define ADVANCEDLOGGER_WARNING 3
#define ADVANCEDLOGGER_ERROR 4
#define ADVANCEDLOGGER_FATAL 5

#define ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL 2 // 2 = INFO
#define ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL 3  // 3 = WARNING
#define ADVANCEDLOGGER_DEFAULT_MAX_LOG_LINES 1000 // 1000 lines before the log is cleared

#define ADVANCEDLOGGER_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"
#define ADVANCEDLOGGER_FORMAT "[%s] [%lu ms] [%s] [Core %d] [%s] %s" // [TIME] [MICROS us] [LOG_LEVEL] [Core CORE] [FUNCTION] MESSAGE

#define ADVANCEDLOGGER_LOG_PATH "/AdvancedLogger/log.txt"
#define ADVANCEDLOGGER_CONFIG_PATH "/AdvancedLogger/config.txt"

#include <Arduino.h>

#include <SPIFFS.h>

class AdvancedLogger
{
public:
    AdvancedLogger();

    void begin();

    void log(const char *message, const char *function, int logLevel);
    void logOnly(const char *message, const char *function, int logLevel);
    void setPrintLevel(int printLevel);
    void setSaveLevel(int saveLevel);

    String getPrintLevel();
    String getSaveLevel();

    void setDefaultLogLevels();

    void setMaxLogLines(int maxLines);
    int getLogLines();
    void clearLog();

    void dumpToSerial();

private:
    int _printLevel;
    int _saveLevel;

    int _maxLogLines;
    
    void _save(const char *messageFormatted);
    bool _setConfigFromSpiffs();
    void _saveConfigToSpiffs();

    String _logLevelToString(int logLevel);
    int _saturateLogLevel(int logLevel);

    String _getTimestamp();
};

#endif
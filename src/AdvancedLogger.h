/*
 * File: AdvancedLogger.h
 * ----------------------
 * This file exports the class AdvancedLogger, which provides
 * advanced logging for the ESP32.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 12/05/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 * Version: 1.1.4
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This library provides advanced logging for the ESP32.
 * It allows you to log messages to the console and to a file on the filesystem.
 * You can set the print level and save level, and the library will only log
 * messages accordingly.
 */

#ifndef ADVANCEDLOGGER_H
#define ADVANCEDLOGGER_H

enum class LogLevel : int {
    VERBOSE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    FATAL = 5
};

constexpr const LogLevel DEFAULT_PRINT_LEVEL = LogLevel::INFO;
constexpr const LogLevel DEFAULT_SAVE_LEVEL = LogLevel::WARNING;
constexpr const char* DEFAULT_LOG_PATH = "/AdvancedLogger/log.txt";
constexpr const char* DEFAULT_CONFIG_PATH = "/AdvancedLogger/config.txt";

constexpr int DEFAULT_MAX_LOG_LINES = 1000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";

constexpr const char* LOG_FORMAT = "[%s] [%lu ms] [%s] [Core %d] [%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FUNCTION] MESSAGE

#include <Arduino.h>

#ifdef ESP32
#include <SPIFFS.h>
#define Filesystem SPIFFS
#define CORE_ID xPortGetCoreID()
#define LOG_D(format, ...) log_d(format, ##__VA_ARGS__)
#define LOG_I(format, ...) log_i(format, ##__VA_ARGS__)
#define LOG_W(format, ...) log_w(format, ##__VA_ARGS__)
#define LOG_E(format, ...) log_e(format, ##__VA_ARGS__)
#elif defined(ESP8266)
#include <LittleFS.h>
#define Filesystem LittleFS
#define CORE_ID 0
#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif
#define LOG_D(format, ...) if (LOG_LEVEL <= 1) { Serial.printf("[%lu ms] [DEBUG] [AdvancedLogger] " format, millis(), ##__VA_ARGS__); Serial.println(); }
#define LOG_I(format, ...) if (LOG_LEVEL <= 2) { Serial.printf("[%lu ms] [INFO] [AdvancedLogger] " format, millis(), ##__VA_ARGS__); Serial.println(); }
#define LOG_W(format, ...) if (LOG_LEVEL <= 3) { Serial.printf("[%lu ms] [WARNING] [AdvancedLogger] " format, millis(), ##__VA_ARGS__); Serial.println(); }
#define LOG_E(format, ...) if (LOG_LEVEL <= 4) { Serial.printf("[%lu ms] [ERROR] [AdvancedLogger] " format, millis(), ##__VA_ARGS__); Serial.println(); }
#endif

class AdvancedLogger
{
public:
    AdvancedLogger(
        const char *logFilePath = DEFAULT_LOG_PATH,
        const char *configFilePath = DEFAULT_CONFIG_PATH,
        const char *timestampFormat = DEFAULT_TIMESTAMP_FORMAT);

    void begin();

    void verbose(const char *message, const char *function = "untracked", bool printOnly = false);
    void debug(const char *message, const char *function = "untracked", bool printOnly = false);
    void info(const char *message, const char *function = "untracked", bool printOnly = false);
    void warning(const char *message, const char *function = "untracked", bool printOnly = false);
    void error(const char *message, const char *function = "untracked", bool printOnly = false);
    void fatal(const char *message, const char *function = "untracked", bool printOnly = false);

    void setPrintLevel(LogLevel logLevel);
    void setSaveLevel(LogLevel logLevel);

    LogLevel getPrintLevel();
    LogLevel getSaveLevel();

    void setDefaultConfig();

    void setMaxLogLines(int maxLogLines);
    int getLogLines();
    void clearLog();

    void dump(Stream& stream);

    String logLevelToString(LogLevel logLevel, bool trim = true);

private:
    String _logFilePath = DEFAULT_LOG_PATH;
    String _configFilePath = DEFAULT_CONFIG_PATH;

    LogLevel _printLevel = DEFAULT_PRINT_LEVEL;
    LogLevel _saveLevel = DEFAULT_SAVE_LEVEL;

    int _maxLogLines = DEFAULT_MAX_LOG_LINES;
    int _logLines = 0;

    void _log(const char *message, const char *function, LogLevel logLevel, bool printOnly = false);
    void _save(const char *messageFormatted);
    bool _setConfigFromSpiffs();
    void _saveConfigToSpiffs();

    LogLevel _charToLogLevel(const char *logLevelStr);

    const char *_timestampFormat = DEFAULT_TIMESTAMP_FORMAT;
    String _getTimestamp();
    
    bool _invalidPath = false;
    bool _invalidTimestampFormat = false;
    bool _isValidPath(const char *path);
    bool _isValidTimestampFormat(const char *format);
};

#endif
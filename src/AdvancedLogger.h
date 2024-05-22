/*
 * File: AdvancedLogger.h
 * ----------------------
 * This file exports the class AdvancedLogger, which provides
 * advanced logging for the ESP32.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Date: 22/05/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 * Version: 1.2.0
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This library provides advanced logging capabilities for the ESP32, allowing you to log messages
 * to the console and to a file on the SPIFFS.
 */

#ifndef ADVANCEDLOGGER_H
#define ADVANCEDLOGGER_H

#include <Arduino.h>
#include <SPIFFS.h>

#define CORE_ID xPortGetCoreID()
#define LOG_D(format, ...) log_d(format, ##__VA_ARGS__)
#define LOG_I(format, ...) log_i(format, ##__VA_ARGS__)
#define LOG_W(format, ...) log_w(format, ##__VA_ARGS__)
#define LOG_E(format, ...) log_e(format, ##__VA_ARGS__)

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

constexpr int MAX_LOG_LENGTH = 1024;

constexpr const char* DEFAULT_LOG_PATH = "/AdvancedLogger/log.txt";
constexpr const char* DEFAULT_CONFIG_PATH = "/AdvancedLogger/config.txt";

constexpr int DEFAULT_MAX_LOG_LINES = 1000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";

constexpr const char* LOG_FORMAT = "[%s] [%s ms] [%s] [Core %d] [%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FUNCTION] MESSAGE

class AdvancedLogger
{
public:
    AdvancedLogger(
        const char *logFilePath,
        const char *configFilePath,
        const char *timestampFormat);

    void begin();

    void verbose(const char *format, const char *function, ...);
    void debug(const char *format, const char *function, ...);
    void info(const char *format, const char *function, ...);
    void warning(const char *format, const char *function, ...);
    void error(const char *format, const char *function, ...);
    void fatal(const char *format, const char *function, ...);

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

    void _log(const char *format, const char *function, LogLevel logLevel);
    void _logPrint(const char *format, const char *function, LogLevel logLevel, ...);
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

    std::string _formatMillis(unsigned long millis);
};

#endif
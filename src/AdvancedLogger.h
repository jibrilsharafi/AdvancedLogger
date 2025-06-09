/*
 * File: AdvancedLogger.h
 * ----------------------
 * This file exports the class AdvancedLogger, which provides
 * advanced logging for the ESP32.
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
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

#include <vector>

#define CORE_ID xPortGetCoreID()
#define LOG_D(format, ...) log_d(format, ##__VA_ARGS__)
#define LOG_I(format, ...) log_i(format, ##__VA_ARGS__)
#define LOG_W(format, ...) log_w(format, ##__VA_ARGS__)
#define LOG_E(format, ...) log_e(format, ##__VA_ARGS__)

enum class LogLevel : int {
    VERBOSE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

constexpr const LogLevel DEFAULT_PRINT_LEVEL = LogLevel::DEBUG;
constexpr const LogLevel DEFAULT_SAVE_LEVEL = LogLevel::INFO;

constexpr int MAX_LOG_LENGTH = 1024;

constexpr const char* DEFAULT_LOG_PATH = "/AdvancedLogger/log.txt";
constexpr const char* DEFAULT_CONFIG_PATH = "/AdvancedLogger/config.txt";

constexpr int DEFAULT_MAX_LOG_LINES = 1000;
constexpr int MAX_WHILE_LOOP_COUNT = 10000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";

constexpr const char* LOG_FORMAT = "[%s] [%s ms] [%s] [Core %d] [%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FUNCTION] MESSAGE

using LogCallback = std::function<void(
    const char* timestamp,
    unsigned long millisEsp,
    const char* level, 
    unsigned int coreId,
    const char* function,
    const char* message
)>;
     
// TODO: add better log rotation
class AdvancedLogger
{
public:
    AdvancedLogger(
        const char *logFilePath = DEFAULT_LOG_PATH,
        const char *configFilePath = DEFAULT_CONFIG_PATH,
        const char *timestampFormat = DEFAULT_TIMESTAMP_FORMAT);

    void begin();
    void end();

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
    bool _checkAndOpenLogFile();
    void clearLog();
    void clearLogKeepLatestXPercent(int percent = 10);

    void dump(Stream& stream);

    // Get the total amount of verbose logs since boot (or last reset) regardless of print or save level
    unsigned long getVerboseCount() { return _verboseCount; }
    // Get the total amount of debug logs since boot (or last reset) regardless of print or save level
    unsigned long getDebugCount() { return _debugCount; }
    // Get the total amount of info logs since boot (or last reset) regardless of print or save level
    unsigned long getInfoCount() { return _infoCount; }
    // Get the total amount of warning logs since boot (or last reset) regardless of print or save level
    unsigned long getWarningCount() { return _warningCount; }
    // Get the total amount of error logs since boot (or last reset) regardless of print or save level
    unsigned long getErrorCount() { return _errorCount; }
    // Get the total amount of fatal logs since boot (or last reset) regardless of print or save level
    unsigned long getFatalCount() { return _fatalCount; }
    // Get the total amount of logs since boot (or last reset) regardless of print or save level
    unsigned long getTotalLogCount() { return _verboseCount + _debugCount + _infoCount + _warningCount + _errorCount + _fatalCount; }
    void resetLogCounters();

    static const char* logLevelToString(LogLevel level, bool trim = true) {
        switch (level) {
            case LogLevel::VERBOSE: return trim ? "VERBOSE" : "VERBOSE ";
            case LogLevel::DEBUG:   return trim ? "DEBUG"   : "DEBUG   ";
            case LogLevel::INFO:    return trim ? "INFO"    : "INFO    ";
            case LogLevel::WARNING: return trim ? "WARNING" : "WARNING ";
            case LogLevel::ERROR:   return trim ? "ERROR"   : "ERROR   ";
            case LogLevel::FATAL:   return trim ? "FATAL"   : "FATAL   ";
            default:               return "UNKNOWN";
        }
    }

    static const char* logLevelToStringLower(LogLevel level) {
        switch (level) {
            case LogLevel::VERBOSE: return "verbose";
            case LogLevel::DEBUG:   return "debug";
            case LogLevel::INFO:    return "info";
            case LogLevel::WARNING: return "warning";
            case LogLevel::ERROR:   return "error";
            case LogLevel::FATAL:   return "fatal";
            default:               return "unknown";
        }
    }

    void setCallback(LogCallback callback) {
        _callback = callback;
    }
    void removeCallback() {
        _callback = nullptr;
    }

private:
    String _logFilePath = DEFAULT_LOG_PATH;
    String _configFilePath = DEFAULT_CONFIG_PATH;

    LogLevel _printLevel = DEFAULT_PRINT_LEVEL;
    LogLevel _saveLevel = DEFAULT_SAVE_LEVEL;    
    int _maxLogLines = DEFAULT_MAX_LOG_LINES;
    int _logLines = 0;

    unsigned long _verboseCount = 0;
    unsigned long _debugCount = 0;
    unsigned long _infoCount = 0;
    unsigned long _warningCount = 0;
    unsigned long _errorCount = 0;
    unsigned long _fatalCount = 0;

    void _log(const char *format, const char *function, LogLevel logLevel);
    void _increaseLogCount(LogLevel logLevel);
    void _logPrint(const char *format, const char *function, LogLevel logLevel, ...);
    
    File _logFile;
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

    String _formatMillis(unsigned long millis);

    LogCallback _callback = nullptr;
};

#endif
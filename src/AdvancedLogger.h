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

constexpr const LogLevel DEFAULT_PRINT_LEVEL = LogLevel::DEBUG;
constexpr const LogLevel DEFAULT_SAVE_LEVEL = LogLevel::INFO;
constexpr const char* DEFAULT_LOG_PATH = "/AdvancedLogger/log.txt";
constexpr const char* DEFAULT_CONFIG_PATH = "/AdvancedLogger/config.txt";

constexpr int DEFAULT_MAX_LOG_LINES = 1000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%Y-%m-%d %H:%M:%S";

constexpr const char* LOG_FORMAT = "[%s] [%lu ms] [%s] [Core %d] [%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FUNCTION] MESSAGE

#include <Arduino.h>
#include <FS.h>

class AdvancedLogger
{
public:
    /**
     * Constructs a new AdvancedLogger.
     *
     * @param fs The file system to use.
     * @param printLevel The minimum log level to print. Should be a value from the LogLevel enum class.
     * @param saveLevel The minimum log level to save. Should be a value from the LogLevel enum class.
     * @param maxLogLines The maximum number of log lines to save.
     * @param logFilePath The path to the log file.
     * @param configFilePath The path to the config file.
     * @param timestampFormat The format to use for timestamps.
     */
    AdvancedLogger(
        FS* fs = nullptr,
        const char *logFilePath = DEFAULT_LOG_PATH,
        const char *configFilePath = DEFAULT_CONFIG_PATH,
        const char *timestampFormat = DEFAULT_TIMESTAMP_FORMAT);

    void begin();

    void verbose(const char *message, const char *function, bool printOnly = false);
    void debug(const char *message, const char *function, bool printOnly = false);
    void info(const char *message, const char *function, bool printOnly = false);
    void warning(const char *message, const char *function, bool printOnly = false);
    void error(const char *message, const char *function, bool printOnly = false);
    void fatal(const char *message, const char *function, bool printOnly = false);

    void setPrintLevel(LogLevel logLevel);
    void setSaveLevel(LogLevel logLevel);

    String getPrintLevel();
    String getSaveLevel();

    void setDefaultConfig();

    void setMaxLogLines(int maxLines);
    int getLogLines();
    void clearLog();

    void dumpToSerial();

private:
    bool _shouldLogToFile = false;
    FS* _fs = nullptr;

    String _logFilePath = DEFAULT_LOG_PATH;
    String _configFilePath = DEFAULT_CONFIG_PATH;

    LogLevel _printLevel = DEFAULT_PRINT_LEVEL;
    LogLevel _saveLevel = DEFAULT_SAVE_LEVEL;

    int _maxLogLines = DEFAULT_MAX_LOG_LINES;
    int _logLines = 0;

    void _log(const char *message, const char *function, LogLevel logLevel, bool printOnly = false);
    void _save(const char *messageFormatted);
    bool _setConfigFromFs();
    void _saveConfigToFs();

    String _logLevelToString(LogLevel logLevel);
    LogLevel _stringToLogLevel(const String &logLevelStr);
    LogLevel _saturateLogLevel(LogLevel logLevel);

    const char *_timestampFormat = DEFAULT_TIMESTAMP_FORMAT;
    String _getTimestamp();
    
    bool _invalidPath = false;
    bool _invalidTimestampFormat = false;
    bool _isValidPath(const char *path);
    bool _isValidTimestampFormat(const char *format);
};

#endif
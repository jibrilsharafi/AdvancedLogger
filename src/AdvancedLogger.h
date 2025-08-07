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
 * to the console, to a file on the LittleFS, and to any callback function.
 */

#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <cstring>

// TODOs:
// - Use defines
// - Use macros so that the tag does not need to be passed
// - Global flags to remove at compile time some logs
// - Buffering logs for writing

// Legacy macros - consider using new LOG_* macros instead
#define LOG_D(format, ...) log_d(format, ##__VA_ARGS__)
#define LOG_I(format, ...) log_i(format, ##__VA_ARGS__)
#define LOG_W(format, ...) log_w(format, ##__VA_ARGS__)
#define LOG_E(format, ...) log_e(format, ##__VA_ARGS__)

// New AdvancedLogger macros with automatic file/function/line detection
#define LOG_VERBOSE(format, ...) AdvancedLogger::verbose(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)   AdvancedLogger::debug(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...)    AdvancedLogger::info(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) AdvancedLogger::warning(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)   AdvancedLogger::error(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)   AdvancedLogger::fatal(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

enum class FileMode : int {
    APPEND,   // "a" - append mode
    READ,     // "r" - read mode  
    WRITE     // "w" - write mode (truncates file)
};

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

constexpr unsigned int MAX_MESSAGE_LENGTH = 1024;
constexpr unsigned int MAX_LOG_LENGTH = MAX_MESSAGE_LENGTH + 128; // Extra space for timestamp, log level, and other metadata

constexpr const char* DEFAULT_LOG_PATH = "/log.txt";
constexpr const char* PREFERENCES_NAMESPACE = "adv_log_ns";

constexpr unsigned int DEFAULT_MAX_LOG_LINES = 1000;
constexpr unsigned int MAX_WHILE_LOOP_COUNT = 10000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";
constexpr unsigned int TIMESTAMP_BUFFER_SIZE = 25; // 2024-03-21T12:34:56.789Z (ISO 8601 format with milliseconds) is always 24 characters long

// Buffer sizes for char arrays
constexpr unsigned int MAX_LOG_PATH_LENGTH = 64;
constexpr unsigned int MAX_MILLIS_STRING_LENGTH = 32;  // For formatted milliseconds with spaces
constexpr unsigned int MAX_LOG_LINE_LENGTH = 1024;     // For reading log file lines
constexpr unsigned int MAX_TEMP_FILE_PATH_LENGTH = MAX_LOG_PATH_LENGTH + 4; // Original path + ".tmp" suffix
constexpr unsigned int MAX_LOG_MESSAGE_LENGTH = 64;     // For simple log messages

constexpr const char* LOG_PRINT_FORMAT = "[%s] [%s ms] [%s] [Core %d] [%s:%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FILE:FUNCTION] MESSAGE

struct LogEntry {
    char timestamp[TIMESTAMP_BUFFER_SIZE];
    unsigned long long millis;
    LogLevel level;
    int coreId;
    const char* file;
    const char* function;
    const char* message;

    LogEntry(
        const char* ts, 
        unsigned long long ms, 
        LogLevel lvl, 
        int core, 
        const char* f,
        const char* func, 
        const char* msg
    ) : millis(ms), level(lvl), coreId(core), file(f), function(func), message(msg) 
    { snprintf(timestamp, sizeof(timestamp), "%s", ts); }
};

using LogCallback = std::function<void(const LogEntry&)>;

namespace AdvancedLogger
{
    void begin(const char *logFilePath = DEFAULT_LOG_PATH);
    void end();

    void verbose(const char *format, const char *file, const char *function, int line, ...);
    void debug(const char *format, const char *file, const char *function, int line, ...);
    void info(const char *format, const char *file, const char *function, int line, ...);
    void warning(const char *format, const char *file, const char *function, int line, ...);
    void error(const char *format, const char *file, const char *function, int line, ...);
    void fatal(const char *format, const char *file, const char *function, int line, ...);

    void setPrintLevel(LogLevel logLevel);
    void setSaveLevel(LogLevel logLevel);

    LogLevel getPrintLevel();
    LogLevel getSaveLevel();

    void setDefaultConfig();

    void setMaxLogLines(unsigned long maxLogLines);
    unsigned long getLogLines();
    void clearLog();
    void clearLogKeepLatestXPercent(unsigned char percent = 10);

    void dump(Stream& stream);

    void resetLogCounters();

    /**
     * @brief Sets a callback function to handle log entries.
     *
     * This method sets a callback function that will be called for each log entry.
     *
     * @param callback The callback function to handle log entries.
     */
    void setCallback(LogCallback callback);
    
    /**
     * @brief Removes the currently set callback function.
     *
     * This method removes the callback function that was previously set.
     */
    void removeCallback();

    /**
     * @brief Converts a log level to a string representation.
     *
     * This method converts a log level to its string representation.
     *
     * @param level The log level to convert.
     * @param trim Whether to trim the string to fit the log format.
     * @return The string representation of the log level.
     */
    inline const char* logLevelToString(LogLevel level, bool trim = true) {
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

    /**
     * @brief Converts a log level to a lowercase string representation.
     *
     * This method converts a log level to its lowercase string representation.
     *
     * @param level The log level to convert.
     * @return The lowercase string representation of the log level.
     */
    inline const char* logLevelToStringLower(LogLevel level) {
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

    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getVerboseCount();
    
    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getDebugCount();

    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getInfoCount();

    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getWarningCount();

    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getErrorCount();

    /**
     * @brief Gets the total amount of logs for a specific log level.
     *
     * This method returns the total amount of logs for the specified log level.
     *
     * @param logLevel Log level for which to get the count.
     * @return unsigned long Total count of logs for the specified log level.
     */
    unsigned long getFatalCount();

    /**
     * @brief Gets the total amount of logs since boot (or last reset) regardless of print or save level.
     *
     * This method returns the total amount of logs since boot (or last reset) regardless of print or save level.
     *
     * @return unsigned long Total count of logs since boot (or last reset).
     */
    unsigned long getTotalLogCount();
};
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/*
 * Queue configuration defines that can be overridden by the user:
 * 
 * - ADVANCED_LOGGER_QUEUE_SIZE: Number of log entries that can be queued (default: 32)
 * - ADVANCED_LOGGER_TASK_STACK_SIZE: Stack size for the log processing task (default: 4096 bytes)
 * - ADVANCED_LOGGER_TASK_PRIORITY: Priority for the log processing task (default: 1)
 * - ADVANCED_LOGGER_TASK_CORE: Core ID for the log processing task (default: tskNO_AFFINITY)
 * - ADVANCED_LOGGER_MAX_MESSAGE_LENGTH: Maximum length of log messages (default: 256)
 *
 * Usage:
 * In platformio.ini: build_flags = -DADVANCED_LOGGER_QUEUE_SIZE=64
 * In Arduino IDE: Add #define ADVANCED_LOGGER_QUEUE_SIZE 64 before including this header
 * In CMake: add_definitions(-DADVANCED_LOGGER_QUEUE_SIZE=64)
 *
 * Note: The logging system uses a non-blocking queue. If the queue is full, 
 * log messages will be dropped to avoid blocking the calling thread. Use 
 * getQueueSpacesAvailable() and getQueueMessagesWaiting() to monitor queue status.
 */

#ifndef ADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE
    #define ADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE (10 * 1024)
#endif

#ifndef ADVANCED_LOGGER_TASK_STACK_SIZE
    #define ADVANCED_LOGGER_TASK_STACK_SIZE (4 * 1024)
#endif

#ifndef ADVANCED_LOGGER_TASK_PRIORITY
    #define ADVANCED_LOGGER_TASK_PRIORITY 2
#endif

#ifndef ADVANCED_LOGGER_TASK_CORE
    #define ADVANCED_LOGGER_TASK_CORE tskNO_AFFINITY
#endif

#ifndef ADVANCED_LOGGER_MAX_MESSAGE_LENGTH
    #define ADVANCED_LOGGER_MAX_MESSAGE_LENGTH 512
#endif


/*
 * Compilation flags to conditionally disable logging:
 * 
 * - ADVANCED_LOGGER_DISABLE_VERBOSE: Disables VERBOSE level logging
 * - ADVANCED_LOGGER_DISABLE_DEBUG: Disables DEBUG level logging  
 * - ADVANCED_LOGGER_DISABLE_INFO: Disables INFO level logging
 * - ADVANCED_LOGGER_DISABLE_WARNING: Disables WARNING level logging
 * - ADVANCED_LOGGER_DISABLE_ERROR: Disables ERROR level logging
 * - ADVANCED_LOGGER_DISABLE_FATAL: Disables FATAL level logging
 * - ADVANCED_LOGGER_DISABLE_FILE_LOGGING: Disables file logging
 * - ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING: Disables console output
 *
 * Usage:
 * In platformio.ini: build_flags = -DADVANCED_LOGGER_DISABLE_DEBUG
 * In Arduino IDE: Add #define ADVANCED_LOGGER_DISABLE_DEBUG before including this header
 * In CMake: add_definitions(-DADVANCED_LOGGER_DISABLE_DEBUG)
 */

#ifdef ADVANCED_LOGGER_DISABLE_VERBOSE
    #define LOG_VERBOSE(format, ...) ((void)0)
#else
    #define LOG_VERBOSE(format, ...) AdvancedLogger::verbose(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ADVANCED_LOGGER_DISABLE_DEBUG
    #define LOG_DEBUG(format, ...)   ((void)0)
#else
    #define LOG_DEBUG(format, ...)   AdvancedLogger::debug(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ADVANCED_LOGGER_DISABLE_INFO
    #define LOG_INFO(format, ...)    ((void)0)
#else
    #define LOG_INFO(format, ...)    AdvancedLogger::info(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ADVANCED_LOGGER_DISABLE_WARNING
    #define LOG_WARNING(format, ...) ((void)0)
#else
    #define LOG_WARNING(format, ...) AdvancedLogger::warning(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ADVANCED_LOGGER_DISABLE_ERROR
    #define LOG_ERROR(format, ...)   ((void)0)
#else
    #define LOG_ERROR(format, ...)   AdvancedLogger::error(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef ADVANCED_LOGGER_DISABLE_FATAL
    #define LOG_FATAL(format, ...)   ((void)0)
#else
    #define LOG_FATAL(format, ...)   AdvancedLogger::fatal(format, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#endif

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

constexpr const char* DEFAULT_LOG_PATH = "/log.txt";
constexpr const char* PREFERENCES_NAMESPACE = "adv_log_ns";

constexpr unsigned int DEFAULT_MAX_LOG_LINES = 1000;
constexpr unsigned int MAX_WHILE_LOOP_COUNT = 10000;

constexpr const char* DEFAULT_TIMESTAMP_FORMAT = "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ";
constexpr unsigned int TIMESTAMP_BUFFER_SIZE = 25; // 2024-03-21T12:34:56.789Z (ISO 8601 format with milliseconds) is always 24 characters long

constexpr unsigned int MAX_MESSAGE_LENGTH = ADVANCED_LOGGER_MAX_MESSAGE_LENGTH;
constexpr unsigned int MAX_LOG_LENGTH = MAX_MESSAGE_LENGTH + 160; // Extra space for timestamp, log level, and other metadata
constexpr unsigned int MAX_LOG_PATH_LENGTH = 64;
constexpr unsigned int MAX_MILLIS_STRING_LENGTH = 32;
constexpr unsigned int MAX_TEMP_FILE_PATH_LENGTH = MAX_LOG_PATH_LENGTH + 4;
constexpr unsigned int MAX_LOG_MESSAGE_LENGTH = 64;
constexpr unsigned int MAX_FILE_LENGTH = 32;
constexpr unsigned int MAX_FUNCTION_LENGTH = 32;
constexpr unsigned int MAX_INTERNAL_LOG_LENGTH = 128;

constexpr const char* LOG_PRINT_FORMAT = "[%s] [%s ms] [%s] [Core %d] [%s:%s] %s"; // [TIME] [MILLIS ms] [LOG_LEVEL] [Core CORE] [FILE:FUNCTION] MESSAGE

struct LogEntry {
    unsigned long long unixTimeMilliseconds;
    unsigned long long millis;
    LogLevel level;
    int coreId;
    char file[MAX_FILE_LENGTH];
    char function[MAX_FUNCTION_LENGTH];
    char message[MAX_MESSAGE_LENGTH];

    LogEntry()
        : unixTimeMilliseconds(0), millis(0), level(LogLevel::INFO), coreId(0)
    {
        file[0] = '\0';
        function[0] = '\0';
        message[0] = '\0';
    }

    LogEntry(
        unsigned long long unixTimeMs,
        unsigned long long ms,
        LogLevel lvl,
        int core,
        const char* f,
        const char* func, 
        const char* msg
    ) : unixTimeMilliseconds(unixTimeMs), millis(ms), level(lvl), coreId(core)
    {
        snprintf(file, sizeof(file), "%s", f);
        snprintf(function, sizeof(function), "%s", func);
        snprintf(message, sizeof(message), "%s", msg);
    }
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
     * @return The string representation of the log level (maximum 8 characters).
     */
    inline const char* logLevelToString(LogLevel level, bool trim = true) {
        switch (level) {
            case LogLevel::VERBOSE: return trim ? "VERBOSE" : "VERBOSE";
            case LogLevel::DEBUG:   return trim ? "DEBUG"   : "DEBUG  ";
            case LogLevel::INFO:    return trim ? "INFO"    : "INFO   ";
            case LogLevel::WARNING: return trim ? "WARNING" : "WARNING";
            case LogLevel::ERROR:   return trim ? "ERROR"   : "ERROR  ";
            case LogLevel::FATAL:   return trim ? "FATAL"   : "FATAL  ";
            default:               return "UNKNOWN";
        }
    }

    /**
     * @brief Converts a log level to a lowercase string representation.
     *
     * This method converts a log level to its lowercase string representation.
     *
     * @param level The log level to convert.
     * @return The lowercase string representation of the log level (maximum 8 characters).
     */
    inline const char* logLevelToStringLower(LogLevel level, bool trim = true) {
        switch (level) {
            case LogLevel::VERBOSE: return trim ? "verbose" : "verbose";
            case LogLevel::DEBUG:   return trim ? "debug"   : "debug  ";
            case LogLevel::INFO:    return trim ? "info"    : "info   ";
            case LogLevel::WARNING: return trim ? "warning" : "warning";
            case LogLevel::ERROR:   return trim ? "error"   : "error  ";
            case LogLevel::FATAL:   return trim ? "fatal"   : "fatal  ";
            default:               return "unknown";
        }
    }

    /**
     * @brief Formats a given Unix timestamp (in milliseconds) as an ISO 8601 UTC string with milliseconds.
     *
     * This function converts the provided Unix time in milliseconds to a string in ISO 8601 UTC format,
     * including milliseconds, and stores it in the provided buffer.
     *
     * @param unixTimeMilliseconds The Unix timestamp in milliseconds.
     * @param buffer Buffer to store the formatted timestamp string.
     * @param bufferSize Size of the buffer.
     */
    inline void getTimestampIsoUtcFromUnixTimeMilliseconds(unsigned long long unixTimeMilliseconds, char* buffer, size_t bufferSize)
    {
        time_t seconds = unixTimeMilliseconds / 1000;
        int milliseconds = (int)(unixTimeMilliseconds % 1000);

        struct tm utc_tm;
        gmtime_r(&seconds, &utc_tm);

        snprintf(buffer, bufferSize, DEFAULT_TIMESTAMP_FORMAT,
                utc_tm.tm_year + 1900,
                utc_tm.tm_mon + 1,
                utc_tm.tm_mday,
                utc_tm.tm_hour,
                utc_tm.tm_min,
                utc_tm.tm_sec,
                milliseconds);
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

    /**
     * @brief Gets the number of dropped log entries due to a full queue.
     *
     * This method returns the number of log entries that were dropped because the queue was full.
     *
     * @return unsigned long Number of dropped log entries.
     */
    unsigned long getDroppedCount();

    /**
     * @brief Gets the number of available spaces in the log queue.
     *
     * This method returns the number of log entries that can still be queued before the queue becomes full.
     *
     * @return unsigned long Number of available spaces in the log queue, or 0 if queue is not initialized.
     */
    unsigned long getQueueSpacesAvailable();

    /**
     * @brief Gets the number of log entries currently waiting in the log queue.
     *
     * This method returns the number of log entries currently waiting to be processed in the queue.
     *
     * @return unsigned long Number of messages waiting in the log queue, or 0 if queue is not initialized.
     */
    unsigned long getQueueMessagesWaiting();
};
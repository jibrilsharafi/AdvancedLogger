#include "AdvancedLogger.h"

#include <atomic>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>

// Macros
#define PROCESS_ARGS(format, line)                      \
    char message[MAX_MESSAGE_LENGTH];                   \
    va_list args;                                       \
    va_start(args, line);                               \
    vsnprintf(message, sizeof(message), format, args);  \
    va_end(args);

namespace AdvancedLogger
{    
    // Configuration variables
    static char _logFilePath[MAX_LOG_PATH_LENGTH];
    static LogLevel _printLevel = DEFAULT_PRINT_LEVEL;
    static LogLevel _saveLevel = DEFAULT_SAVE_LEVEL;    
    static unsigned long _maxLogLines = DEFAULT_MAX_LOG_LINES;
    static unsigned long _logLines = 0;

    // Log level counters
    static unsigned long _verboseCount = 0;
    static unsigned long _debugCount = 0;
    static unsigned long _infoCount = 0;
    static unsigned long _warningCount = 0;
    static unsigned long _errorCount = 0;
    static unsigned long _fatalCount = 0;
    static std::atomic<unsigned long> _droppedCount{0}; // Incremented by any task, on either core

    // File handling
    File _logFile;
    static FileMode _currentFileMode = FileMode::APPEND;

    // The log task appends to _logFile while any other task may call clearLog(), dump(),
    // getLogLines()... which close and reopen that same handle in another mode. Every user of
    // _logFile holds this lock. Recursive, as a rotation is started from inside a save.
    // Never deleted once created: another task may hold it or be waiting on it at any time.
    static SemaphoreHandle_t _fileMutex = nullptr;

    // The log task can afford to wait out a rotation started by another task. A public call runs
    // on the caller's task (a web handler under a watchdog, typically): it gives up quickly.
    class FileLock
    {
    public:
        explicit FileLock(unsigned long timeoutMs = FILE_MUTEX_API_TIMEOUT_MS)
            : _mutex(_fileMutex), _taken(_mutex && xSemaphoreTakeRecursive(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE) {}
        ~FileLock() { if (_taken) xSemaphoreGiveRecursive(_mutex); }
        FileLock(const FileLock&) = delete;
        FileLock& operator=(const FileLock&) = delete;
        explicit operator bool() const { return _taken; }
    private:
        SemaphoreHandle_t _mutex; // The one that was taken, whatever happens to the global since
        bool _taken;
    };

    // Callback function pointer
    static LogCallback _callback = nullptr;
    static LogLevel _callbackLevel = LogLevel::VERBOSE; // The callback gets entries at or above this level

    // Queue-based logging system
    static QueueHandle_t _logQueue = nullptr;
    static TaskHandle_t _logTaskHandle = nullptr;
    static volatile bool _queueInitialized = false;
    static volatile bool _logTaskShouldStop = false;
    static SemaphoreHandle_t _logTaskStopped = nullptr; // Given by the log task right before it deletes itself
    // Queue storage in PSRAM when there is one (the control structure stays in internal RAM)
    static StaticQueue_t _logQueueStruct;
    static uint8_t *_logQueueStorage = nullptr;

    // File flushing control
    static unsigned long _lastFlushTime = 0;

    // Forward declarations of private functions
    static void _log(const char *message, const char *file, const char *function, int line, LogLevel logLevel);
    static void _internalLog(const char* level, const char* format, ...);
    static void _save(const char *messageFormatted, bool flush = false);
    
    static void _increaseLogCount(LogLevel logLevel);
    
    static void _closeLogFile();      
    static bool _reopenLogFile(FileMode mode = FileMode::APPEND);
    static bool _checkAndOpenLogFile(FileMode mode = FileMode::APPEND);
    static const char* _fileModeToString(FileMode mode);
    
    static bool _setConfigFromPreferences();
    static void _saveConfigToPreferences();
    
    static unsigned long long _getUnixTimeMilliseconds();
    static void _formatMillis(unsigned long long millis, char* buffer, size_t bufferSize);
    
    static bool _isValidPath(const char *path);
    static bool _ensureDirectoryExists(const char* filePath);
    
    static void _initLogQueue();
    static void _destroyLogQueue();
    
    static void _logProcessingTask(void* parameter);
    static void _processLogEntry(const LogEntry& entry);
    static void _reportDroppedEntries(LogEntry& scratch);

    // Public functions
    // ================

    /**
     * @brief Initializes the AdvancedLogger.
     *
     * Sets up the logger by mounting LittleFS (if needed), setting configuration
     * from preferences (or defaults), ensuring log directory exists, and 
     * initializing the queue-based logging system.
     * 
     * @param logFilePath Path to the log file (defaults to DEFAULT_LOG_PATH)
     */
    void begin(const char *logFilePath)
    {
        _internalLog("DEBUG", "AdvancedLogger initializing...");

        // Mount LittleFS if not already mounted
        if (!LittleFS.begin(false)) {
            Serial.printf("Failed to mount LittleFS. Please mount it before using AdvancedLogger.\n");
            _internalLog("ERROR", "LittleFS mount failed");
            return;
        }

        // Initialize _logFilePath with provided path or default
        if (logFilePath && _isValidPath(logFilePath)) {
            snprintf(_logFilePath, MAX_LOG_PATH_LENGTH, "%s", logFilePath);
        } else {
            if (logFilePath) {
                Serial.printf(
                    "Invalid path for log file %s, using default path: %s\n",
                    logFilePath,
                    DEFAULT_LOG_PATH
                );
            }
            snprintf(_logFilePath, MAX_LOG_PATH_LENGTH, "%s", DEFAULT_LOG_PATH);
        }

        if (!_setConfigFromPreferences())
        {
            _internalLog("DEBUG", "Using default config as preferences were not found");
        }

        if (!_fileMutex) _fileMutex = xSemaphoreCreateRecursiveMutex();
        if (!_fileMutex) {
            _internalLog("ERROR", "Failed to create the log file mutex");
            return;
        }

        File testFile = LittleFS.open("/", "r");
        bool isAlreadyMounted = testFile;
        if (testFile) testFile.close();
        
        if (!isAlreadyMounted) {
            bool littleFsMounted = LittleFS.begin(false);
            if (!littleFsMounted) {
                Serial.printf("Failed to mount LittleFS. Please mount it before using AdvancedLogger.\n");
                _internalLog("ERROR", "LittleFS mount failed");
                return;
            }
            _internalLog("DEBUG", "LittleFS mounted successfully");
        } else {
            _internalLog("DEBUG", "LittleFS already mounted");
        }    
        
        if (!_ensureDirectoryExists(_logFilePath)) {
            Serial.printf("Failed to create directory for log file %s, falling back to default path\n", _logFilePath);
            snprintf(_logFilePath, MAX_LOG_PATH_LENGTH, "%s", DEFAULT_LOG_PATH);
            if (!_ensureDirectoryExists(_logFilePath)) {
                Serial.printf("Failed to create directory for default log file %s\n", _logFilePath);
                _internalLog("ERROR", "Log file directory creation failed");
                return;
            }
        }
        
        FileLock lock(FILE_MUTEX_TIMEOUT_MS); // A second begin() without end() finds the log task already running
        bool isLogFileOpen = lock && _checkAndOpenLogFile(FileMode::APPEND);
        if (!isLogFileOpen) {
            Serial.printf("Failed to open log file %s\n", _logFilePath);
            _internalLog("ERROR", "Log file opening failed");
            return;
        }
        
        _logLines = getLogLines();
        
        // Initialize flush timestamp
        _lastFlushTime = millis();
        
        _initLogQueue();
        
        _internalLog("DEBUG", "AdvancedLogger initialized");
    }

    /**
     * @brief Ends the AdvancedLogger.
     *
     * Closes the log file and cleans up the queue-based logging system.
     */
    void end()
    {
        if (_logTaskHandle && xTaskGetCurrentTaskHandle() == _logTaskHandle) {
            _internalLog("ERROR", "AdvancedLogger end called from a log callback, ignored");
            return;
        }

        // The task goes first: closing the file under a running task only makes it reopen it
        _destroyLogQueue();

        FileLock lock;
        if (!lock) {
            // Another task is in the middle of a file operation: it closes the file itself
            _internalLog("WARNING", "AdvancedLogger ended while the log file was in use, not closing it");
        } else if (_logFile) {
            _internalLog("INFO", "AdvancedLogger ended");
            _closeLogFile();
        } else {
            _internalLog("WARNING", "AdvancedLogger end called but log file was not open");
        }
    }

    void verbose(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::VERBOSE);
    }

    void debug(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::DEBUG);
    }

    void info(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::INFO);
    }

    void warning(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::WARNING);
    }

    void error(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::ERROR);
    }

    void fatal(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::FATAL);
    }

    /**
     * @brief Safe internal logging function that doesn't trigger recursion.
     * 
     * This function is used for internal AdvancedLogger operations to avoid
     * infinite recursion when logging operations themselves need to log.
     */
    static void _internalLog(const char* level, const char* format, ...)
    {
#ifndef ADVANCED_LOGGER_DISABLE_INTERNAL_LOGGING
            char buffer[MAX_INTERNAL_LOG_LENGTH];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            
#ifndef ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING
            Serial.printf("[%s] [AdvancedLogger] %s\n", level, buffer);
#endif
#endif
    }

    // The queue storage is the largest allocation of the library, and a plain FreeRTOS queue
    // always comes from internal RAM, the scarce heap on ESP32. With PSRAM, only the small
    // control structure stays internal and the entries live in PSRAM. Entries are copied in and
    // out by xQueueSend/xQueueReceive, so nothing downstream (file writes included) ever reads
    // PSRAM directly. Define ADVANCED_LOGGER_DISABLE_PSRAM_QUEUE to keep everything internal.
    static size_t _queueEntriesFor(size_t bytes)
    {
        size_t entries = bytes / sizeof(LogEntry);
        return entries > 0 ? entries : 1; // Ensure at least one entry can be queued
    }

    static QueueHandle_t _createLogQueue()
    {
#ifndef ADVANCED_LOGGER_DISABLE_PSRAM_QUEUE
        if (psramFound()) {
            size_t queueSize = _queueEntriesFor(ADVANCED_LOGGER_PSRAM_QUEUE_SIZE);
            _logQueueStorage = (uint8_t*)heap_caps_malloc(queueSize * sizeof(LogEntry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (_logQueueStorage) {
                QueueHandle_t queue = xQueueCreateStatic(queueSize, sizeof(LogEntry), _logQueueStorage, &_logQueueStruct);
                if (queue) {
                    _internalLog("DEBUG", "Log queue of %u entries in PSRAM", (unsigned)queueSize);
                    return queue;
                }
                free(_logQueueStorage);
                _logQueueStorage = nullptr;
            }
        }
#endif
        // The internal RAM budget is separate: a size meant for PSRAM must never land here
        size_t queueSize = _queueEntriesFor(ADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE);
        _internalLog("DEBUG", "Log queue of %u entries in internal RAM", (unsigned)queueSize);
        return xQueueCreate(queueSize, sizeof(LogEntry));
    }

    static void _deleteLogQueue()
    {
        if (!_logQueue) return;
        vQueueDelete(_logQueue); // A static queue's memory is not freed by FreeRTOS
        _logQueue = nullptr;
        if (_logQueueStorage) {
            free(_logQueueStorage);
            _logQueueStorage = nullptr;
        }
    }

    static void _initLogQueue()
    {
        if (_queueInitialized) return; // Already initialized

        // Create the queue for log entries
        _logQueue = _createLogQueue();
        if (!_logQueue) {
            _internalLog("ERROR", "Failed to create log queue");
            return;
        }

        _logTaskShouldStop = false;
        _logTaskStopped = xSemaphoreCreateBinary();
        if (!_logTaskStopped) {
            _internalLog("ERROR", "Failed to create the log task stop semaphore");
            _deleteLogQueue();
            return;
        }

        BaseType_t taskResult = xTaskCreatePinnedToCore(
            _logProcessingTask,
            "AdvancedLogTask",
            ADVANCED_LOGGER_TASK_STACK_SIZE,
            nullptr,
            ADVANCED_LOGGER_TASK_PRIORITY,
            &_logTaskHandle,
            ADVANCED_LOGGER_TASK_CORE
        );

        if (taskResult != pdPASS) {
            _internalLog("ERROR", "Failed to create log processing task");
            _logTaskHandle = nullptr;
            _deleteLogQueue();
            vSemaphoreDelete(_logTaskStopped);
            _logTaskStopped = nullptr;
            return;
        }

        _queueInitialized = true;
        _internalLog("DEBUG", "Log queue and task initialized successfully");
    }

    static void _destroyLogQueue()
    {
        if (!_queueInitialized) return; // Not initialized

        _queueInitialized = false; // First: no new entry gets in from here on

        if (_logTaskHandle) {
            // The task is asked to stop: it saves what was queued, says so and parks itself. A
            // task deleted from outside in the middle of a file write or a print never releases
            // the locks it holds (LittleFS, UART), and the next user of those blocks forever.
            _logTaskShouldStop = true;
            bool stopped = _logTaskStopped && xSemaphoreTake(_logTaskStopped, pdMS_TO_TICKS(LOG_TASK_STOP_TIMEOUT_MS)) == pdTRUE;
            if (!stopped) {
                _internalLog("WARNING", "Log task did not stop in time, deleting it");
                // If it dies holding the file lock nobody can ever take it again: a fresh one takes
                // its place. The old one is leaked on purpose, another task may be waiting on it.
                if (_fileMutex && xSemaphoreGetMutexHolder(_fileMutex) == _logTaskHandle) {
                    SemaphoreHandle_t freshMutex = xSemaphoreCreateRecursiveMutex();
                    if (freshMutex) _fileMutex = freshMutex;
                }
            }
            // The task never deletes itself, so the handle is valid here in both cases
            vTaskDelete(_logTaskHandle);
            _logTaskHandle = nullptr;
        }

        // A caller that passed the _queueInitialized check just before it was cleared may still be
        // waiting for a slot: let it finish before the queue goes away under it
        vTaskDelay(pdMS_TO_TICKS(ADVANCED_LOGGER_QUEUE_FULL_WAIT_MS) + 2);

        _deleteLogQueue();
        if (_logTaskStopped) {
            vSemaphoreDelete(_logTaskStopped);
            _logTaskStopped = nullptr;
        }

        _internalLog("DEBUG", "Log queue and task destroyed");
    }

    /**
     * @brief FreeRTOS task function for processing log entries.
     *
     * This task (only) continuously processes log entries from the queue.
     *
     * @param parameter Task parameter (unused).
     */
    static void _logProcessingTask(void* parameter)
    {
        LogEntry entry; // Default constructor values

        // The wait is bounded only so that a stop request is seen
        while (!_logTaskShouldStop) {
            if (xQueueReceive(_logQueue, &entry, pdMS_TO_TICKS(LOG_TASK_STOP_POLL_MS)) == pdTRUE) {
                _processLogEntry(entry);
                _reportDroppedEntries(entry);
            }
        }

        // Stopping: what is already queued still reaches the sinks (the restart reason is
        // typically in there). Bounded by what was queued at this point.
        for (UBaseType_t remaining = uxQueueMessagesWaiting(_logQueue); remaining > 0; remaining--) {
            if (xQueueReceive(_logQueue, &entry, 0) != pdTRUE) break;
            _processLogEntry(entry);
        }

        // end() deletes the task: with a single owner of the deletion, a stop that completes
        // right at the timeout cannot end in a delete of an already freed task
        xSemaphoreGive(_logTaskStopped);
        while (true) vTaskSuspend(NULL); // A task function must never return
    }

    /**
     * @brief Leaves a trace in the log itself when entries were dropped.
     *
     * Runs on the log task, so it goes straight to the sinks without taking a queue slot.
     * Rate limited: during a sustained overflow it must not add to the load.
     *
     * @param scratch Entry buffer of the log task, reused so the notice costs no extra stack.
     */
    static void _reportDroppedEntries(LogEntry& scratch)
    {
        static unsigned long reportedCount = 0;
        static unsigned long lastReportTime = 0;

        unsigned long droppedCount = _droppedCount;
        if (droppedCount < reportedCount) reportedCount = 0; // The counters were reset
        if (droppedCount == reportedCount) return;
        if (lastReportTime != 0 && (millis() - lastReportTime < DROPPED_REPORT_INTERVAL_MS)) return;

        scratch.unixTimeMilliseconds = _getUnixTimeMilliseconds();
        scratch.millis = esp_timer_get_time() / 1000ULL;
        scratch.level = LogLevel::WARNING;
        scratch.coreId = xPortGetCoreID();
        snprintf(scratch.file, sizeof(scratch.file), "AdvancedLogger");
        snprintf(scratch.function, sizeof(scratch.function), "log");
        snprintf(scratch.message, sizeof(scratch.message), "%lu log entries dropped because the queue was full (%lu since the counters were reset)", droppedCount - reportedCount, droppedCount);
        reportedCount = droppedCount;
        lastReportTime = millis();

        _processLogEntry(scratch);
    }

    /**
     * @brief Processes a single log entry.
     *
     * This function handles the actual logging operations (console output, file writing, callbacks).
     *
     * @param entry The log entry to process.
     */
    // Who wants an entry of this level. A sink disabled at compile time wants nothing, whatever
    // level is stored for it: its entries must not take queue slots only to be thrown away.
    static bool _isWantedByCallback(LogLevel logLevel) { return _callback && (logLevel >= _callbackLevel); }

    static bool _isWantedByConsole(LogLevel logLevel)
    {
#ifdef ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING
        return false;
#else
        return logLevel >= _printLevel;
#endif
    }

    static bool _isWantedByFile(LogLevel logLevel)
    {
#ifdef ADVANCED_LOGGER_DISABLE_FILE_LOGGING
        return false;
#else
        return logLevel >= _saveLevel;
#endif
    }

    static void _processLogEntry(const LogEntry& entry)
    {
        if (_isWantedByCallback(entry.level)) _callback(entry);

        // Eventual early return
        if (!_isWantedByConsole(entry.level) && !_isWantedByFile(entry.level)) return;

        char messageFormatted[MAX_LOG_LENGTH + 2]; // Room for the console line ending

        char timestamp[TIMESTAMP_BUFFER_SIZE];
        getTimestampIsoUtcFromUnixTimeMilliseconds(entry.unixTimeMilliseconds, timestamp, sizeof(timestamp));

        char formattedMillis[MAX_MILLIS_STRING_LENGTH];
        _formatMillis(entry.millis, formattedMillis, sizeof(formattedMillis));

        snprintf(
            messageFormatted,
            MAX_LOG_LENGTH,
            LOG_PRINT_FORMAT,
            timestamp,
            formattedMillis,
            logLevelToString(entry.level, false),
            entry.coreId,
            entry.file,
            entry.function,
            entry.message);

#ifndef ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING
        if (_isWantedByConsole(entry.level)) {
            // One write for the line and its ending: the serial driver locks per write, so a
            // print from another task can no longer land between the two
            size_t length = strlen(messageFormatted);
            messageFormatted[length] = '\r';
            messageFormatted[length + 1] = '\n';
            Serial.write(reinterpret_cast<const uint8_t*>(messageFormatted), length + 2);
            messageFormatted[length] = '\0';
        }
#endif

#ifndef ADVANCED_LOGGER_DISABLE_FILE_LOGGING
        if (_isWantedByFile(entry.level)) {
            // Determine if immediate flush is needed based on log level
            bool forceFlush = (entry.level >= ADVANCED_LOGGER_FLUSH_LOG_LEVEL);
            _save(messageFormatted, forceFlush);
        }
#endif
    }

    /**
     * @brief Core logging function that queues log entries for processing.
     *
     * Handles log entry creation, queue management, and fallback processing.
     * Log entries are queued for asynchronous processing by the log task.
     *
     * @param message The formatted message to log.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param logLevel Log level of the message.
     */
    void _log(const char *message, const char *file, const char *function, int line, LogLevel logLevel)
    {
        _increaseLogCount(logLevel); // Increment regardless

        if (!_queueInitialized || !_logQueue) {
            _internalLog("WARNING", "Log queue not initialized, skipping log entry");
            return;
        }

        // Early return if nobody wants this entry: it must not take a queue slot from one that
        // is wanted (a VERBOSE flood otherwise fills the queue and pushes real logs out)
        if (!_isWantedByCallback(logLevel) && !_isWantedByConsole(logLevel) && !_isWantedByFile(logLevel)) return;

        unsigned long long unixTimeMs = _getUnixTimeMilliseconds();
        unsigned long long millis = (esp_timer_get_time() / 1000ULL);

        LogEntry entry(
            unixTimeMs,
            millis,
            logLevel,
            xPortGetCoreID(),
            file,
            function,
            message
        );

        // A full queue makes the caller wait a bounded time for a slot, then drops the entry
        // (counted, and reported by the log task). It used to process one entry inline to make
        // room, but that ran the file write and the callback on the CALLER's task: concurrently
        // with the log task on the same file (lines glued together in the log) and on a stack
        // sized for the caller, not for LittleFS. Size the queue for the bursts instead - with
        // PSRAM it can be hundreds of entries for free.
        // The log task itself never waits: nobody else empties the queue.
        bool canWait = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) && (xTaskGetCurrentTaskHandle() != _logTaskHandle);
        TickType_t ticksToWait = canWait ? pdMS_TO_TICKS(ADVANCED_LOGGER_QUEUE_FULL_WAIT_MS) : 0;
        if (xQueueSend(_logQueue, &entry, ticksToWait) != pdTRUE) _droppedCount++;
    }

    /**
     * @brief Increases the counter for the specified log level.
     * @param logLevel Log level to increment.
     */
    void _increaseLogCount(LogLevel logLevel)
    {
        switch (logLevel)
        {
            case LogLevel::VERBOSE:
                _verboseCount++;
                break;
            case LogLevel::DEBUG:
                _debugCount++;
                break;
            case LogLevel::INFO:
                _infoCount++;
                break;
            case LogLevel::WARNING:
                _warningCount++;
                break;
            case LogLevel::ERROR:
                _errorCount++;
                break;
            case LogLevel::FATAL:
                _fatalCount++;
                break;
        }
    }

    void setPrintLevel(LogLevel logLevel)
    {
        _printLevel = logLevel;
        _saveConfigToPreferences();
        _internalLog("DEBUG", "Set print level to %s", logLevelToString(logLevel));
    }

    void setSaveLevel(LogLevel logLevel)
    {
        _saveLevel = logLevel;
        _saveConfigToPreferences();
        _internalLog("DEBUG", "Set save level to %s", logLevelToString(logLevel));
    }

    LogLevel getPrintLevel()
    {
        return _printLevel;
    }

    LogLevel getSaveLevel()
    {
        return _saveLevel;
    }

    void setDefaultConfig()
    {
        setPrintLevel(DEFAULT_PRINT_LEVEL);
        setSaveLevel(DEFAULT_SAVE_LEVEL);
        setMaxLogLines(DEFAULT_MAX_LOG_LINES);

        _internalLog("DEBUG", "Config set to default");
    }

    unsigned long getVerboseCount() { return _verboseCount; }
    unsigned long getDebugCount() { return _debugCount; }
    unsigned long getInfoCount() { return _infoCount; }
    unsigned long getWarningCount() { return _warningCount; }
    unsigned long getErrorCount() { return _errorCount; }
    unsigned long getFatalCount() { return _fatalCount; }
    unsigned long getTotalLogCount() { return _verboseCount + _debugCount + _infoCount + _warningCount + _errorCount + _fatalCount; }
    unsigned long getDroppedCount() { return _droppedCount; }

    unsigned long getQueueSpacesAvailable() 
    { 
        if (!_queueInitialized || !_logQueue) {
            return 0;
        }
        return uxQueueSpacesAvailable(_logQueue);
    }
    
    unsigned long getQueueMessagesWaiting() 
    { 
        if (!_queueInitialized || !_logQueue) {
            return 0;
        }
        return uxQueueMessagesWaiting(_logQueue);
    }
    
    void setCallback(LogCallback callback) { _callback = callback; }
    void setCallbackLevel(LogLevel logLevel) { _callbackLevel = logLevel; }
    LogLevel getCallbackLevel() { return _callbackLevel; }
    void removeCallback() { _callback = nullptr; }

    bool _setConfigFromPreferences()
    {
        Preferences preferences;

        // Try to open preferences in read-write mode (this creates namespace if it doesn't exist)
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
            _internalLog("DEBUG", "Failed to open preferences namespace");
            // Set default values
            _printLevel = DEFAULT_PRINT_LEVEL;
            _saveLevel = DEFAULT_SAVE_LEVEL;
            _maxLogLines = DEFAULT_MAX_LOG_LINES;
            return false;
        }
        
        if (!preferences.isKey("printLevel")) {
            _internalLog("DEBUG", "Fresh preferences namespace detected, initializing with defaults");
            preferences.putInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
            preferences.putInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
            preferences.putULong("maxLogLines", DEFAULT_MAX_LOG_LINES);
            
            _printLevel = DEFAULT_PRINT_LEVEL;
            _saveLevel = DEFAULT_SAVE_LEVEL;
            _maxLogLines = DEFAULT_MAX_LOG_LINES;
        } else {
            _internalLog("DEBUG", "Loading existing preferences");
            int printLevelInt = preferences.getInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
            _printLevel = static_cast<LogLevel>(printLevelInt);
            
            int saveLevelInt = preferences.getInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
            _saveLevel = static_cast<LogLevel>(saveLevelInt);
            
            _maxLogLines = preferences.getULong("maxLogLines", DEFAULT_MAX_LOG_LINES);
        }
        
        preferences.end();

        _internalLog("DEBUG", "Config loaded from preferences");
        return true;
    }

    void _saveConfigToPreferences()
    {
        Preferences preferences;

        // Try to open in read-write mode (this will create the namespace if it doesn't exist)
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
            _internalLog("DEBUG", "Failed to open preferences for writing");
            return;
        }
        
        preferences.putInt("printLevel", static_cast<int>(_printLevel));
        preferences.putInt("saveLevel", static_cast<int>(_saveLevel));
        preferences.putULong("maxLogLines", _maxLogLines);
        
        preferences.end();

        _internalLog("DEBUG", "Config saved to preferences");
    }

    void setMaxLogLines(unsigned long maxLogLines)
    {
        _internalLog("DEBUG", "Setting max log lines to %d", maxLogLines);
        _maxLogLines = maxLogLines;
        _saveConfigToPreferences();
    }

    unsigned long getLogLines()
    {
        FileLock lock;
        if (!lock || !_checkAndOpenLogFile(FileMode::READ)) {
            return 0;
        }

        // The whole file is counted, in chunks and bounded by its size. It used to stop after
        // MAX_WHILE_LOOP_COUNT BYTES: every boot restarted the line count at about a hundred
        // whatever the size of the file, so a device saving fewer than the maximum per boot never
        // reached the automatic rotation and the log grew without limit.
        unsigned long lines = 0;
        uint8_t buffer[FILE_READ_CHUNK_SIZE];
        size_t remaining = _logFile.size();
        while (remaining > 0)
        {
            size_t toRead = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            size_t bytesRead = _logFile.read(buffer, toRead);
            if (bytesRead == 0 || bytesRead > toRead) break;
            for (size_t i = 0; i < bytesRead; i++)
            {
                if (buffer[i] == '\n') lines++;
            }
            remaining -= bytesRead;
        }

        _closeLogFile();
        _checkAndOpenLogFile(FileMode::APPEND);
        
        return lines;
    }

    /**
     * @brief Checks if log file is open with correct mode and opens/reopens as needed.
     * @param mode Desired file mode.
     * @return true if file is ready for use, false otherwise.
     */
    bool _checkAndOpenLogFile(FileMode mode)
    {
        // If file is already open with the correct mode, return true
        if (_logFile && _currentFileMode == mode) {
            return true;
        }
        
        // If file is open but with wrong mode, close and reopen
        if (_logFile && _currentFileMode != mode) {
            _closeLogFile();
        }
        
        // Open file with requested mode
        return _reopenLogFile(mode);
    }

    bool clearLog()
    {
        FileLock lock;
        if (!lock) {
            _internalLog("WARNING", "Log file busy, log not cleared");
            return false;
        }
        if (!_checkAndOpenLogFile(FileMode::WRITE)) return false;

        _closeLogFile();
        _logLines = 0;
        _internalLog("INFO", "Log cleared");
        return true;
    }
    
    void clearLogKeepLatestXPercent(unsigned char percent) 
    {
        FileLock lock;
        if (!lock || !_checkAndOpenLogFile(FileMode::READ)) return;

        size_t totalLines = 0;
        char lineBuffer[MAX_LOG_LENGTH + 2]; // A whole saved line plus its line ending, or long lines get split in two
        while (_logFile.available() && totalLines < MAX_WHILE_LOOP_COUNT) {
            if (_logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1) > 0) {
                totalLines++;
            }
        }
        
        if (!_reopenLogFile(FileMode::READ)) return;

        percent = percent > 100 ? 100 : percent;

        size_t linesToKeep = (totalLines * percent) / 100;
        size_t linesToSkip = totalLines - linesToKeep;

        char tempFilePath[MAX_TEMP_FILE_PATH_LENGTH];
        snprintf(tempFilePath, sizeof(tempFilePath), "%s.tmp", _logFilePath);
        
        File tempFile = LittleFS.open(tempFilePath, "w");
        if (!tempFile) {
            _internalLog("ERROR", "Failed to create temp file");
            _closeLogFile();
            return;
        }

        // Same rule as the counting pass above: only a read that returned something is a line
        size_t skippedLines = 0;
        int loopCount = 0;
        while (skippedLines < linesToSkip && _logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT) {
            if (_logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1) > 0) skippedLines++;
            loopCount++;
        }

        size_t expectedBytes = 0;
        loopCount = 0;
        while (_logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT) {
            int bytesRead = _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
            if (bytesRead > 0) {
                // println() adds the line ending back: without this every kept line gains one
                // more '\r' at each rotation
                while (bytesRead > 0 && lineBuffer[bytesRead - 1] == '\r') bytesRead--;
                lineBuffer[bytesRead] = '\0';
                tempFile.println(lineBuffer);
                expectedBytes += bytesRead + 2;
            }
            loopCount++;
        }

        // Writes are buffered, so a full filesystem only shows once flushed, as a short file.
        // The log is then kept as it is rather than replaced by a truncated copy.
        tempFile.flush();
        bool copied = (tempFile.size() == expectedBytes);

        _closeLogFile();
        tempFile.close();

        // rename() replaces the destination in one step on LittleFS, so a power loss never leaves
        // the device without a log file. Removing first is only the fallback. Both fail while
        // someone else holds the log open (a download in progress): the log is then left alone.
        bool replaced = copied && LittleFS.rename(tempFilePath, _logFilePath);
        if (copied && !replaced) {
            LittleFS.remove(_logFilePath);
            replaced = LittleFS.rename(tempFilePath, _logFilePath);
        }

        // On failure the count still restarts from the kept share: the next attempt comes after
        // another batch of lines, not at every single line
        _logLines = linesToKeep;

        if (!replaced) {
            // Unless the fallback removed the log and then failed to rename: the copy is all there is
            if (LittleFS.exists(_logFilePath)) LittleFS.remove(tempFilePath);
            _internalLog("ERROR", "Failed to replace the log file, the log was left as it is");
            return;
        }
        _internalLog("INFO", "Log cleared keeping latest entries");
    }

    /**
     * @brief Writes a formatted message to the log file.
     * @param messageFormatted The formatted message to save.
     * @param flush Whether to force immediate write to flash storage.
     */
    void _save(const char *messageFormatted, bool flush)
    {
        // Once asked to stop, the log task no longer waits out somebody else's rotation
        FileLock lock(_logTaskShouldStop ? FILE_MUTEX_API_TIMEOUT_MS : FILE_MUTEX_TIMEOUT_MS);
        if (!lock || !_checkAndOpenLogFile(FileMode::APPEND)) return;

        _logFile.println(messageFormatted);
        
        // Smart flushing logic
        unsigned long currentTime = millis();
        bool shouldFlush = flush;
        
        // Check if periodic flush interval has elapsed
        if (!shouldFlush && (currentTime - _lastFlushTime >= ADVANCED_LOGGER_FLUSH_INTERVAL_MS)) {
            shouldFlush = true;
        }
        
        if (shouldFlush) {
            _logFile.flush();
            _lastFlushTime = currentTime;
        }
        
        _logLines++;

        if (_logLines >= _maxLogLines) {
            clearLogKeepLatestXPercent();
        }
    }

    void dump(Stream &stream)
    {
        _internalLog("DEBUG", "Dumping log to Stream...");

        FileLock lock;
        if (!lock || !_checkAndOpenLogFile(FileMode::READ)) return;

        // Whole file, in chunks and bounded by its size (it used to stop after MAX_WHILE_LOOP_COUNT bytes)
        uint8_t buffer[FILE_READ_CHUNK_SIZE];
        size_t remaining = _logFile.size();
        while (remaining > 0)
        {
            size_t toRead = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            size_t bytesRead = _logFile.read(buffer, toRead);
            if (bytesRead == 0 || bytesRead > toRead) break;
            stream.write(buffer, bytesRead);
            remaining -= bytesRead;
        }
        stream.flush();

        _internalLog("DEBUG", "Log dumped to Stream");
    }

    unsigned long long _getUnixTimeMilliseconds() 
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (static_cast<unsigned long long>(tv.tv_sec) * 1000ULL) + (tv.tv_usec / 1000ULL);
    }

    bool _isValidPath(const char *path)
    {
        const char *invalidChars = "<>:\"\\|?*";
        const char *invalidStartChars = ". ";
        const char *invalidEndChars = " .";
        const int filesystemMaxPathLength = 255;

        for (size_t i = 0; i < strlen(invalidChars); i++)
        {
            if (strchr(path, invalidChars[i]) != nullptr) return false;
        }

        for (size_t i = 0; i < strlen(invalidStartChars); i++)
        {
            if (path[0] == invalidStartChars[i]) return false;
        }

        for (size_t i = 0; i < strlen(invalidEndChars); i++)
        {
            if (path[strlen(path) - 1] == invalidEndChars[i]) return false;
        }

        if (strlen(path) > filesystemMaxPathLength) return false;

        return true;
    }

    bool _ensureDirectoryExists(const char* filePath)
    {
        // Find the last slash
        const char* lastSlash = strrchr(filePath, '/');
        if (!lastSlash) {
            // No directory in path, file is in root
            return true;
        }
        
        // Extract directory path
        size_t dirLen = lastSlash - filePath;
        if (dirLen == 0) {
            _internalLog("DEBUG", "File is in root directory, no directory creation needed");
            return true;
        }
        
        if (dirLen >= MAX_LOG_PATH_LENGTH) {
            _internalLog("DEBUG", "Directory path too long");
            return false;
        }
        
        char dirPath[MAX_LOG_PATH_LENGTH];
        snprintf(dirPath, sizeof(dirPath), "%.*s", (int)dirLen, filePath);
        
        if (LittleFS.mkdir(dirPath)) {
            _internalLog("DEBUG", "Directory created: %s", dirPath);
            return true;
        }
        
        File dir = LittleFS.open(dirPath, "r");
        if (dir && dir.isDirectory()) {
            dir.close();
            _internalLog("DEBUG", "Directory already exists: %s", dirPath);
            return true;
        }
        if (dir) dir.close();

        _internalLog("DEBUG", "Failed to create directory: %s", dirPath);
        return false;
    }

    /**
     * @brief Formats milliseconds with space separators for readability.
     * 
     * Adds spaces every 3 digits (e.g., "1 234 567" instead of "1234567").
     * 
     * @param millisToFormat Milliseconds value to format.
     * @param buffer Buffer to store the formatted string.
     * @param bufferSize Size of the buffer.
     */
    void _formatMillis(unsigned long long millisToFormat, char* buffer, size_t bufferSize) {
        char numStr[MAX_MILLIS_STRING_LENGTH];
        snprintf(numStr, sizeof(numStr), "%llu", millisToFormat);
        int len = strlen(numStr);
        
        int spaces = (len - 1) / 3;
        int resultLen = len + spaces;
        
        if (resultLen + 1 > bufferSize) {
            snprintf(buffer, bufferSize, "%s", numStr);
            return;
        }
        
        int pos = len % 3;
        if (pos == 0) pos = 3;
        
        int bufferPos = 0;
        
        for (int i = 0; i < pos && bufferPos < bufferSize - 1; i++) {
            buffer[bufferPos++] = numStr[i];
        }
        
        while (pos < len && bufferPos < bufferSize - 1) {
            buffer[bufferPos++] = ' ';
            for (int i = 0; i < 3 && pos < len && bufferPos < bufferSize - 1; i++) {
                buffer[bufferPos++] = numStr[pos++];
            }
        }
        
        buffer[bufferPos] = '\0';
    }

    /**
     * @brief Resets all log level counters to zero.
     */
    void resetLogCounters() {
        _verboseCount = 0;
        _debugCount = 0;
        _infoCount = 0;
        _warningCount = 0;
        _errorCount = 0;
        _fatalCount = 0;
        _droppedCount = 0;
        _internalLog("DEBUG", "Log counters reset");
    }

    /**
     * @brief Converts FileMode enum to LittleFS file mode string.
     * @param mode FileMode enum value.
     * @return File mode string for LittleFS operations.
     */
    const char* _fileModeToString(FileMode mode)
    {
        switch (mode) {
            case FileMode::APPEND:  return "a";
            case FileMode::READ:    return "r";
            case FileMode::WRITE:   return "w";
            default:                return "a";
        }
    }

    void _closeLogFile() 
    {
        if (_logFile) {
            _logFile.flush(); // Ensure all data is written before closing
            _logFile.close();
            _currentFileMode = FileMode::APPEND; // Reset to default
        }
    }

    /**
     * @brief Opens the log file with the specified mode.
     * @param mode FileMode (APPEND, READ, or WRITE).
     * @return true if successful, false otherwise.
     */
    bool _reopenLogFile(FileMode mode) 
    {
        _closeLogFile();
        
        if (mode == FileMode::READ && !LittleFS.exists(_logFilePath)) {
            _internalLog("DEBUG", "Log file does not exist for reading: %s", _logFilePath);
            return false;
        }
        
        _logFile = LittleFS.open(_logFilePath, _fileModeToString(mode));
        if (_logFile) {
            _currentFileMode = mode;
            _internalLog("DEBUG", "Log file opened in %s mode: %s", _fileModeToString(mode), _logFilePath);
            return true;
        }

        _internalLog("DEBUG", "Failed to open log file in %s mode: %s", _fileModeToString(mode), _logFilePath);
        _currentFileMode = FileMode::APPEND; // Reset to default on failure
        return false;
    }
}

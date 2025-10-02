#include "AdvancedLogger.h"

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
    static unsigned long _droppedCount = 0;

    // File handling
    File _logFile;
    static FileMode _currentFileMode = FileMode::APPEND;

    // Callback function pointer
    static LogCallback _callback = nullptr;

    // Queue-based logging system
    static QueueHandle_t _logQueue = nullptr;
    static TaskHandle_t _logTaskHandle = nullptr;
    static bool _queueInitialized = false;

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
        
        bool isLogFileOpen = _checkAndOpenLogFile(FileMode::APPEND);
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
        if (_logFile) {
            _internalLog("INFO", "AdvancedLogger ended");
            _closeLogFile();
        } else {
            _internalLog("WARNING", "AdvancedLogger end called but log file was not open");
        }
        
        _destroyLogQueue();
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

    static void _initLogQueue()
    {
        if (_queueInitialized) return; // Already initialized

        // Create the queue for log entries
        size_t queueSize = ADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE / sizeof(LogEntry);
        queueSize = queueSize > 0 ? queueSize : 1; // Ensure at least one entry can be queued
        _logQueue = xQueueCreate(queueSize, sizeof(LogEntry));
        if (!_logQueue) {
            _internalLog("ERROR", "Failed to create log queue");
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
            vQueueDelete(_logQueue);
            _logQueue = nullptr;
            return;
        }

        _queueInitialized = true;
        _internalLog("DEBUG", "Log queue and task initialized successfully");
    }

    static void _destroyLogQueue()
    {
        if (!_queueInitialized) return; // Not initialized

        if (_logTaskHandle) {
            vTaskDelete(_logTaskHandle);
            _logTaskHandle = nullptr;
        }

        if (_logQueue) {
            vQueueDelete(_logQueue);
            _logQueue = nullptr;
        }

        _queueInitialized = false;
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

        while (true) {
            // Wait for a log entry from the queue
            if (xQueueReceive(_logQueue, &entry, portMAX_DELAY) == pdTRUE) {
                _processLogEntry(entry);
            }
        }
    }

    /**
     * @brief Processes a single log entry.
     *
     * This function handles the actual logging operations (console output, file writing, callbacks).
     *
     * @param entry The log entry to process.
     */
    static void _processLogEntry(const LogEntry& entry)
    {
        if (_callback) _callback(entry);

        // Eventual early return
        if ((entry.level < _printLevel) && (entry.level < _saveLevel)) return;

        char messageFormatted[MAX_LOG_LENGTH];

        char timestamp[TIMESTAMP_BUFFER_SIZE];
        getTimestampIsoUtcFromUnixTimeMilliseconds(entry.unixTimeMilliseconds, timestamp, sizeof(timestamp));

        char formattedMillis[MAX_MILLIS_STRING_LENGTH];
        _formatMillis(entry.millis, formattedMillis, sizeof(formattedMillis));

        snprintf(
            messageFormatted,
            sizeof(messageFormatted),
            LOG_PRINT_FORMAT,
            timestamp,
            formattedMillis,
            logLevelToString(entry.level, false),
            entry.coreId,
            entry.file,
            entry.function,
            entry.message);

#ifndef ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING
        if (entry.level >= _printLevel) Serial.println(messageFormatted);
#endif

#ifndef ADVANCED_LOGGER_DISABLE_FILE_LOGGING
        if (entry.level >= _saveLevel) {
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

        // Early return if nothing to do
        if (!_callback && (logLevel < _printLevel) && (logLevel < _saveLevel)) return;

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

        // Check if the queue is full and process one entry to make space and avoid dropping logs
        // This WILL block
        if (uxQueueSpacesAvailable(_logQueue) == 0) {
            _internalLog("DEBUG", "Log queue is full, processing one entry to make space");
            LogEntry processedEntry;
            if (xQueueReceive(_logQueue, &processedEntry, 0) == pdTRUE) {
                _processLogEntry(processedEntry);
            }
        }

        if (xQueueSend(_logQueue, &entry, 0) != pdTRUE) _droppedCount++;
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
        if (!_checkAndOpenLogFile(FileMode::READ)) {
            return 0;
        }

        unsigned int lines = 0;
        unsigned int loopCount = 0;
        while (_logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT)
        {
            loopCount++;
            if (_logFile.read() == '\n')
            {
                lines++;
            }
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

    void clearLog()
    {
        if (!_checkAndOpenLogFile(FileMode::WRITE)) return;

        _closeLogFile();
        _logLines = 0;
        
        // Reopen the log file in append mode for subsequent logging
        _checkAndOpenLogFile(FileMode::APPEND);
        
        _internalLog("INFO", "Log cleared");
    }
    
    void clearLogKeepLatestXPercent(unsigned char percent) 
    {
        if (!_checkAndOpenLogFile(FileMode::READ)) return;

        size_t totalLines = 0;
        char lineBuffer[MAX_MESSAGE_LENGTH];
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

        for (size_t i = 0; i < linesToSkip && _logFile.available(); i++) {
            _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        }

        int loopCount = 0;
        while (_logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT) {
            int bytesRead = _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
            if (bytesRead > 0) {
                lineBuffer[bytesRead] = '\0';
                tempFile.println(lineBuffer);
            }
            loopCount++;
        }

        _closeLogFile();
        tempFile.close();

        LittleFS.remove(_logFilePath);
        LittleFS.rename(tempFilePath, _logFilePath);

        _logLines = linesToKeep;
        
        // Reopen the log file in append mode for subsequent logging
        _checkAndOpenLogFile(FileMode::APPEND);
        
        _internalLog("INFO", "Log cleared keeping latest entries");
    }

    /**
     * @brief Writes a formatted message to the log file.
     * @param messageFormatted The formatted message to save.
     * @param flush Whether to force immediate write to flash storage.
     */
    void _save(const char *messageFormatted, bool flush)
    {
        if (!_checkAndOpenLogFile(FileMode::APPEND)) return;

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

        if (!_checkAndOpenLogFile(FileMode::READ)) return;

        int loopCount = 0;
        while (_logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT)
        {
            loopCount++;
            stream.write(static_cast<uint8_t>(_logFile.read()));
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

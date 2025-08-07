#include "AdvancedLogger.h"

// Macros
#define PROCESS_ARGS(format, line)                       \
    char message[MAX_LOG_LENGTH];                       \
    va_list args;                                        \
    va_start(args, line);                                \
    vsnprintf(message, sizeof(message), format, args); \
    va_end(args);

namespace AdvancedLogger
{    
    static char _logFilePath[MAX_LOG_PATH_LENGTH];

    static LogLevel _printLevel = DEFAULT_PRINT_LEVEL;
    static LogLevel _saveLevel = DEFAULT_SAVE_LEVEL;    
    static unsigned long _maxLogLines = DEFAULT_MAX_LOG_LINES;
    static unsigned long _logLines = 0;

    static unsigned long _verboseCount = 0;
    static unsigned long _debugCount = 0;
    static unsigned long _infoCount = 0;
    static unsigned long _warningCount = 0;
    static unsigned long _errorCount = 0;
    static unsigned long _fatalCount = 0;

    File _logFile;
    static FileMode _currentFileMode = FileMode::APPEND;

    static void _log(const char *message, const char *file, const char *function, int line, LogLevel logLevel);
    static void _increaseLogCount(LogLevel logLevel);
    
    static void _save(const char *messageFormatted, bool flush = false);
    static void _closeLogFile();      
    static bool _reopenLogFile(FileMode mode = FileMode::APPEND);
    static bool _checkAndOpenLogFile(FileMode mode = FileMode::APPEND);
    static const char* _fileModeToString(FileMode mode);
    static bool _setConfigFromPreferences();
    static void _saveConfigToPreferences();

    static unsigned long long _getUnixTimeMilliseconds();

    static bool _isValidPath(const char *path);
    static bool _ensureDirectoryExists(const char* filePath);

    static void _formatMillis(unsigned long long millis, char* buffer, size_t bufferSize);

    static LogCallback _callback = nullptr;
    
    // Recursion guard to prevent infinite loops when internal logging triggers more logging
    static bool _internalLogging = false;

    // Safe internal logging function that doesn't trigger recursion
    static void _internalLog(const char* level, const char* format, ...);

    
    /**
     * @brief Safe internal logging function that doesn't trigger recursion.
     * 
     * This function is used for internal AdvancedLogger operations to avoid
     * infinite recursion when logging operations themselves need to log.
     */
    static void _internalLog(const char* level, const char* format, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.printf("[%s] [AdvancedLogger Internal] %s\n", level, buffer);
    }

    
    /**
     * @brief Initializes the AdvancedLogger object.
     *
     * Initializes the AdvancedLogger object by setting the configuration
     * from the Preferences. If the configuration is not found,
     * the default configuration is used.
     * 
     */
    void begin(const char *logFilePath)
    {
        LOG_DEBUG("AdvancedLogger initializing...");

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
            LOG_DEBUG("Using default config as preferences were not found");
        }

        // Check if LittleFS is already mounted by trying to open a test file
        // If it fails, then we need to mount it
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
            LOG_DEBUG("LittleFS mounted successfully");
        } else {
            LOG_DEBUG("LittleFS already mounted");
        }    
        
        // Ensure the directory for the log file exists
        if (!_ensureDirectoryExists(_logFilePath)) {
            Serial.printf("Failed to create directory for log file %s, falling back to default path\n", _logFilePath);
            // Fall back to default path if custom path fails
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
        
        // Now that the file exists, get the log lines count
        _logLines = getLogLines();
        
        LOG_DEBUG("AdvancedLogger initialized");
    }

    /**
     * @brief Ends the AdvancedLogger object.
     *
     * This method closes the log file if it is open and logs a message indicating
     * that the AdvancedLogger has ended. If the log file was not open, it logs a warning.
     */
    void end()
    {
        if (_logFile) {
            LOG_INFO("AdvancedLogger ended");
            _closeLogFile();
        } else {
            LOG_WARNING("AdvancedLogger end called but log file was not open");
        }
    }

    /**
     * @brief Logs a verbose message.
     *
     * This method logs a verbose message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void verbose(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::VERBOSE);
    }

    /**
     * @brief Logs a debug message.
     *
     * This method logs a debug message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void debug(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::DEBUG);
    }

    /**
     * @brief Logs an info message.
     *
     * This method logs an info message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void info(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::INFO);
    }

    /**
     * @brief Logs a warning message.
     *
     * This method logs a warning message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void warning(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::WARNING);
    }

    /**
     * @brief Logs an error message.
     *
     * This method logs an error message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void error(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::ERROR);
    }

    /**
     * @brief Logs a fatal message.
     *
     * This method logs a fatal message with the provided format and location information.
     *
     * @param format Format of the message.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param ... Arguments to be formatted into the message using the printf format.
    */
    void fatal(const char *format, const char *file, const char *function, int line, ...)
    {
        PROCESS_ARGS(format, line);
        _log(message, file, function, line, LogLevel::FATAL);
    }

    /**
     * @brief Logs a message with a specific log level.
     *
     * This method logs a message with the provided format, location information, and log level.
     *
     * @param message The formatted message to log.
     * @param file Name of the file where the message is logged.
     * @param function Name of the function where the message is logged.
     * @param line Line number where the message is logged.
     * @param logLevel Log level of the message.
    */
    void _log(const char *message, const char *file, const char *function, int line, LogLevel logLevel)
    {
        // Prevent infinite recursion when internal logging operations trigger more logging
        if (_internalLogging) { return; }

        // We increase the log count regardless of the log level.
        _increaseLogCount(logLevel);

        // First we check if we have any callback, of if the loglevel is below the
        // print level or save level. If so, we return early (quickier).
        if (!_callback && (logLevel < _printLevel) && (logLevel < _saveLevel)) return;

        // Use this instead of millis to avoid rollover after 49 days
        unsigned long long unixTimeMs = _getUnixTimeMilliseconds();
        unsigned long long millis = (esp_timer_get_time() / 1000ULL);

        // Always call the callback if it is set, regardless of log level.
        // This allows for external handling of log messages
        if (_callback) {
            _callback(
                LogEntry(
                    unixTimeMs,
                    millis,
                    logLevel,
                    xPortGetCoreID(),
                    file,
                    function,
                    message
                )
            );
        }

        // If the log level is below the print level and save level, we return early
        // to avoid unnecessary processing.
        if ((logLevel < _printLevel) && (logLevel < _saveLevel)) return;

        char messageFormatted[MAX_LOG_LENGTH];

        char timestamp[TIMESTAMP_BUFFER_SIZE];
        getTimestampIsoUtcFromUnixTimeMilliseconds(unixTimeMs, timestamp, sizeof(timestamp));

        char formattedMillis[MAX_MILLIS_STRING_LENGTH];
        _formatMillis(millis, formattedMillis, sizeof(formattedMillis));

        snprintf(
            messageFormatted,
            sizeof(messageFormatted),
            LOG_PRINT_FORMAT,
            timestamp,
            formattedMillis,
            logLevelToString(logLevel, false),
            xPortGetCoreID(),
            file,
            function,
            message);

        Serial.println(messageFormatted);

        if (logLevel >= _saveLevel) _save(messageFormatted);
    }

    /**
     * @brief Increases the log count for a specific log level.
     *
     * This method increases the log count for the specified log level.
     *
     * @param logLevel Log level for which to increase the count.
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

    /**
     * @brief Sets the print level.
     *
     * This method sets the print level to the provided log level.
     *
     * @param logLevel Log level to set.
    */
    void setPrintLevel(LogLevel logLevel)
    {
        _printLevel = logLevel;
        _saveConfigToPreferences();
        LOG_DEBUG("Set print level to %s", logLevelToString(logLevel));
    }

    /**
     * @brief Sets the save level.
     *
     * This method sets the save level to the provided log level.
     *
     * @param logLevel Log level to set.
    */
    void setSaveLevel(LogLevel logLevel)
    {
        _saveLevel = logLevel;
        _saveConfigToPreferences();
        LOG_DEBUG("Set save level to %s", logLevelToString(logLevel));
    }

    /**
     * @brief Gets the print level.
     *
     * This method returns the current print level.
     *
     * @return LogLevel Current print level.
    */
    LogLevel getPrintLevel()
    {
        return _printLevel;
    }

    /**
     * @brief Gets the save level.
     *
     * This method returns the current save level.
     *
     * @return LogLevel Current save level.
    */
    LogLevel getSaveLevel()
    {
        return _saveLevel;
    }

    /**
     * @brief Sets the configuration to default.
     *
     * This method sets the configuration to the default values.
    */
    void setDefaultConfig()
    {
        setPrintLevel(DEFAULT_PRINT_LEVEL);
        setSaveLevel(DEFAULT_SAVE_LEVEL);
        setMaxLogLines(DEFAULT_MAX_LOG_LINES);

        LOG_DEBUG("Config set to default");
    }

    unsigned long getVerboseCount() { return _verboseCount; }
    unsigned long getDebugCount() { return _debugCount; }
    unsigned long getInfoCount() { return _infoCount; }
    unsigned long getWarningCount() { return _warningCount; }
    unsigned long getErrorCount() { return _errorCount; }
    unsigned long getFatalCount() { return _fatalCount; }
    unsigned long getTotalLogCount() { return _verboseCount + _debugCount + _infoCount + _warningCount + _errorCount + _fatalCount; }
    
    void setCallback(LogCallback callback) { _callback = callback; }
    void removeCallback() { _callback = nullptr; }

    /**
     * @brief Sets the configuration from Preferences.
     *
     * This method loads the configuration from ESP32 Preferences (NVS).
     * 
     * @return true if successful, false otherwise
    */
    bool _setConfigFromPreferences()
    {
        Preferences preferences;

        // Try to open preferences in read-write mode (this creates namespace if it doesn't exist)
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
            LOG_DEBUG("Failed to open preferences namespace");
            // Set default values
            _printLevel = DEFAULT_PRINT_LEVEL;
            _saveLevel = DEFAULT_SAVE_LEVEL;
            _maxLogLines = DEFAULT_MAX_LOG_LINES;
            return false;
        }
        
        // Check if this is a fresh namespace by looking for a key that should always exist
        if (!preferences.isKey("printLevel")) {
            LOG_DEBUG("Fresh preferences namespace detected, initializing with defaults");
            // This is a new/empty namespace, set defaults and save them
            preferences.putInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
            preferences.putInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
            preferences.putULong("maxLogLines", DEFAULT_MAX_LOG_LINES);
            
            _printLevel = DEFAULT_PRINT_LEVEL;
            _saveLevel = DEFAULT_SAVE_LEVEL;
            _maxLogLines = DEFAULT_MAX_LOG_LINES;
        } else {
            LOG_DEBUG("Loading existing preferences");
            // Load existing values
            int printLevelInt = preferences.getInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
            _printLevel = static_cast<LogLevel>(printLevelInt);
            
            int saveLevelInt = preferences.getInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
            _saveLevel = static_cast<LogLevel>(saveLevelInt);
            
            _maxLogLines = preferences.getULong("maxLogLines", DEFAULT_MAX_LOG_LINES);
        }
        
        preferences.end();

        LOG_DEBUG("Config loaded from preferences");
        return true;
    }

    /**
     * @brief Saves the configuration to Preferences.
     *
     * This method saves the configuration to ESP32 Preferences (NVS).
    */
    void _saveConfigToPreferences()
    {
        Preferences preferences;

        // Try to open in read-write mode (this will create the namespace if it doesn't exist)
        if (!preferences.begin(PREFERENCES_NAMESPACE, false)) {
            LOG_DEBUG("Failed to open preferences for writing");
            return;
        }
        
        preferences.putInt("printLevel", static_cast<int>(_printLevel));
        preferences.putInt("saveLevel", static_cast<int>(_saveLevel));
        preferences.putULong("maxLogLines", _maxLogLines);
        
        preferences.end();

        LOG_DEBUG("Config saved to preferences");
    }

    /**
     * @brief Sets the maximum number of log lines.
     *
     * This method sets the maximum number of log lines to the provided value.
     *
     * @param maxLogLines Maximum number of log lines.
    */
    void setMaxLogLines(unsigned long maxLogLines)
    {
        LOG_DEBUG("Setting max log lines to %d", maxLogLines);
        _maxLogLines = maxLogLines;
        _saveConfigToPreferences();
    }

    /**
     * @brief Gets the number of log lines.
     *
     * This method returns the number of log lines in the log file.
     *
     * @return unsigned long Number of log lines.
    */
    unsigned long getLogLines()
    {
        if (!_checkAndOpenLogFile(FileMode::READ)) {
            // If file doesn't exist or can't be opened, return 0
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
        
        // Close the file and reopen in append mode for future operations
        _closeLogFile();
        _checkAndOpenLogFile(FileMode::APPEND);
        
        return lines;
    }

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

    /**
     * @brief Clears the log.
     *
     * This method clears the log file.
    */
    void clearLog()
    {
        if (!_checkAndOpenLogFile(FileMode::WRITE)) return;

        // File is truncated when opened in "w" mode, so just close it
        _closeLogFile();
        _logLines = 0;
        _internalLog("INFO", "Log cleared");
    }

    /**
     * @brief Clears the log but keeps the latest X percent of log entries.
     *
     * This method clears the log file but retains the latest X percent of log entries.
     * The default value is 10%.
     */
    void clearLogKeepLatestXPercent(unsigned char percent) 
    {
        if (!_checkAndOpenLogFile(FileMode::READ)) return;

        // Count lines first
        size_t totalLines = 0;
        char lineBuffer[MAX_LOG_LINE_LENGTH];
        while (_logFile.available() && totalLines < MAX_WHILE_LOOP_COUNT) {
            if (_logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1) > 0) {
                totalLines++;
            }
        }
        
        // Reopen for reading from beginning
        if (!_reopenLogFile(FileMode::READ)) return;

        // Min don't work without casting, so we use this poor man's clamping
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

        // Skip lines by reading
        for (size_t i = 0; i < linesToSkip && _logFile.available(); i++) {
            _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        }

        // Direct copy of remaining lines
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
        _internalLog("INFO", "Log cleared keeping latest entries");
    }

    /**
     * @brief Saves a message to the log file.
     *
     * This method saves a message to the log file.
     *
     * @param messageFormatted Formatted message to save.
    */
    void _save(const char *messageFormatted, bool flush)
    {
        if (!_checkAndOpenLogFile(FileMode::APPEND)) return;

        _logFile.println(messageFormatted);
        if (flush) _logFile.flush(); // Ensure data is written to flash
        _logLines++;

        if (_logLines >= _maxLogLines) {
            // Set internal logging flag to prevent recursion during log cleanup
            _internalLogging = true;
            clearLogKeepLatestXPercent();
            _internalLogging = false;
        }
    }

    /**
     * @brief Dumps the log to a Stream.
     *
     * Dump the log to a Stream, such as Serial or an opened file.
     *
     * @param stream Stream to dump the log to.
    */
    void dump(Stream &stream)
    {
        LOG_DEBUG("Dumping log to Stream...");

        if (!_checkAndOpenLogFile(FileMode::READ)) return;

        int loopCount = 0;
        while (_logFile.available() && loopCount < MAX_WHILE_LOOP_COUNT)
        {
            loopCount++;
            stream.write(static_cast<uint8_t>(_logFile.read()));
        }
        stream.flush();

        LOG_DEBUG("Log dumped to Stream");
    }

    /**
     * @brief Gets the current Unix time in milliseconds.
     *
     * This method gets the current Unix time in milliseconds since epoch.
     *
     * @return unsigned long long Current Unix time in milliseconds.
    */
    unsigned long long _getUnixTimeMilliseconds() 
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (static_cast<unsigned long long>(tv.tv_sec) * 1000ULL) + (tv.tv_usec / 1000ULL);
    }

    /**
     * @brief Checks if a path is valid.
     *
     * This method checks if a path is valid.
     *
     * @param path Path to check.
     * @return bool Whether the path is valid.
    */
    bool _isValidPath(const char *path)
    {
        const char *invalidChars = "<>:\"\\|?*";
        const char *invalidStartChars = ". ";
        const char *invalidEndChars = " .";
        const int filesystemMaxPathLength = 255;

        for (size_t i = 0; i < strlen(invalidChars); i++)
        {
            if (strchr(path, invalidChars[i]) != nullptr)
            {
                return false;
            }
        }

        for (size_t i = 0; i < strlen(invalidStartChars); i++)
        {
            if (path[0] == invalidStartChars[i])
            {
                return false;
            }
        }

        for (size_t i = 0; i < strlen(invalidEndChars); i++)
        {
            if (path[strlen(path) - 1] == invalidEndChars[i])
            {
                return false;
            }
        }

        if (strlen(path) > filesystemMaxPathLength)
        {
            return false;
        }

        return true;
    }

    /**
     * @brief Ensures that the directory for the given file path exists.
     *
     * This method creates the directory structure needed for the given file path
     * if it doesn't already exist. Uses LittleFS.mkdir() to create directories.
     *
     * @param filePath Full path to the file (directory will be extracted from this)
     * @return bool Whether the directory exists or was successfully created.
    */
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
            // File is in root directory (path starts with /), no need to create directory
            _internalLog("DEBUG", "File is in root directory, no directory creation needed");
            return true;
        }
        
        if (dirLen >= MAX_LOG_PATH_LENGTH) {
            _internalLog("DEBUG", "Directory path too long");
            return false;
        }
        
        char dirPath[MAX_LOG_PATH_LENGTH];
        snprintf(dirPath, sizeof(dirPath), "%.*s", (int)dirLen, filePath);
        
        // Try to create the directory first - if it already exists, this will fail but that's ok
        // This avoids the error when trying to open a non-existent directory
        if (LittleFS.mkdir(dirPath)) {
            _internalLog("DEBUG", "Directory created: %s", dirPath);
            return true;
        }
        
        // If mkdir failed, check if it's because the directory already exists
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
     * @brief Formats milliseconds with space separators.
     *
     * This method formats milliseconds with space separators every 3 digits.
     *
     * @param millisToFormat Milliseconds to format.
     * @param buffer Buffer to store the formatted string.
     * @param bufferSize Size of the buffer.
    */
    void _formatMillis(unsigned long long millisToFormat, char* buffer, size_t bufferSize) {
        // Convert number to string first
        char numStr[MAX_MILLIS_STRING_LENGTH];
        snprintf(numStr, sizeof(numStr), "%llu", millisToFormat);
        int len = strlen(numStr);
        
        // Calculate how many spaces we need
        int spaces = (len - 1) / 3;
        int resultLen = len + spaces;
        
        // Check if buffer is large enough
        if (resultLen + 1 > bufferSize) {
            // Fallback: just copy the number without spaces
            snprintf(buffer, bufferSize, "%s", numStr);
            return;
        }
        
        // Build the formatted string
        int pos = len % 3;
        if (pos == 0) pos = 3;
        
        int bufferPos = 0;
        
        // Copy first group
        for (int i = 0; i < pos && bufferPos < bufferSize - 1; i++) {
            buffer[bufferPos++] = numStr[i];
        }
        
        // Copy remaining groups with spaces
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
     *
     * This method resets all internal counters that track the number of logs
     * for each level back to zero.
    */
    void resetLogCounters() {
        _verboseCount = 0;
        _debugCount = 0;
        _infoCount = 0;
        _warningCount = 0;
        _errorCount = 0;
        _fatalCount = 0;
    }

    /**
     * @brief Converts FileMode enum to string representation
     * @param mode FileMode enum value
     * @return const char* String representation of the file mode
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

    /**
     * @brief Closes the current log file handle
     */
    void _closeLogFile() 
    {
        if (_logFile) {
            _logFile.close();
            _currentFileMode = FileMode::APPEND; // Reset to default
        }
    }

    /**
     * @brief Reopens the log file with specified mode
     * @param mode FileMode enum value (APPEND, READ, WRITE)
     * @return true if successful, false otherwise
     */
    bool _reopenLogFile(FileMode mode) 
    {
        _closeLogFile();
        
        // If we're trying to read a file that doesn't exist, return false
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

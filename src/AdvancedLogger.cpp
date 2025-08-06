#include "AdvancedLogger.h"

// Macros
#define PROCESS_ARGS(format, function)                   \
    char _message[MAX_LOG_LENGTH];                       \
    va_list args;                                        \
    va_start(args, function);                            \
    vsnprintf(_message, sizeof(_message), format, args); \
    va_end(args);

static const char *TAG = "advancedlogger";

/**
 * @brief Constructs a new AdvancedLogger object.
 *
 * This constructor initializes the AdvancedLogger object with the provided
 * log file path and timestamp format. If the provided path
 * or format are invalid, it uses the default values and logs a warning.
 *
 * @param logFilePath Path to the log file. If invalid, the default path will be used.
 * @param timestampFormat Format for timestamps in the log. If invalid, the default format will be used.
 */
AdvancedLogger::AdvancedLogger(
    const char *logFilePath
)
{
    // Initialize _logFilePath with provided path or default
    if (logFilePath && _isValidPath(logFilePath)) {
        strncpy(_logFilePath, logFilePath, MAX_LOG_PATH_LENGTH - 1);
        _logFilePath[MAX_LOG_PATH_LENGTH - 1] = '\0';
    } else {
        if (logFilePath) {
            Serial.printf(
                "Invalid path for log file %s, using default path: %s\n",
                logFilePath,
                DEFAULT_LOG_PATH
            );
        }
        strncpy(_logFilePath, DEFAULT_LOG_PATH, MAX_LOG_PATH_LENGTH - 1);
        _logFilePath[MAX_LOG_PATH_LENGTH - 1] = '\0';
    }
}

/**
 * @brief Initializes the AdvancedLogger object.
 *
 * Initializes the AdvancedLogger object by setting the configuration
 * from the Preferences. If the configuration is not found,
 * the default configuration is used.
 * 
 */
void AdvancedLogger::begin()
{
    debug("AdvancedLogger initializing...", TAG);

    if (!_setConfigFromPreferences())
    {
        debug("Using default config as preferences were not found", TAG);
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
            error("LittleFS mount failed", TAG);
            return;
        }
        debug("LittleFS mounted successfully", TAG);
    } else {
        debug("LittleFS already mounted", TAG);
    }    
    
    // Ensure the directory for the log file exists
    if (!_ensureDirectoryExists(_logFilePath)) {
        Serial.printf("Failed to create directory for log file %s, falling back to default path\n", _logFilePath);
        // Fall back to default path if custom path fails
        strncpy(_logFilePath, DEFAULT_LOG_PATH, MAX_LOG_PATH_LENGTH - 1);
        _logFilePath[MAX_LOG_PATH_LENGTH - 1] = '\0';
        if (!_ensureDirectoryExists(_logFilePath)) {
            Serial.printf("Failed to create directory for default log file %s\n", _logFilePath);
            error("Log file directory creation failed", TAG);
            return;
        }
    }
    
    bool isLogFileOpen = _checkAndOpenLogFile(FileMode::APPEND);
    if (!isLogFileOpen) {
        Serial.printf("Failed to open log file %s\n", _logFilePath);
        error("Log file opening failed", TAG);
        return;
    }
    
    // Now that the file exists, get the log lines count
    _logLines = getLogLines();
    
    debug("AdvancedLogger initialized", TAG);
}

/**
 * @brief Ends the AdvancedLogger object.
 *
 * This method closes the log file if it is open and logs a message indicating
 * that the AdvancedLogger has ended. If the log file was not open, it logs a warning.
 */
void AdvancedLogger::end()
{
    if (_logFile) {
        info("AdvancedLogger ended", TAG);
        _closeLogFile();
    } else {
        warning("AdvancedLogger end called but log file was not open", TAG);
    }
}

/**
 * @brief Logs a verbose message.
 *
 * This method logs a verbose message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::verbose(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::VERBOSE);
}

/**
 * @brief Logs a debug message.
 *
 * This method logs a debug message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::debug(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::DEBUG);
}

/**
 * @brief Logs an info message.
 *
 * This method logs an info message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::info(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::INFO);
}

/**
 * @brief Logs a warning message.
 *
 * This method logs a warning message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::warning(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::WARNING);
}

/**
 * @brief Logs an error message.
 *
 * This method logs an error message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::error(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::ERROR);
}

/**
 * @brief Logs a fatal message.
 *
 * This method logs a fatal message with the provided format and function name.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::fatal(const char *format, const char *function = "unknown", ...)
{
    PROCESS_ARGS(format, function);
    _log(_message, function, LogLevel::FATAL);
}

/**
 * @brief Logs a message with a specific log level.
 *
 * This method logs a message with the provided format, function name, and log level.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param logLevel Log level of the message.
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::_log(const char *message, const char *function, LogLevel logLevel)
{
    // We increase the log count regardless of the log level.
    _increaseLogCount(logLevel);

    // First we check if we have any callback, of if the loglevel is below the
    // print level or save level. If so, we return early (quickier).
    if (!_callback && (logLevel < _printLevel) && (logLevel < _saveLevel)) return;

    // Pre-allocation to avoid computing twice and ensuring consistent values
    unsigned long _millis = millis();
    char _timestamp[TIMESTAMP_BUFFER_SIZE];
    char _formattedMillis[MAX_MILLIS_STRING_LENGTH];
    unsigned int _coreId = CORE_ID;

    _getTimestampIsoUtc(_timestamp, sizeof(_timestamp));
    _formatMillis(_millis, _formattedMillis, sizeof(_formattedMillis));

    // Always call the callback if it is set, regardless of log level.
    // This allows for external handling of log messages
    if (_callback) {
        _callback(
            _timestamp,
            _millis,
            logLevelToStringLower(logLevel),
            _coreId,
            function,
            message
        );
    }

    // If the log level is below the print level and save level, we return early
    // to avoid unnecessary processing.
    if ((logLevel < _printLevel) && (logLevel < _saveLevel)) return;

    char _messageFormatted[MAX_LOG_LENGTH];

    snprintf(
        _messageFormatted,
        sizeof(_messageFormatted),
        LOG_FORMAT,
        _timestamp,
        _formattedMillis,
        logLevelToString(logLevel, false),
        _coreId,
        function,
        message);

    Serial.println(_messageFormatted);

    if (logLevel >= _saveLevel) _save(_messageFormatted);
}

/**
 * @brief Increases the log count for a specific log level.
 *
 * This method increases the log count for the specified log level.
 *
 * @param logLevel Log level for which to increase the count.
*/
void AdvancedLogger::_increaseLogCount(LogLevel logLevel)
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
 * @brief Logs a message with a specific log level and prints it.
 *
 * This method logs a message with the provided format, function name, and log level.
 * It also prints the message to the Serial monitor.
 *
 * @param format Format of the message.
 * @param function Name of the function where the message is logged. 
 * @param logLevel Log level of the message.
 * @param ... Arguments to be formatted into the message using the printf format.
*/
void AdvancedLogger::_logPrint(const char *format, const char *function, LogLevel logLevel, ...)
{
    if (logLevel < _printLevel && (logLevel < _saveLevel)) return;

    PROCESS_ARGS(format, logLevel);
    char logMessage[MAX_LOG_LENGTH + 200];
    char timestamp[TIMESTAMP_BUFFER_SIZE];
    char formattedMillis[MAX_MILLIS_STRING_LENGTH];

    _getTimestampIsoUtc(timestamp, sizeof(timestamp));
    _formatMillis(millis(), formattedMillis, sizeof(formattedMillis));

    snprintf(
        logMessage,
        sizeof(logMessage),
        LOG_FORMAT,
        timestamp,
        formattedMillis,
        logLevelToString(logLevel, false),
        CORE_ID,
        function,
        _message);

    Serial.println(logMessage);
}

/**
 * @brief Sets the print level.
 *
 * This method sets the print level to the provided log level.
 *
 * @param logLevel Log level to set.
*/
void AdvancedLogger::setPrintLevel(LogLevel logLevel)
{
    debug("Setting print level to %s", TAG, logLevelToString(logLevel));
    _printLevel = logLevel;
    _saveConfigToPreferences();
}

/**
 * @brief Sets the save level.
 *
 * This method sets the save level to the provided log level.
 *
 * @param logLevel Log level to set.
*/
void AdvancedLogger::setSaveLevel(LogLevel logLevel)
{
    debug("Setting save level to %s", TAG, logLevelToString(logLevel));
    _saveLevel = logLevel;
    _saveConfigToPreferences();
}

/**
 * @brief Gets the print level.
 *
 * This method returns the current print level.
 *
 * @return LogLevel Current print level.
*/
LogLevel AdvancedLogger::getPrintLevel()
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
LogLevel AdvancedLogger::getSaveLevel()
{
    return _saveLevel;
}

/**
 * @brief Sets the configuration to default.
 *
 * This method sets the configuration to the default values.
*/
void AdvancedLogger::setDefaultConfig()
{
    debug("Setting config to default...", TAG);

    setPrintLevel(DEFAULT_PRINT_LEVEL);
    setSaveLevel(DEFAULT_SAVE_LEVEL);
    setMaxLogLines(DEFAULT_MAX_LOG_LINES);

    debug("Config set to default", TAG);
}

/**
 * @brief Sets the configuration from Preferences.
 *
 * This method loads the configuration from ESP32 Preferences (NVS).
 * 
 * @return true if successful, false otherwise
*/
bool AdvancedLogger::_setConfigFromPreferences()
{
    debug("Setting config from preferences...", TAG);

    // Try to open preferences in read-write mode (this creates namespace if it doesn't exist)
    if (!_preferences.begin(PREFERENCES_NAMESPACE, false)) {
        debug("Failed to open preferences namespace", TAG);
        // Set default values
        _printLevel = DEFAULT_PRINT_LEVEL;
        _saveLevel = DEFAULT_SAVE_LEVEL;
        _maxLogLines = DEFAULT_MAX_LOG_LINES;
        return false;
    }
    
    // Check if this is a fresh namespace by looking for a key that should always exist
    if (!_preferences.isKey("printLevel")) {
        debug("Fresh preferences namespace detected, initializing with defaults", TAG);
        // This is a new/empty namespace, set defaults and save them
        _preferences.putInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
        _preferences.putInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
        _preferences.putInt("maxLogLines", DEFAULT_MAX_LOG_LINES);
        
        _printLevel = DEFAULT_PRINT_LEVEL;
        _saveLevel = DEFAULT_SAVE_LEVEL;
        _maxLogLines = DEFAULT_MAX_LOG_LINES;
    } else {
        debug("Loading existing preferences", TAG);
        // Load existing values
        int printLevelInt = _preferences.getInt("printLevel", static_cast<int>(DEFAULT_PRINT_LEVEL));
        _printLevel = static_cast<LogLevel>(printLevelInt);
        
        int saveLevelInt = _preferences.getInt("saveLevel", static_cast<int>(DEFAULT_SAVE_LEVEL));
        _saveLevel = static_cast<LogLevel>(saveLevelInt);
        
        _maxLogLines = _preferences.getInt("maxLogLines", DEFAULT_MAX_LOG_LINES);
    }
    
    _preferences.end();

    debug("Config loaded from preferences", TAG);
    return true;
}

/**
 * @brief Saves the configuration to Preferences.
 *
 * This method saves the configuration to ESP32 Preferences (NVS).
*/
void AdvancedLogger::_saveConfigToPreferences()
{
    debug("Saving config to preferences...", TAG);
    
    // Try to open in read-write mode (this will create the namespace if it doesn't exist)
    if (!_preferences.begin(PREFERENCES_NAMESPACE, false)) {
        debug("Failed to open preferences for writing", TAG);
        return;
    }
    
    _preferences.putInt("printLevel", static_cast<int>(_printLevel));
    _preferences.putInt("saveLevel", static_cast<int>(_saveLevel));
    _preferences.putInt("maxLogLines", _maxLogLines);
    
    _preferences.end();

    debug("Config saved to preferences", TAG);
}

/**
 * @brief Sets the maximum number of log lines.
 *
 * This method sets the maximum number of log lines to the provided value.
 *
 * @param maxLogLines Maximum number of log lines.
*/
void AdvancedLogger::setMaxLogLines(int maxLogLines)
{
    char message[MAX_LOG_MESSAGE_LENGTH];
    snprintf(message, sizeof(message), "Setting max log lines to %d", maxLogLines);
    debug(message, TAG);
    _maxLogLines = maxLogLines;
    _saveConfigToPreferences();
}

/**
 * @brief Gets the number of log lines.
 *
 * This method returns the number of log lines in the log file.
 *
 * @return int Number of log lines.
*/
int AdvancedLogger::getLogLines()
{
    if (!_checkAndOpenLogFile(FileMode::READ)) {
        // If file doesn't exist or can't be opened, return 0
        return 0;
    }

    int lines = 0;
    int _loopCount = 0;
    while (_logFile.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
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

bool AdvancedLogger::_checkAndOpenLogFile(FileMode mode)
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
void AdvancedLogger::clearLog()
{
    if (!_checkAndOpenLogFile(FileMode::WRITE)) return;

    // File is truncated when opened in "w" mode, so just close it
    _closeLogFile();
    _logLines = 0;
    _logPrint("Log cleared", TAG, LogLevel::INFO);
}

/**
 * @brief Clears the log but keeps the latest X percent of log entries.
 *
 * This method clears the log file but retains the latest X percent of log entries.
 * The default value is 10%.
 */
void AdvancedLogger::clearLogKeepLatestXPercent(int percent) 
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

    // Calculate lines to keep/skip
    percent = min(max(percent, 0), 100);
    size_t linesToKeep = (totalLines * percent) / 100;
    size_t linesToSkip = totalLines - linesToKeep;

    char tempFilePath[MAX_TEMP_FILE_PATH_LENGTH];
    snprintf(tempFilePath, sizeof(tempFilePath), "%s.tmp", _logFilePath);
    
    File tempFile = LittleFS.open(tempFilePath, "w");
    if (!tempFile) {
        _logPrint("Failed to create temp file", TAG, LogLevel::ERROR);
        _closeLogFile();
        return;
    }

    // Skip lines by reading
    for (size_t i = 0; i < linesToSkip && _logFile.available(); i++) {
        _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
    }

    // Direct copy of remaining lines
    int _loopCount = 0;
    while (_logFile.available() && _loopCount < MAX_WHILE_LOOP_COUNT) {
        int bytesRead = _logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        if (bytesRead > 0) {
            lineBuffer[bytesRead] = '\0';
            tempFile.println(lineBuffer);
        }
        _loopCount++;
    }

    _closeLogFile();
    tempFile.close();

    LittleFS.remove(_logFilePath);
    LittleFS.rename(tempFilePath, _logFilePath);

    _logLines = linesToKeep;
    _logPrint("Log cleared keeping latest entries", TAG, LogLevel::INFO);
}

/**
 * @brief Saves a message to the log file.
 *
 * This method saves a message to the log file.
 *
 * @param messageFormatted Formatted message to save.
*/
void AdvancedLogger::_save(const char *messageFormatted)
{
    if (!_checkAndOpenLogFile(FileMode::APPEND)) return;

    _logFile.println(messageFormatted);
    _logFile.flush(); // Ensure data is written to flash
    _logLines++;

    if (_logLines >= _maxLogLines) clearLogKeepLatestXPercent();
}

/**
 * @brief Dumps the log to a Stream.
 *
 * Dump the log to a Stream, such as Serial or an opened file.
 *
 * @param stream Stream to dump the log to.
*/
void AdvancedLogger::dump(Stream &stream)
{
    debug("Dumping log to Stream...", TAG);

    if (!_checkAndOpenLogFile(FileMode::READ)) return;

    int _loopCount = 0;
    while (_logFile.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        stream.write(static_cast<uint8_t>(_logFile.read()));
    }
    stream.flush();

    debug("Log dumped to Stream", TAG);
}

/**
 * @brief Converts a character to a log level.
 *
 * This method converts a character to a log level.
 *
 * @param logLevelChar Character to convert.
 * @return LogLevel Log level.
*/
LogLevel AdvancedLogger::_charToLogLevel(const char *logLevelChar)
{
    if (strcmp(logLevelChar, "DEBUG") == 0)
        return LogLevel::DEBUG;
    else if (strcmp(logLevelChar, "INFO") == 0)
        return LogLevel::INFO;
    else if (strcmp(logLevelChar, "WARNING") == 0)
        return LogLevel::WARNING;
    else if (strcmp(logLevelChar, "ERROR") == 0)
        return LogLevel::ERROR;
    else if (strcmp(logLevelChar, "FATAL") == 0)
        return LogLevel::FATAL;
    else
        Serial.printf(
            "Unknown log level %s, using default log level %s",
            logLevelChar,
            logLevelToString(DEFAULT_PRINT_LEVEL));
    return DEFAULT_PRINT_LEVEL;
}

/**
 * @brief Gets the timestamp.
 *
 * This method gets the timestamp in ISO UTC format and stores it in the provided buffer.
 *
 * @param buffer Buffer to store the timestamp.
 * @param bufferSize Size of the buffer.
*/
void AdvancedLogger::_getTimestampIsoUtc(char* buffer, size_t bufferSize)
{
    time_t _time = time(nullptr);
    struct tm _timeinfo = *gmtime(&_time);
    strftime(buffer, bufferSize, DEFAULT_TIMESTAMP_FORMAT, &_timeinfo);
}

/**
 * @brief Checks if a path is valid.
 *
 * This method checks if a path is valid.
 *
 * @param path Path to check.
 * @return bool Whether the path is valid.
*/
bool AdvancedLogger::_isValidPath(const char *path)
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
bool AdvancedLogger::_ensureDirectoryExists(const char* filePath)
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
        debug("File is in root directory, no directory creation needed", TAG);
        return true;
    }
    
    if (dirLen >= MAX_LOG_PATH_LENGTH) {
        debug("Directory path too long", TAG);
        return false;
    }
    
    char dirPath[MAX_LOG_PATH_LENGTH];
    strncpy(dirPath, filePath, dirLen);
    dirPath[dirLen] = '\0';
    
    // Try to create the directory first - if it already exists, this will fail but that's ok
    // This avoids the error when trying to open a non-existent directory
    if (LittleFS.mkdir(dirPath)) {
        debug("Directory created: %s", TAG, dirPath);
        return true;
    }
    
    // If mkdir failed, check if it's because the directory already exists
    File dir = LittleFS.open(dirPath, "r");
    if (dir && dir.isDirectory()) {
        dir.close();
        debug("Directory already exists: %s", TAG, dirPath);
        return true;
    }
    if (dir) dir.close();
    
    debug("Failed to create directory: %s", TAG, dirPath);
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
void AdvancedLogger::_formatMillis(unsigned long millisToFormat, char* buffer, size_t bufferSize) {
    // Convert number to string first
    char numStr[16];
    snprintf(numStr, sizeof(numStr), "%lu", millisToFormat);
    int len = strlen(numStr);
    
    // Calculate how many spaces we need
    int spaces = (len - 1) / 3;
    int resultLen = len + spaces;
    
    // Check if buffer is large enough
    if (resultLen + 1 > bufferSize) {
        // Fallback: just copy the number without spaces
        strncpy(buffer, numStr, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
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
void AdvancedLogger::resetLogCounters() {
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
const char* AdvancedLogger::_fileModeToString(FileMode mode)
{
    switch (mode) {
        case FileMode::APPEND: return "a";
        case FileMode::READ:   return "r";
        case FileMode::WRITE:  return "w";
        default:              return "a";
    }
}

/**
 * @brief Closes the current log file handle
 */
void AdvancedLogger::_closeLogFile() 
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
bool AdvancedLogger::_reopenLogFile(FileMode mode) 
{
    _closeLogFile();
    
    // If we're trying to read a file that doesn't exist, return false
    if (mode == FileMode::READ && !LittleFS.exists(_logFilePath)) {
        debug("Log file does not exist for reading: %s", TAG, _logFilePath);
        return false;
    }
    
    _logFile = LittleFS.open(_logFilePath, _fileModeToString(mode));
    if (_logFile) {
        _currentFileMode = mode;
        debug("Log file opened in %s mode: %s", TAG, _fileModeToString(mode), _logFilePath);
        return true;
    }
    
    debug("Failed to open log file in %s mode: %s", TAG, _fileModeToString(mode), _logFilePath);
    _currentFileMode = FileMode::APPEND; // Reset to default on failure
    return false;
}

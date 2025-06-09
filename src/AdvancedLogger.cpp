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
 * log file path, config file path, and timestamp format. If the provided paths
 * or format are invalid, it uses the default values and logs a warning.
 *
 * @param logFilePath Path to the log file. If invalid, the default path will be used.
 * @param configFilePath Path to the config file. If invalid, the default path will be used.
 * @param timestampFormat Format for timestamps in the log. If invalid, the default format will be used.
 */
AdvancedLogger::AdvancedLogger(
    const char *logFilePath,
    const char *configFilePath,
    const char *timestampFormat)
    : _logFilePath(logFilePath),
      _configFilePath(configFilePath),
      _timestampFormat(timestampFormat)
{
    if (!_isValidPath(_logFilePath.c_str()) || !_isValidPath(_configFilePath.c_str()))
    {
        Serial.printf(
            "Invalid path for log %s or config file %s, using default paths: %s and %s",
            _logFilePath.c_str(),
            _configFilePath.c_str(),
            DEFAULT_LOG_PATH,
            DEFAULT_CONFIG_PATH);

        _logFilePath = DEFAULT_LOG_PATH;
        _configFilePath = DEFAULT_CONFIG_PATH;
        _invalidPath = true;
    }

    if (!_isValidTimestampFormat(_timestampFormat))
    {
        Serial.printf(
            "Invalid timestamp format %s, using default format: %s",
            _timestampFormat,
            DEFAULT_TIMESTAMP_FORMAT);

        _timestampFormat = DEFAULT_TIMESTAMP_FORMAT;
        _invalidTimestampFormat = true;
    }
}

/**
 * @brief Initializes the AdvancedLogger object.
 *
 * Initializes the AdvancedLogger object by setting the configuration
 * from the SPIFFS filesystem. If the configuration file is not found,
 * the default configuration is used.
 * 
 */
void AdvancedLogger::begin()
{
    debug("AdvancedLogger initializing...", TAG);

    if (!_setConfigFromSpiffs())
    {
        Serial.printf("Failed to set config from filesystem, using default config");
        setDefaultConfig();
    }
    _logLines = getLogLines();

    if (_invalidPath)
    {
        warning("Invalid path for log or config file, using default paths", TAG);
    }
    if (_invalidTimestampFormat)
    {
        warning("Invalid timestamp format, using default format", TAG);
    }

    bool spiffsMounted = SPIFFS.begin(false);
    if (!spiffsMounted) {
        Serial.printf("Failed to mount SPIFFS. Please mount it before using AdvancedLogger.");
        error("SPIFFS mount failed", TAG);
        return;
    }

    bool isLogFileOpen = _checkAndOpenLogFile();
    if (!isLogFileOpen) {
        Serial.printf("Failed to open log file %s", _logFilePath.c_str());
        error("Log file opening failed", TAG);
        return;
    }

    _logFile = SPIFFS.open(_logFilePath, "a+");
    if (_logFile) {
        debug("AdvancedLogger initialized", TAG);
    } else {
        _logFile = SPIFFS.open(DEFAULT_LOG_PATH, "a+");
        if (!_logFile) {
            Serial.printf("Failed to open default log file %s", DEFAULT_LOG_PATH);
            error("AdvancedLogger initialization failed", TAG);
            return;
        }
        Serial.printf("Log file %s opened successfully", _logFilePath.c_str());
        warning("AdvancedLogger initialized", TAG);
    }
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
        _logFile.close();
        debug("AdvancedLogger ended", TAG);
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
    String _timestamp = _getTimestamp();
    unsigned int _coreId = CORE_ID;

    // Always call the callback if it is set, regardless of log level.
    // This allows for external handling of log messages
    if (_callback) {
        _callback(
            _timestamp.c_str(),
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
        _timestamp.c_str(),
        _formatMillis(_millis).c_str(),
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

    snprintf(
        logMessage,
        sizeof(logMessage),
        LOG_FORMAT,
        _getTimestamp().c_str(),
        _formatMillis(millis()).c_str(),
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
    _saveConfigToSpiffs();
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
    _saveConfigToSpiffs();
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
 * @brief Sets the maximum number of log lines.
 *
 * This method sets the maximum number of log lines to the provided value.
 *
 * @param maxLogLines Maximum number of log lines.
*/
bool AdvancedLogger::_setConfigFromSpiffs()
{
    debug("Setting config from filesystem...", TAG);

    File _file = SPIFFS.open(_configFilePath, "r");
    if (!_file)
    {
        Serial.printf("Failed to open config file for reading");
        _logPrint("Failed to open config file", TAG, LogLevel::ERROR);
        return false;
    }

    int _loopCount = 0;
    while (_file.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        String line = _file.readStringUntil('\n');
        int separatorPosition = line.indexOf('=');
        String key = line.substring(0, separatorPosition);
        String value = line.substring(separatorPosition + 1);
        value.trim();

        if (key == "printLevel")
        {
            setPrintLevel(_charToLogLevel(value.c_str()));
        }
        else if (key == "saveLevel")
        {
            setSaveLevel(_charToLogLevel(value.c_str()));
        }
        else if (key == "maxLogLines")
        {
            setMaxLogLines(value.toInt());
        }
    }

    _file.close();

    debug("Config set from filesystem", TAG);
    return true;
}

/**
 * @brief Saves the configuration to the SPIFFS filesystem.
 *
 * This method saves the configuration to the SPIFFS filesystem.
*/
void AdvancedLogger::_saveConfigToSpiffs()
{
    debug("Saving config to filesystem...", TAG);
    File _file = SPIFFS.open(_configFilePath, "w");
    if (!_file)
    {
        Serial.printf("Failed to open config file for writing");
        _logPrint("Failed to open config file", TAG, LogLevel::ERROR);
        return;
    }

    _file.println(String("printLevel=") + logLevelToString(_printLevel));
    _file.println(String("saveLevel=") + logLevelToString(_saveLevel));
    _file.println(String("maxLogLines=") + String(_maxLogLines));
    _file.close();

    debug("Config saved to filesystem", TAG);
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
    debug(
        ("Setting max log lines to " + String(maxLogLines)).c_str(),
        TAG);
    _maxLogLines = maxLogLines;
    _saveConfigToSpiffs();
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
    if (!_checkAndOpenLogFile()) return 0;

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
    _logFile.close();
    return lines;
}

bool AdvancedLogger::_checkAndOpenLogFile()
{
    if (!_logFile)
    {
        _logFile = SPIFFS.open(_logFilePath, "r");
        if (!_logFile)
        {
            Serial.printf("Failed to open log file for reading");
            _logPrint("Failed to open log file", TAG, LogLevel::ERROR);
            return false;
        }
    }
    return true;
}

/**
 * @brief Clears the log.
 *
 * This method clears the log file.
*/
void AdvancedLogger::clearLog()
{
    if (!_checkAndOpenLogFile()) return;

    _logFile.print("");
    _logFile.close();
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
    if (!_checkAndOpenLogFile()) return;

    // Count lines first
    size_t totalLines = 0;
    while (_logFile.available() && totalLines < MAX_WHILE_LOOP_COUNT) {
        if (_logFile.readStringUntil('\n').length() > 0) totalLines++;
    }
    _logFile.seek(0);

    // Calculate lines to keep/skip
    percent = min(max(percent, 0), 100);
    size_t linesToKeep = (totalLines * percent) / 100;
    size_t linesToSkip = totalLines - linesToKeep;

    File tempFile = SPIFFS.open(_logFilePath + ".tmp", "w");
    if (!tempFile) {
        _logPrint("Failed to create temp file", TAG, LogLevel::ERROR);
        _logFile.close();
        return;
    }

    // Skip lines by reading
    for (size_t i = 0; i < linesToSkip && _logFile.available(); i++) {
        _logFile.readStringUntil('\n');
    }

    // Direct copy of remaining lines
    int _loopCount = 0;
    while (_logFile.available() && _loopCount < MAX_WHILE_LOOP_COUNT) {
        String line = _logFile.readStringUntil('\n');
        if (line.length() > 0) {
            tempFile.print(line);
        }
    }

    _logFile.close();
    tempFile.close();

    SPIFFS.remove(_logFilePath);
    SPIFFS.rename(_logFilePath + ".tmp", _logFilePath);

    _checkAndOpenLogFile();

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
    if (!_checkAndOpenLogFile()) return;

    _logFile.println(messageFormatted);
    _logFile.close();
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

    if (!_checkAndOpenLogFile()) return;

    int _loopCount = 0;
    while (_logFile.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        stream.write(_logFile.read());
    }
    stream.flush();
    _logFile.close();

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
 * This method gets the timestamp.
 *
 * @return String Timestamp.
*/
String AdvancedLogger::_getTimestamp()
{
    char _timestamp[1024];

    time_t _time = time(nullptr);
    struct tm _timeinfo = *localtime(&_time);
    strftime(_timestamp, sizeof(_timestamp), _timestampFormat, &_timeinfo);
    return String(_timestamp);
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
 * @brief Checks if a timestamp format is valid.
 *
 * This method checks if a timestamp format is valid.
 *
 * @param format Timestamp format to check.
 * @return bool Whether the timestamp format is valid.
*/
bool AdvancedLogger::_isValidTimestampFormat(const char *format)
{
    if (_getTimestamp().length() > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Formats milliseconds.
 *
 * This method formats milliseconds.
 *
 * @param millis Milliseconds to format.
 * @return String Formatted milliseconds.
*/
String AdvancedLogger::_formatMillis(unsigned long millisToFormat) {
    String str = String(millisToFormat);
    int len = str.length();
    
    // Pre-calculate final length and reserve space
    int spaces = (len - 1) / 3;
    String result;
    result.reserve(len + spaces);
    
    // Build from left to right to avoid substring overhead
    int pos = len % 3;
    if (pos == 0) pos = 3;
    
    result += str.substring(0, pos);
    while (pos < len) {
        result += " ";
        result += str.substring(pos, pos + 3);
        pos += 3;
    }
    return result;
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

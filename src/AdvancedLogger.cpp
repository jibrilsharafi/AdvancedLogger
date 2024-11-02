#include "AdvancedLogger.h"

// Macros
#define PROCESS_ARGS(format, function)                   \
    char _message[MAX_LOG_LENGTH];                       \
    va_list args;                                        \
    va_start(args, function);                            \
    vsnprintf(_message, sizeof(_message), format, args); \
    va_end(args);

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
    debug("AdvancedLogger initializing...", "AdvancedLogger::begin");

    if (!_setConfigFromSpiffs())
    {
        Serial.printf("Failed to set config from filesystem, using default config");
        setDefaultConfig();
    }
    _logLines = getLogLines();

    if (_invalidPath)
    {
        warning("Invalid path for log or config file, using default paths", "AdvancedLogger::begin");
    }
    if (_invalidTimestampFormat)
    {
        warning("Invalid timestamp format, using default format", "AdvancedLogger::begin");
    }

    debug("AdvancedLogger initialized", "AdvancedLogger::begin");
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
    if ((logLevel < _printLevel) && (logLevel < _saveLevel)) return;

    char _messageFormatted[MAX_LOG_LENGTH];

    snprintf(
        _messageFormatted,
        sizeof(_messageFormatted),
        LOG_FORMAT,
        _getTimestamp().c_str(),
        _formatMillis(millis()).c_str(),
        logLevelToString(logLevel, false),
        CORE_ID,
        function,
        message);

    Serial.println(_messageFormatted);

    if (logLevel >= _saveLevel)
    {
        _save(_messageFormatted);
        if (_logLines >= _maxLogLines)
        {
            clearLogKeepLatestXPercent();
        }
    }

    if (_callback) {
        _callback(
            _getTimestamp().c_str(),
            millis(),
            logLevelToStringLower(logLevel),
            CORE_ID,
            function,
            message
        );
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
    debug("Setting print level to %s", "AdvancedLogger::setPrintLevel", logLevelToString(logLevel));
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
    debug("Setting save level to %s", "AdvancedLogger::setSaveLevel", logLevelToString(logLevel));
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
    debug("Setting config to default...", "AdvancedLogger::setDefaultConfig");

    setPrintLevel(DEFAULT_PRINT_LEVEL);
    setSaveLevel(DEFAULT_SAVE_LEVEL);
    setMaxLogLines(DEFAULT_MAX_LOG_LINES);

    debug("Config set to default", "AdvancedLogger::setDefaultConfig");
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
    debug("Setting config from filesystem...", "AdvancedLogger::_setConfigFromSpiffs");

    File _file = SPIFFS.open(_configFilePath, "r");
    if (!_file)
    {
        Serial.printf("Failed to open config file for reading");
        _logPrint("Failed to open config file", "AdvancedLogger::_setConfigFromSpiffs", LogLevel::ERROR);
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

    debug("Config set from filesystem", "AdvancedLogger::_setConfigFromSpiffs");
    return true;
}

/**
 * @brief Saves the configuration to the SPIFFS filesystem.
 *
 * This method saves the configuration to the SPIFFS filesystem.
*/
void AdvancedLogger::_saveConfigToSpiffs()
{
    debug("Saving config to filesystem...", "AdvancedLogger::_saveConfigToSpiffs");
    File _file = SPIFFS.open(_configFilePath, "w");
    if (!_file)
    {
        Serial.printf("Failed to open config file for writing");
        _logPrint("Failed to open config file", "AdvancedLogger::_saveConfigToSpiffs", LogLevel::ERROR);
        return;
    }

    _file.println(String("printLevel=") + logLevelToString(_printLevel));
    _file.println(String("saveLevel=") + logLevelToString(_saveLevel));
    _file.println(String("maxLogLines=") + String(_maxLogLines));
    _file.close();

    debug("Config saved to filesystem", "AdvancedLogger::_saveConfigToSpiffs");
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
        "AdvancedLogger::setMaxLogLines");
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
    File _file = SPIFFS.open(_logFilePath, "r");
    if (!_file)
    {
        Serial.printf("Failed to open log file for reading");
        _logPrint("Failed to open log file", "AdvancedLogger::getLogLines", LogLevel::ERROR);
        return 0;
    }

    int lines = 0;
    int _loopCount = 0;
    while (_file.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        if (_file.read() == '\n')
        {
            lines++;
        }
    }
    _file.close();
    return lines;
}

/**
 * @brief Clears the log.
 *
 * This method clears the log file.
*/
void AdvancedLogger::clearLog()
{
    File _file = SPIFFS.open(_logFilePath, "w");
    if (!_file)
    {
        Serial.printf("Failed to open log file for writing");
        _logPrint("Failed to open log file", "AdvancedLogger::clearLog", LogLevel::ERROR);
        return;
    }
    _file.print("");
    _file.close();
    _logLines = 0;
    _logPrint("Log cleared", "AdvancedLogger::clearLog", LogLevel::INFO);
}

/**
 * @brief Clears the log but keeps the latest X percent of log entries.
 *
 * This method clears the log file but retains the latest X percent of log entries.
 * The default value is 10%.
 */
void AdvancedLogger::clearLogKeepLatestXPercent(int percent)
{
    File _file = SPIFFS.open(_logFilePath, "r");
    if (!_file)
    {
        Serial.printf("Failed to open log file for reading");
        _logPrint("Failed to open log file", "AdvancedLogger::clearLogKeepLatestXPercent", LogLevel::ERROR);
        return;
    }

    std::vector<String> lines;
    int _loopCount = 0;
    while (_file.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        String line = _file.readStringUntil('\n');
        lines.push_back(line.c_str());
    }
    _file.close();

    size_t totalLines = lines.size();
    percent = min(max(percent, 0), 100);
    size_t linesToKeep = totalLines / 100 * percent;

    _file = SPIFFS.open(_logFilePath, "w");
    if (!_file)
    {
        Serial.printf("Failed to open log file for writing");
        _logPrint("Failed to open log file", "AdvancedLogger::clearLogKeepLatestXPercent", LogLevel::ERROR);
        return;
    }

    for (size_t i = totalLines - linesToKeep; i < totalLines; ++i)
    {
        _file.print(lines[i].c_str());
    }
    _file.close();

    _logLines = linesToKeep;
    _logPrint("Log cleared but kept the latest 10%", "AdvancedLogger::clearLogKeepLatestXPercent", LogLevel::INFO);
}

// ...

/**
 * @brief Saves a message to the log file.
 *
 * This method saves a message to the log file.
 *
 * @param messageFormatted Formatted message to save.
*/
void AdvancedLogger::_save(const char *messageFormatted)
{
    File _file = SPIFFS.open(_logFilePath, "a");
    if (!_file)
    {
        Serial.printf("Failed to open log file for writing");
        _logPrint("Failed to open log file", "AdvancedLogger::_save", LogLevel::ERROR);
        return;
    }
    else
    {
        _file.println(messageFormatted);
        _file.close();
        _logLines++;
    }
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
    debug("Dumping log to Stream...", "AdvancedLogger::dump");

    File _file = SPIFFS.open(_logFilePath, "r");
    if (!_file)
    {
        Serial.printf("Failed to open log file for reading");
        _logPrint("Failed to open log file", "AdvancedLogger::dump", LogLevel::ERROR);
        return;
    }

    int _loopCount = 0;
    while (_file.available() && _loopCount < MAX_WHILE_LOOP_COUNT)
    {
        _loopCount++;
        stream.write(_file.read());
    }
    stream.flush();
    _file.close();

    debug("Log dumped to Stream", "AdvancedLogger::dump");
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
    int n = str.length();
    int insertPosition = n - 3;

    int _loopCount = 0;
    while (insertPosition > 0 && _loopCount < MAX_WHILE_LOOP_COUNT) {
        _loopCount++;
        str = str.substring(0, insertPosition) + " " + str.substring(insertPosition);
        insertPosition -= 3;
    }
    return str;
}

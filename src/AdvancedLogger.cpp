#include "AdvancedLogger.h"

// TODO: Write the documentation for all the functions (say what is optional and what not) and report in README.md
// TODO: add wishlist with new changes done (look at commits)
// TODO: implement better way to write messages (with %s, %d, etc)
// TODO: Implement a buffer
// TODO: update to v1.2.0
// TODO: add created and last modified in examples

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
        LOG_W(
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
        LOG_W(
            "Invalid timestamp format %s, using default format: %s",
            _timestampFormat,
            DEFAULT_TIMESTAMP_FORMAT);

        _timestampFormat = DEFAULT_TIMESTAMP_FORMAT;
        _invalidTimestampFormat = true;
    }
}

void AdvancedLogger::begin()
{
    debug("AdvancedLogger initializing...", "AdvancedLogger::begin");

    if (!_setConfigFromSpiffs())
    {
        LOG_W("Failed to set config from filesystem, using default config");
        setDefaultConfig();
    }
    _logLines = getLogLines();

    if (_invalidPath)
    {
        warning(
            (
                "Invalid path for log or config file, using default paths: " +
                String(DEFAULT_LOG_PATH) +
                " and " +
                String(DEFAULT_CONFIG_PATH))
                .c_str(),
            "AdvancedLogger::begin");
    }
    if (_invalidTimestampFormat)
    {
        warning(
            (
                "Invalid timestamp format, using default format: " +
                String(DEFAULT_TIMESTAMP_FORMAT))
                .c_str(),
            "AdvancedLogger::begin");
    }

    debug("AdvancedLogger initialized", "AdvancedLogger::begin");
}

void AdvancedLogger::verbose(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::VERBOSE, printOnly);
}

void AdvancedLogger::debug(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::DEBUG, printOnly);
}

void AdvancedLogger::info(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::INFO, printOnly);
}

void AdvancedLogger::warning(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::WARNING, printOnly);
}

void AdvancedLogger::error(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::ERROR, printOnly);
}

void AdvancedLogger::fatal(const char *message, const char *function, bool printOnly)
{
    _log(message, function, LogLevel::FATAL, printOnly);
}

void AdvancedLogger::_log(const char *message, const char *function, LogLevel logLevel, bool printOnly)
{
    if (static_cast<int>(logLevel) < static_cast<int>(_printLevel) && (printOnly || static_cast<int>(logLevel) < static_cast<int>(_saveLevel)))
    {
        LOG_D("Message not logged due to log level too low");
        return;
    }

    char _message_formatted[
        30 + // timestamp
        1 + // space
        20 + // millis
        1 + // space
        10 + // log level
        1 + // space
        10 + // core id
        1 + // space
        strlen(function) +
        1 + // space
        strlen(message) +
        1]; // null terminator

    snprintf(
        _message_formatted,
        sizeof(_message_formatted),
        LOG_FORMAT,
        _getTimestamp().c_str(),
        millis(),
        logLevelToString(logLevel, false).c_str(),
        CORE_ID,
        function,
        message);
    
    Serial.println(_message_formatted);

    if (!printOnly && logLevel >= _saveLevel)
    {
        _save(_message_formatted);
        if (_logLines >= _maxLogLines)
        {
            clearLog();
        }
    }
}

void AdvancedLogger::setPrintLevel(LogLevel logLevel)
{
    debug(
        ("Setting print level to " + logLevelToString(logLevel)).c_str(),
        "AdvancedLogger::setPrintLevel");
    _printLevel = logLevel;
    _saveConfigToSpiffs();
}

void AdvancedLogger::setSaveLevel(LogLevel logLevel)
{
    debug(
        ("Setting save level to " + logLevelToString(logLevel)).c_str(),
        "AdvancedLogger::setSaveLevel");
    _saveLevel = logLevel;
    _saveConfigToSpiffs();
}

LogLevel AdvancedLogger::getPrintLevel()
{
    return _printLevel;
}

LogLevel AdvancedLogger::getSaveLevel()
{
    return _saveLevel;
}

void AdvancedLogger::setDefaultConfig()
{
    debug("Setting config to default...", "AdvancedLogger::setDefaultConfig");

    setPrintLevel(DEFAULT_PRINT_LEVEL);
    setSaveLevel(DEFAULT_SAVE_LEVEL);
    setMaxLogLines(DEFAULT_MAX_LOG_LINES);

    debug("Config set to default", "AdvancedLogger::setDefaultConfig");
}

bool AdvancedLogger::_setConfigFromSpiffs()
{
    debug("Setting config from filesystem...", "AdvancedLogger::_setConfigFromSpiffs");

    File _file = Filesystem.open(_configFilePath, "r");
    if (!_file)
    {
        LOG_E("Failed to open config file for reading");
        error("Failed to open config file for reading", "AdvancedLogger::_setConfigFromSpiffs");
        return false;
    }

    while (_file.available())
    {
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

void AdvancedLogger::_saveConfigToSpiffs()
{
    debug("Saving config to filesystem...", "AdvancedLogger::_saveConfigToSpiffs");

    File _file = Filesystem.open(_configFilePath, "w");
    if (!_file)
    {
        LOG_E("Failed to open config file for writing");
        error("Failed to open config file for writing", "AdvancedLogger::_saveConfigToSpiffs");
        return;
    }

    _file.println(String("printLevel=") + logLevelToString(_printLevel));
    _file.println(String("saveLevel=") + logLevelToString(_saveLevel));
    _file.println(String("maxLogLines=") + String(_maxLogLines));
    _file.close();

    debug("Config saved to filesystem", "AdvancedLogger::_saveConfigToSpiffs");
}

void AdvancedLogger::setMaxLogLines(int maxLogLines)
{
    debug(
        ("Setting max log lines to " + String(maxLogLines)).c_str(),
        "AdvancedLogger::setMaxLogLines");
    _maxLogLines = maxLogLines;
    _saveConfigToSpiffs();
}

int AdvancedLogger::getLogLines()
{
    File _file = Filesystem.open(_logFilePath, "r");
    if (!_file)
    {
        LOG_E("Failed to open log file for reading");
        error("Failed to open log file", "AdvancedLogger::getLogLines", true);
        return 0;
    }

    int lines = 0;
    while (_file.available())
    {
        if (_file.read() == '\n')
        {
            lines++;
        }
    }
    _file.close();
    return lines;
}

void AdvancedLogger::clearLog()
{
    File _file = Filesystem.open(_logFilePath, "w");
    if (!_file)
    {
        LOG_E("Failed to open log file for writing");
        error("Failed to open log file", "AdvancedLogger::clearLog", true); // Avoid recursive saving
        return;
    }
    _file.print("");
    _file.close();
    _logLines = 0;
    info("Log cleared", "AdvancedLogger::clearLog", true);
}

void AdvancedLogger::_save(const char *messageFormatted)
{
    File _file = Filesystem.open(_logFilePath, "a");
    if (!_file)
    {
        LOG_E("Failed to open log file for writing");
        error("Failed to open log file", "AdvancedLogger::_save", true); // Avoid recursive saving
        return;
    }
    else
    {
        _file.println(messageFormatted);
        _file.close();
        _logLines++;
    }
}

void AdvancedLogger::dump(Stream &stream)
{
    debug("Dumping log to Stream...", "AdvancedLogger::dump");

    File _file = Filesystem.open(_logFilePath, "r");
    if (!_file)
    {
        LOG_E("Failed to open log file for reading");
        error("Failed to open log file", "AdvancedLogger::dump", true); // Avoid recursive saving
        return;
    }

    while (_file.available())
    {
        stream.write(_file.read());
    }
    stream.flush();
    _file.close();

    debug("Log dumped to Stream", "AdvancedLogger::dump");
}

String AdvancedLogger::logLevelToString(LogLevel logLevel, bool trim)
{
    String logLevelStr;

    switch (logLevel)
    {
    case LogLevel::DEBUG:
        logLevelStr = "DEBUG  ";
        break;
    case LogLevel::INFO:
        logLevelStr = "INFO   ";
        break;
    case LogLevel::WARNING:
        logLevelStr = "WARNING";
        break;
    case LogLevel::ERROR:
        logLevelStr = "ERROR  ";
        break;
    case LogLevel::FATAL:
        logLevelStr = "FATAL  ";
        break;
    default:
        LOG_W("Unknown log level %d", static_cast<int>(logLevel));
        logLevelStr = "UNKNOWN";
        break;
    }

    if (trim)
    {
        logLevelStr.trim();
    }

    return logLevelStr;
}

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
        LOG_W(
            "Unknown log level %s, using default log level %s",
            logLevelChar,
            logLevelToString(DEFAULT_PRINT_LEVEL).c_str());
    return DEFAULT_PRINT_LEVEL;
}

String AdvancedLogger::_getTimestamp()
{
    char _timestamp[1024];

    time_t _time = time(nullptr);
    struct tm _timeinfo = *localtime(&_time);
    strftime(_timestamp, sizeof(_timestamp), _timestampFormat, &_timeinfo);
    return String(_timestamp);
}

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
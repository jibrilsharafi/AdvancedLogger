#include "AdvancedLogger.h"

// TODO: Write the documentation for all the functions (say what is optional and what not) and report in README.md
// TODO: add wishlist with new changes done (look at commits)
// TODO: implement better way to write messages (with %s, %d, etc)
// TODO: make the FS optional
// TODO: allow for adding a custom Serial object
// TODO: Implement a buffer
// TODO: Dump to any serial or any file

AdvancedLogger::AdvancedLogger(
    FS* fs,
    const char *logFilePath,
    const char *configFilePath,
    const char *timestampFormat)
    : _logFilePath(logFilePath),
      _configFilePath(configFilePath),
      _timestampFormat(timestampFormat)
{
    if (fs != nullptr)
    {
        _fs = fs;
        _shouldLogToFile = true;
    }

    if (!_isValidPath(_logFilePath.c_str()) || !_isValidPath(_configFilePath.c_str()))
    {
        log_w(
            "Invalid path for log %s or config file %s, using default paths: %s and %s",
            _logFilePath,
            _configFilePath,
            DEFAULT_LOG_PATH,
            DEFAULT_CONFIG_PATH);

        _logFilePath = DEFAULT_LOG_PATH;
        _configFilePath = DEFAULT_CONFIG_PATH;
        _invalidPath = true;
    }

    if (!_isValidTimestampFormat(_timestampFormat))
    {
        log_w(
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

    if (!_setConfigFromFs())
    {
        log_w("Failed to set config from filesystem, using default config");
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

    if (!_shouldLogToFile)
    {
        log_w("File system not passed to constructor, logging to file disabled");
        warning("Logging to file disabled as file system not passed to constructor", "AdvancedLogger::begin");
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
    logLevel = _saturateLogLevel(logLevel);
    if (static_cast<int>(logLevel) < static_cast<int>(_printLevel) && (printOnly || static_cast<int>(logLevel) < static_cast<int>(_saveLevel)))
    {
        log_d("Message not logged due to log level too low");
        return;
    }

    char _message_formatted[1024]; // 1024 is a safe value

    snprintf(
        _message_formatted,
        sizeof(_message_formatted),
        LOG_FORMAT,
        _getTimestamp().c_str(),
        millis(),
        _logLevelToString(logLevel).c_str(),
        xPortGetCoreID(),
        function,
        message);

    Serial.println(_message_formatted);

    if (!printOnly && logLevel >= _saveLevel && _shouldLogToFile)
    {
        _save(_message_formatted);
        if (_logLines >= _maxLogLines)
        {
            _logLines = 0;
            clearLog();
            warning(
                ("Log cleared due to max log lines reached (" + String(_maxLogLines) + ")").c_str(),
                "AdvancedLogger::_log");
        }
    }
}

void AdvancedLogger::setPrintLevel(LogLevel logLevel)
{
    info(
        ("Setting print level to " + _logLevelToString(logLevel)).c_str(),
        "AdvancedLogger::setPrintLevel");
    _printLevel = _saturateLogLevel(logLevel);
    _saveConfigToFs();
}

void AdvancedLogger::setSaveLevel(LogLevel logLevel)
{
    info(
        ("Setting save level to " + _logLevelToString(logLevel)).c_str(),
        "AdvancedLogger::setSaveLevel");
    _saveLevel = _saturateLogLevel(logLevel);
    _saveConfigToFs();
}

String AdvancedLogger::getPrintLevel()
{
    return _logLevelToString(_printLevel);
}

String AdvancedLogger::getSaveLevel()
{
    return _logLevelToString(_saveLevel);
}

void AdvancedLogger::setDefaultConfig()
{
    setPrintLevel(DEFAULT_PRINT_LEVEL);
    setSaveLevel(DEFAULT_SAVE_LEVEL);
    setMaxLogLines(DEFAULT_MAX_LOG_LINES);

    info("Config set to default", "AdvancedLogger::setDefaultConfig");
}

bool AdvancedLogger::_setConfigFromFs()
{
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return false;
    }

    File _file = _fs->open(_configFilePath, "r");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
        error("Failed to open config file for reading", "AdvancedLogger::_setConfigFromFs");
        return false;
    }

    while (_file.available())
    {
        String line = _file.readStringUntil('\n');
        int separatorPosition = line.indexOf('=');
        String key = line.substring(0, separatorPosition);
        String value = line.substring(separatorPosition + 1);

        if (key == "printLevel")
        {
            setPrintLevel(_stringToLogLevel(value));
        }
        else if (key == "saveLevel")
        {
            setSaveLevel(_stringToLogLevel(value));
        }
        else if (key == "maxLogLines")
        {
            setMaxLogLines(value.toInt());
        }
    }

    _file.close();
    debug("Config set from filesystem", "AdvancedLogger::_setConfigFromFs");
    return true;
}

void AdvancedLogger::_saveConfigToFs()
{
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return;
    }

    File _file = _fs->open(_configFilePath, "w");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
        error("Failed to open config file for writing", "AdvancedLogger::_saveConfigToFs");
        return;
    }

    _file.println(String("printLevel=") + _logLevelToString(_printLevel));
    _file.println(String("saveLevel=") + _logLevelToString(_saveLevel));
    _file.println(String("maxLogLines=") + String(_maxLogLines));
    _file.close();
    debug("Config saved to filesystem", "AdvancedLogger::_saveConfigToFs");
}

void AdvancedLogger::setMaxLogLines(int maxLines)
{
    info(
        ("Setting max log lines to " + String(maxLines)).c_str(),
        "AdvancedLogger::setMaxLogLines");
    _maxLogLines = maxLines;
    _saveConfigToFs();
}

int AdvancedLogger::getLogLines()
{
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return -1;
    }

    File _file = _fs->open(_logFilePath, "r");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
        error("Failed to open log file", "AdvancedLogger::getLogLines", true);
        return -1;
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
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return;
    }

    warning("Clearing log", "AdvancedLogger::clearLog", true); // Avoid recursive saving
    _fs->remove(_logFilePath);
    File _file = _fs->open(_logFilePath, "w");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
        error("Failed to open log file", "AdvancedLogger::clearLog", true); // Avoid recursive saving
        return;
    }
    _file.close();
    warning("Log cleared", "AdvancedLogger::clearLog");
}

void AdvancedLogger::_save(const char *messageFormatted)
{
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return;
    }

    File _file = _fs->open(_logFilePath, "a");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
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

void AdvancedLogger::dumpToSerial()
{
    if (!_shouldLogToFile) {
        log_d("Skipping file system as it has not been passed to the constructor");
        return;
    }

    info("Dumping log to Serial", "AdvancedLogger::dumpToSerial", true);

    for (int i = 0; i < 2 * 50; i++)
        Serial.print("_");
    Serial.println();

    File _file = _fs->open(_logFilePath, "r");
    if (!_file)
    {
        log_e("Failed to open config file for reading");
        error("Failed to open log file", "AdvancedLogger::dumpToSerial", true); // Avoid recursive saving
        return;
    }
    while (_file.available())
    {
        Serial.write(_file.read());
        Serial.flush();
    }
    _file.close();

    for (int i = 0; i < 2 * 50; i++)
        Serial.print("_");
    Serial.println();

    info("Log dumped to Serial", "AdvancedLogger::dumpToSerial", true);
}

String AdvancedLogger::_logLevelToString(LogLevel logLevel)
{
    switch (logLevel)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        log_w("Unknown log level %d", static_cast<int>(logLevel));
        return "UNKNOWN";
    }
}

LogLevel AdvancedLogger::_stringToLogLevel(const String &logLevelStr)
{
    if (logLevelStr == "DEBUG")
        return LogLevel::DEBUG;
    else if (logLevelStr == "INFO")
        return LogLevel::INFO;
    else if (logLevelStr == "WARNING")
        return LogLevel::WARNING;
    else if (logLevelStr == "ERROR")
        return LogLevel::ERROR;
    else if (logLevelStr == "FATAL")
        return LogLevel::FATAL;
    else
        log_w("Unknown log level %s, using default log level %s", logLevelStr, _logLevelToString(DEFAULT_PRINT_LEVEL));
    return DEFAULT_PRINT_LEVEL;
}

LogLevel AdvancedLogger::_saturateLogLevel(LogLevel logLevel)
{
    return static_cast<LogLevel>(
        max(
            min(
                static_cast<int>(logLevel),
                static_cast<int>(LogLevel::FATAL)),
            static_cast<int>(LogLevel::DEBUG)));
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

    for (int i = 0; i < strlen(invalidChars); i++)
    {
        if (strchr(path, invalidChars[i]) != nullptr)
        {
            return false;
        }
    }

    for (int i = 0; i < strlen(invalidStartChars); i++)
    {
        if (path[0] == invalidStartChars[i])
        {
            return false;
        }
    }

    for (int i = 0; i < strlen(invalidEndChars); i++)
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
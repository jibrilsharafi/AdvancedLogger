#include "AdvancedLogger.h"

AdvancedLogger::AdvancedLogger(const char *logFilePath, const char *configFilePath)
    : _logFilePath(logFilePath), _configFilePath(configFilePath)
{
    _printLevel = ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL;
    _saveLevel = ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL;
    _maxLogLines = ADVANCEDLOGGER_DEFAULT_MAX_LOG_LINES;

    _logLines = 0;
}

void AdvancedLogger::begin()
{
    log("Initializing AdvancedLogger...", "AdvancedLogger::begin", ADVANCEDLOGGER_DEBUG);

    if (!_setConfigFromSpiffs())
    {
        setDefaultLogLevels();
    }
    _logLines = getLogLines();

    log("AdvancedLogger initialized", "AdvancedLogger::begin", ADVANCEDLOGGER_DEBUG);
}

void AdvancedLogger::log(const char *message, const char *function, int logLevel)
{
    logLevel = _saturateLogLevel(logLevel);
    if (logLevel < _printLevel && logLevel < _saveLevel)
    {
        return;
    }

    char _message_formatted[26 + strlen(message) + strlen(function) + 70]; // 26 = length of timestamp, 70 = length of log level

    snprintf(
        _message_formatted,
        sizeof(_message_formatted),
        ADVANCEDLOGGER_FORMAT,
        _getTimestamp().c_str(),
        millis(),
        _logLevelToString(logLevel).c_str(),
        xPortGetCoreID(),
        function,
        message);

    if (logLevel >= _printLevel)
    {
        Serial.println(_message_formatted);
    }

    if (logLevel >= _saveLevel)
    {
        _save(_message_formatted);
        if (_logLines >= _maxLogLines)
        {
            _logLines = 0;
            clearLog();
            log(
                ("Log cleared due to max log lines (" + String(_maxLogLines) + ") reached").c_str(),
                "AdvancedLogger::log",
                ADVANCEDLOGGER_WARNING);
        }
    }
}

void AdvancedLogger::logOnly(const char *message, const char *function, int logLevel)
{
    logLevel = _saturateLogLevel(logLevel);
    if (logLevel < _printLevel)
    {
        return;
    }

    char _message_formatted[26 + strlen(message) + strlen(function) + 70]; // 26 = length of timestamp, 70 = length of log level

    snprintf(
        _message_formatted,
        sizeof(_message_formatted),
        ADVANCEDLOGGER_FORMAT,
        _getTimestamp().c_str(),
        millis(),
        _logLevelToString(logLevel).c_str(),
        xPortGetCoreID(),
        function,
        message);

    if (logLevel >= _printLevel)
    {
        Serial.println(_message_formatted);
    }
}

void AdvancedLogger::setPrintLevel(int level)
{
    log(
        ("Setting print level to " + String(level)).c_str(),
        "AdvancedLogger::setPrintLevel",
        ADVANCEDLOGGER_INFO);
    _printLevel = _saturateLogLevel(level);
    _saveConfigToSpiffs();
}

void AdvancedLogger::setSaveLevel(int level)
{
    log(
        ("Setting save level to " + String(level)).c_str(),
        "AdvancedLogger::setSaveLevel",
        ADVANCEDLOGGER_INFO);
    _saveLevel = _saturateLogLevel(level);
    _saveConfigToSpiffs();
}

String AdvancedLogger::getPrintLevel()
{
    return _logLevelToString(_printLevel);
}

String AdvancedLogger::getSaveLevel()
{
    return _logLevelToString(_saveLevel);
}

void AdvancedLogger::setDefaultLogLevels()
{
    setPrintLevel(ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL);
    setSaveLevel(ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL);
    setMaxLogLines(ADVANCEDLOGGER_DEFAULT_MAX_LOG_LINES);

    log("Log levels set to default", "AdvancedLogger::setDefaultLogLevels", ADVANCEDLOGGER_INFO);
}

bool AdvancedLogger::_setConfigFromSpiffs()
{
    File _file = SPIFFS.open(_configFilePath, "r");
    if (!_file)
    {
        log("Failed to open config file for reading", "AdvancedLogger::_setConfigFromSpiffs", ADVANCEDLOGGER_ERROR);
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
            setPrintLevel(value.toInt());
        }
        else if (key == "saveLevel")
        {
            setSaveLevel(value.toInt());
        }
        else if (key == "maxLogLines")
        {
            setMaxLogLines(value.toInt());
        }
    }

    _file.close();
    log("Log levels set from SPIFFS", "AdvancedLogger::_setConfigFromSpiffs", ADVANCEDLOGGER_DEBUG);
    return true;
}

void AdvancedLogger::_saveConfigToSpiffs()
{
    File _file = SPIFFS.open(_configFilePath, "w");
    if (!_file)
    {
        log("Failed to open config file for writing", "AdvancedLogger::_saveConfigToSpiffs", ADVANCEDLOGGER_ERROR);
        return;
    }

    _file.println(String("printLevel=") + String(_printLevel));
    _file.println(String("saveLevel=") + String(_saveLevel));
    _file.println(String("maxLogLines=") + String(_maxLogLines));
    _file.close();
    log("Log levels saved to SPIFFS", "AdvancedLogger::_saveConfigToSpiffs", ADVANCEDLOGGER_DEBUG);
}

void AdvancedLogger::setMaxLogLines(int maxLines)
{
    log(
        ("Setting max log lines to " + String(maxLines)).c_str(),
        "AdvancedLogger::setMaxLogLines",
        ADVANCEDLOGGER_INFO);
    _maxLogLines = maxLines;
    _saveConfigToSpiffs();
}

int AdvancedLogger::getLogLines()
{
    File _file = SPIFFS.open(_logFilePath, "r");
    if (!_file)
    {
        logOnly("Failed to open log file", "AdvancedLogger::getLogLines", ADVANCEDLOGGER_ERROR);
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
    logOnly("Clearing log", "AdvancedLogger::clearLog", ADVANCEDLOGGER_WARNING);
    SPIFFS.remove(_logFilePath);
    File _file = SPIFFS.open(_logFilePath, "w");
    if (!_file)
    {
        logOnly("Failed to open log file", "AdvancedLogger::clearLog", ADVANCEDLOGGER_ERROR);
        return;
    }
    _file.close();
    log("Log cleared", "AdvancedLogger::clearLog", ADVANCEDLOGGER_WARNING);
}

void AdvancedLogger::_save(const char *messageFormatted)
{
    File _file = SPIFFS.open(_logFilePath, "a");
    if (_file)
    {
        _file.println(messageFormatted);
        _file.close();
        _logLines++;
    }
    else
    {
        logOnly("Failed to open log file", "AdvancedLogger::_save", ADVANCEDLOGGER_ERROR);
    }
}

void AdvancedLogger::dumpToSerial()
{
    logOnly(
        "Dumping log to Serial",
        "AdvancedLogger::dumpToSerial",
        ADVANCEDLOGGER_INFO);

    for (int i = 0; i < 2 * 50; i++)
        Serial.print("_");
    Serial.println();

    File _file = SPIFFS.open(_logFilePath, "r");
    if (!_file)
    {
        logOnly("Failed to open log file", "AdvancedLogger::dumpToSerial", ADVANCEDLOGGER_ERROR);
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

    logOnly(
        "Log dumped to Serial",
        "AdvancedLogger::dumpToSerial",
        ADVANCEDLOGGER_INFO);
}

String AdvancedLogger::_logLevelToString(int logLevel)
{
    switch (logLevel)
    {
    case ADVANCEDLOGGER_VERBOSE:
        return "VERBOSE";
    case ADVANCEDLOGGER_DEBUG:
        return "DEBUG";
    case ADVANCEDLOGGER_INFO:
        return "INFO";
    case ADVANCEDLOGGER_WARNING:
        return "WARNING";
    case ADVANCEDLOGGER_ERROR:
        return "ERROR";
    case ADVANCEDLOGGER_FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

int AdvancedLogger::_saturateLogLevel(int logLevel)
{
    return min(max(logLevel, ADVANCEDLOGGER_VERBOSE), ADVANCEDLOGGER_FATAL);
}

String AdvancedLogger::_getTimestamp()
{
    struct tm *_timeinfo;
    char _timestamp[26];

    long _time = static_cast<long>(time(nullptr));
    _timeinfo = localtime(&_time);
    strftime(_timestamp, sizeof(_timestamp), ADVANCEDLOGGER_TIMESTAMP_FORMAT, _timeinfo);
    return String(_timestamp);
}

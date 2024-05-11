#include "advancedLogger.h"

AdvancedLogger::AdvancedLogger()
{
    _printLevel = ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL;
    _saveLevel = ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL;
}

void AdvancedLogger::begin()
{
    if (!setLogLevelsFromSpiffs())
    {
        setDefaultLogLevels();
    }
    log("AdvancedLogger initialized", "advancedLogger.cpp::begin", ADVANCEDLOGGER_DEBUG);
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
        "advancedLogger.cpp::setPrintLevel",
        ADVANCEDLOGGER_INFO);
    _printLevel = _saturateLogLevel(level);
    _saveLogLevelsToSpiffs();
}

void AdvancedLogger::setSaveLevel(int level)
{
    log(
        ("Setting save level to " + String(level)).c_str(),
        "advancedLogger.cpp::setSaveLevel",
        ADVANCEDLOGGER_INFO);
    _saveLevel = _saturateLogLevel(level);
    _saveLogLevelsToSpiffs();
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
    log("Log levels set to default", "advancedLogger.cpp::setDefaultLogLevels", ADVANCEDLOGGER_DEBUG);
}

bool AdvancedLogger::setLogLevelsFromSpiffs()
{
    log("Deserializing JSON from SPIFFS", "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_DEBUG);

    File _file = SPIFFS.open(ADVANCEDLOGGER_CONFIG_PATH, "r");
    if (!_file)
    {
        log(
            ("Failed to open file " + String(ADVANCEDLOGGER_CONFIG_PATH)).c_str(),
            "utils::deserializeJsonFromSpiffs",
            ADVANCEDLOGGER_ERROR);
        return false;
    }
    JsonDocument _jsonDocument;

    DeserializationError _error = deserializeJson(_jsonDocument, _file);
    _file.close();
    if (_error)
    {
        log(
            ("Failed to deserialize file " + String(ADVANCEDLOGGER_CONFIG_PATH) + ". Error: " + String(_error.c_str())).c_str(),
            "utils::deserializeJsonFromSpiffs",
            ADVANCEDLOGGER_ERROR);
        return false;
    }

    log("JSON deserialized from SPIFFS correctly", "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_DEBUG);

    if (_jsonDocument.isNull())
    {
        return false;
    }
    setPrintLevel(_jsonDocument["level"]["print"].as<int>());
    setSaveLevel(_jsonDocument["level"]["save"].as<int>());
    log("Log levels set from SPIFFS", "advancedLogger.cpp::setLogLevelsFromSpiffs", ADVANCEDLOGGER_DEBUG);

    return true;
}

void AdvancedLogger::clearLog()
{
    logOnly("Clearing log", "advancedLogger.cpp::clearLog", ADVANCEDLOGGER_WARNING);
    SPIFFS.remove(ADVANCEDLOGGER_LOG_PATH);
    File _file = SPIFFS.open(ADVANCEDLOGGER_LOG_PATH, "w");
    if (!_file)
    {
        logOnly("Failed to open log file", "advancedLogger.cpp::clearLog", ADVANCEDLOGGER_ERROR);
        return;
    }
    _file.close();
    log("Log cleared", "advancedLogger.cpp::clearLog", ADVANCEDLOGGER_WARNING);
}

void AdvancedLogger::_save(const char *messageFormatted)
{
    File file = SPIFFS.open(ADVANCEDLOGGER_LOG_PATH, "a");
    if (file)
    {

        file.println(messageFormatted);
        file.close();
    }
    else
    {
        logOnly("Failed to open log file", "advancedLogger.cpp::_save", ADVANCEDLOGGER_ERROR);
    }
}

void AdvancedLogger::_saveLogLevelsToSpiffs()
{
    JsonDocument _jsonDocument;
    _jsonDocument["level"]["print"] = _printLevel;
    _jsonDocument["level"]["save"] = _saveLevel;
    File _file = SPIFFS.open(ADVANCEDLOGGER_CONFIG_PATH, "w");
    if (!_file)
    {
        log("Failed to open logger.json", "advancedLogger.cpp::_saveLogLevelsToSpiffs", ADVANCEDLOGGER_ERROR);
        return;
    }
    serializeJson(_jsonDocument, _file);
    _file.close();
    log("Log levels saved to SPIFFS", "advancedLogger.cpp::_saveLogLevelsToSpiffs", ADVANCEDLOGGER_DEBUG);
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

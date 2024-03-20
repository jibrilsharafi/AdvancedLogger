#include "advancedLogger.h"

AdvancedLogger::AdvancedLogger()
{
    _print_level = ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL;
    _save_level = ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL;
}

void AdvancedLogger::begin()
{
    if (!setLogLevelsFromSpiffs())
    {
        setDefaultLogLevels();
    }
    log("AdvancedLogger initialized", "AdvancedLogger::begin", ADVANCEDLOGGER_DEBUG);
}

void AdvancedLogger::log(const char *message, const char *function, int logLevel)
{
    logLevel = _saturateLogLevel(logLevel);
    if (logLevel < _print_level && logLevel < _save_level)
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

    if (logLevel >= _print_level)
    {
        Serial.println(_message_formatted);
    }

    if (logLevel >= _save_level)
    {
        _save(_message_formatted);
    }
}

void AdvancedLogger::logOnly(const char *message, const char *function, int logLevel)
{
    logLevel = _saturateLogLevel(logLevel);
    if (logLevel < _print_level)
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

    if (logLevel >= _print_level)
    {
        Serial.println(_message_formatted);
    }
}

void AdvancedLogger::setPrintLevel(int level)
{
    char _buffer[50];
    snprintf(_buffer, sizeof(_buffer), "Setting print level to %d", level);
    log(_buffer, "AdvancedLogger::setPrintLevel", ADVANCEDLOGGER_WARNING);
    _print_level = _saturateLogLevel(level);
    _saveLogLevelsToSpiffs();
}

void AdvancedLogger::setSaveLevel(int level)
{
    char _buffer[50];
    snprintf(_buffer, sizeof(_buffer), "Setting save level to %d", level);
    log(_buffer, "AdvancedLogger::setSaveLevel", ADVANCEDLOGGER_WARNING);
    _save_level = _saturateLogLevel(level);
    _saveLogLevelsToSpiffs();
}

String AdvancedLogger::getPrintLevel()
{
    return _logLevelToString(_print_level);
}

String AdvancedLogger::getSaveLevel()
{
    return _logLevelToString(_save_level);
}

void AdvancedLogger::setDefaultLogLevels()
{
    setPrintLevel(ADVANCEDLOGGER_DEFAULT_PRINT_LEVEL);
    setSaveLevel(ADVANCEDLOGGER_DEFAULT_SAVE_LEVEL);
    log("Log levels set to default", "AdvancedLogger::setDefaultLogLevels", ADVANCEDLOGGER_DEBUG);
}

bool AdvancedLogger::setLogLevelsFromSpiffs()
{
    log("Deserializing JSON from SPIFFS", "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_DEBUG);

    File _file = SPIFFS.open(ADVANCEDLOGGER_CONFIG_PATH, "r");
    if (!_file)
    {
        char _buffer[50 + strlen(ADVANCEDLOGGER_CONFIG_PATH)];
        snprintf(_buffer, sizeof(_buffer), "Failed to open file %s", ADVANCEDLOGGER_CONFIG_PATH);
        log(_buffer, "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_ERROR);
        return false;
    }
    JsonDocument _jsonDocument;

    DeserializationError _error = deserializeJson(_jsonDocument, _file);
    _file.close();
    if (_error)
    {
        char _buffer[50 + strlen(ADVANCEDLOGGER_CONFIG_PATH) + strlen(_error.c_str())];
        snprintf(_buffer, sizeof(_buffer), "Failed to deserialize file %s. Error: %s", ADVANCEDLOGGER_CONFIG_PATH, _error.c_str());
        log(_buffer, "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_ERROR);
        return false;
    }

    log("JSON deserialized from SPIFFS correctly", "utils::deserializeJsonFromSpiffs", ADVANCEDLOGGER_DEBUG);
    serializeJson(_jsonDocument, Serial);
    Serial.println();

    if (_jsonDocument.isNull())
    {
        return false;
    }
    setPrintLevel(_jsonDocument["level"]["print"].as<int>());
    setSaveLevel(_jsonDocument["level"]["save"].as<int>());
    log("Log levels set from SPIFFS", "AdvancedLogger::setLogLevelsFromSpiffs", ADVANCEDLOGGER_DEBUG);

    return true;
}

void AdvancedLogger::clearLog()
{
    logOnly("Clearing log", "AdvancedLogger::clearLog", ADVANCEDLOGGER_WARNING);
    SPIFFS.remove(ADVANCEDLOGGER_LOG_PATH);
    File _file = SPIFFS.open(ADVANCEDLOGGER_LOG_PATH, "w");
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
    File file = SPIFFS.open(ADVANCEDLOGGER_LOG_PATH, "a");
    if (file)
    {

        file.println(messageFormatted);
        file.close();
    }
    else
    {
        logOnly("Failed to open log file", "AdvancedLogger::_save", ADVANCEDLOGGER_ERROR);
    }
}

void AdvancedLogger::_saveLogLevelsToSpiffs()
{
    JsonDocument _jsonDocument;
    _jsonDocument["level"]["print"] = _print_level;
    _jsonDocument["level"]["save"] = _save_level;
    File _file = SPIFFS.open(ADVANCEDLOGGER_CONFIG_PATH, "w");
    if (!_file)
    {
        log("Failed to open logger.json", "AdvancedLogger::_saveLogLevelsToSpiffs", ADVANCEDLOGGER_ERROR);
        return;
    }
    serializeJson(_jsonDocument, _file);
    _file.close();
    log("Log levels saved to SPIFFS", "AdvancedLogger::_saveLogLevelsToSpiffs", ADVANCEDLOGGER_DEBUG);
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

# AdvancedLogger

[![Release](https://img.shields.io/github/v/release/jibrilsharafi/AdvancedLogger)](https://github.com/jibrilsharafi/AdvancedLogger/releases/latest)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/jijio/library/AdvancedLogger.svg)](https://registry.platformio.org/libraries/jijio/AdvancedLogger)
[![arduino-library-badge](https://www.ardu-badge.com/badge/AdvancedLogger.svg?)](https://www.ardu-badge.com/AdvancedLogger)
[![ESP32](https://img.shields.io/badge/ESP-32S3-000000.svg?longCache=true&style=flat&colorA=CC101F)](https://www.espressif.com/en/products/socs/esp32-S3)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/jibrilsharafi/AdvancedLogger/blob/master/LICENSE)

A logging library for ESP32 that saves logs to LittleFS and provides console output with detailed formatting. Features non-blocking queue-based logging, configurable log levels, and callback support.

## Key Features

- Non-blocking queue-based logging system
- File logging to LittleFS with automatic rotation
- Console output with timestamps and core ID
- Configurable log levels for console and file
- Log counters and queue monitoring
- Callback support for custom log handling
- Conditional compilation flags for production builds

## Installation

### PlatformIO
Add to your `platformio.ini` file:
```ini
lib_deps = jijio/AdvancedLogger
```

### Arduino IDE
Search for "AdvancedLogger" in the Library Manager.

### Manual Installation
Download the latest release from the [releases page](https://github.com/jibrilsharafi/AdvancedLogger/releases).

**Tested on:** ESP32S3, ESP-WROVER

## Quick Start

```cpp
#include <AdvancedLogger.h>

void setup() {
    Serial.begin(115200);
    AdvancedLogger::begin();
    
    LOG_INFO("System started");
    LOG_ERROR("Error occurred: %d", 42);
}

void loop() {
    LOG_DEBUG("Loop iteration: %lu", millis());
    delay(5000);
}
```

Output format:
```
[2024-03-23T09:44:10.123Z] [1 450 ms] [INFO   ] [Core 1] [main.cpp:setup] System started
[2024-03-23T09:44:10.456Z] [1 783 ms] [ERROR  ] [Core 1] [main.cpp:setup] Error occurred: 42
```

## Configuration

### Log Levels
Set different log levels for console output and file logging:

```cpp
AdvancedLogger::setPrintLevel(LogLevel::DEBUG);  // Console output
AdvancedLogger::setSaveLevel(LogLevel::WARNING); // File logging
```

Available levels: `VERBOSE`, `DEBUG`, `INFO`, `WARNING`, `ERROR`, `FATAL`

### Queue Configuration
Customize the logging queue with global build flags. They are read when the library itself is compiled, so a `#define` in the sketch is not enough (in the Arduino IDE, put the `-D` flags in a `build_opt.h` file next to the sketch).

In `platformio.ini`:
```ini
build_flags = 
    -DADVANCED_LOGGER_ALLOCABLE_HEAP_SIZE=20480   ; Internal RAM for the queue when there is no PSRAM (default 12 KB = 20 entries)
    -DADVANCED_LOGGER_PSRAM_QUEUE_SIZE=262144     ; PSRAM for the queue when PSRAM is available (default 64 KB = 109 entries)
    -DADVANCED_LOGGER_QUEUE_FULL_WAIT_MS=100      ; Longest a caller waits for a slot when the queue is full (default 100, 0 = never block)
    -DADVANCED_LOGGER_TASK_STACK_SIZE=8192        ; Log task stack (default 8 KB, a log rotation on LittleFS peaks at about 5.6 KB)
    -DADVANCED_LOGGER_TASK_PRIORITY=2             ; Log task priority
    -DADVANCED_LOGGER_MAX_MESSAGE_LENGTH=512      ; Max message size
```

With PSRAM the queue storage is allocated there automatically and internal RAM only holds the small queue control structure. Define `ADVANCED_LOGGER_DISABLE_PSRAM_QUEUE` to keep the queue in internal RAM.

When the queue is full, the caller waits up to `ADVANCED_LOGGER_QUEUE_FULL_WAIT_MS` for a slot, then the entry is dropped and counted (`getDroppedCount()`), and the log task writes a `WARNING` with the number of dropped entries. Sinks (console, file, callback) only ever run on the log task. A log rotation keeps the log task busy for a few seconds: size the queue for what your application logs in that time.

### File Flushing
Configure when log files are flushed to ensure data persistence (global build flags, as above):

```ini
build_flags = 
    -DADVANCED_LOGGER_FLUSH_INTERVAL_MS=5000            ; Flush every 5 seconds (default)
    -DADVANCED_LOGGER_FLUSH_LOG_LEVEL=LogLevel::ERROR   ; Log level that triggers immediate flush (default)
```

The library automatically flushes files periodically and on the specified log level to prevent data loss during power cycles or crashes.

### Callbacks
Register a function to handle log entries:

```cpp
void logHandler(const LogEntry& entry) {
    // Send to server, display on screen, etc.
    Serial.printf("Callback: %s\n", entry.message);
}

void setup() {
    AdvancedLogger::setCallback(logHandler);
    AdvancedLogger::setCallbackLevel(LogLevel::INFO); // Optional, default VERBOSE (everything)
    AdvancedLogger::begin();
}
```

The callback runs on the log task, so keep it short and do not log from it. Entries below the callback level that are also below the print and save levels are discarded before they take a queue slot.

### Monitoring
Check system status:

```cpp
unsigned long available = AdvancedLogger::getQueueSpacesAvailable();
unsigned long waiting = AdvancedLogger::getQueueMessagesWaiting();
unsigned long dropped = AdvancedLogger::getDroppedCount();

LOG_INFO("Queue: %lu available, %lu waiting, %lu dropped", available, waiting, dropped);
```

### File Management
```cpp
// Set max lines before auto-rotation
AdvancedLogger::setMaxLogLines(5000);

// Get current line count
unsigned long lines = AdvancedLogger::getLogLines();

// Clear log (keep 20% of recent entries)
AdvancedLogger::clearLogKeepLatestXPercent(20);

// Dump to Serial
AdvancedLogger::dump(Serial);
```

## Production Builds

Disable specific log levels to reduce binary size:

```cpp
#define ADVANCED_LOGGER_DISABLE_VERBOSE
#define ADVANCED_LOGGER_DISABLE_DEBUG
#define ADVANCED_LOGGER_DISABLE_CONSOLE_LOGGING  // Disable all console output
#define ADVANCED_LOGGER_DISABLE_FILE_LOGGING     // Disable all file logging

#include "AdvancedLogger.h"
```

## API Reference

### Logging Macros
- `LOG_VERBOSE(format, ...)` - Most detailed logging
- `LOG_DEBUG(format, ...)` - Debug information  
- `LOG_INFO(format, ...)` - General information
- `LOG_WARNING(format, ...)` - Warning messages
- `LOG_ERROR(format, ...)` - Error conditions
- `LOG_FATAL(format, ...)` - Fatal errors

### Core Functions
- `AdvancedLogger::begin(path)` - Initialize logger
- `AdvancedLogger::end()` - Save what is still queued, stop the log task and clean up resources
- `AdvancedLogger::setPrintLevel(level)` - Set console log level
- `AdvancedLogger::setSaveLevel(level)` - Set file log level
- `AdvancedLogger::setCallback(callback)` - Register log handler
- `AdvancedLogger::setCallbackLevel(level)` / `getCallbackLevel()` - Lowest level the callback receives (default `VERBOSE`)

### Log Counters
- `getVerboseCount()`, `getDebugCount()`, `getInfoCount()`
- `getWarningCount()`, `getErrorCount()`, `getFatalCount()`
- `getTotalLogCount()`, `resetLogCounters()`

### File Operations
- `setMaxLogLines(count)` - Set rotation threshold
- `getLogLines()` - Get current line count
- `clearLog()` - Delete all logs
- `clearLogKeepLatestXPercent(percent)` - Rotate logs
- `dump(stream)` - Output logs to stream

See the [examples](examples/) folder for complete usage examples.

## Contributing

Fork the repository, create a feature branch, and submit a pull request. All contributions are welcome.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

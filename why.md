
## Why?
There is no way of sugar-coat it: *every well-developed project will eventually need a proper logging* in place to make sure that not only everything is fully monitored, but also that everything monitored is later accessible for debugging. 
The development of [EnergyMe](https://github.com/jibrilsharafi/EnergyMe-Home) (my open-source energy monitoring device) quickly led to the necessity of having such feature. Nonetheless, I quickly found out that no repositories on GitHub where available capable of fully covering all of my [Requirements](#requirements).

### Requirements
Does it exist a C++ module for the ESP32 (the projects I am currently working on use this microcontroller) that allows logging with the following features:
- **Format**: a comprehensive format capable of including all the accessory information to each print message. Something like: `[2024-03-13 09:44:10] [1 313 ms] [INFO   ] [Core 1] [main::setup] Setting up ADE7953...`, which indeed explains very clearly the nature and context of the message. This type of logging is the norm in more complex systems and environments. 
- **Saving to memory**: developing a complex logging system is useless if it can only be accessed in real time. The ability of saving each message in chronological order in a long time storage medium is crucial to unlock the full potential of it. 
- **Ease of use**: the module should comprise of only a few functions, such as the core one `info(...)`  and the accessory ones needed to set the print and save levels to adapt to each use case.

### Don't reinvent the wheel
Some existing libraries are:

|                                                Name                                                |                       Owner                       |                                                                     Description                                                                     | Format | Saving | Ease of use |
| :------------------------------------------------------------------------------------------------: | :-----------------------------------------------: | :-------------------------------------------------------------------------------------------------------------------------------------------------: | :----: | :----: | :---------: |
|                            **[elog](https://github.com/x821938/elog)**                             |       [x821938](https://github.com/x821938)       |                                  Fast and efficient logging to multiple outputs, with many configuration settings.                                  |   ❕    |   ✔    |      ✔      |
|                 **[ESP.multiLogger](https://github.com/voelkerb/ESP.multiLogger)**                 |      [voelkerb](https://github.com/voelkerb)      |                                                         Simple logging to multiple outputs.                                                         |   ❌    |   ✔    |      ✔      |
|                          **[logging](https://github.com/esp32m/logging)**                          |        [esp32m](https://github.com/esp32m)        |                                           Context-based logging, multiple appenders, buffers and queues.                                            |   ❌    |   ✔    |      ❕      |
|                 **[MycilaLogger](https://github.com/mathieucarbou/MycilaLogger)**                  | [mathieucarbou](https://github.com/mathieucarbou) |                                        Simple and efficient. A wrapper around the standard `buffer.print()`.                                        |   ❌    |   ❌    |      ✔      |
| **[esp_log.h](https://github.com/espressif/esp-idf/blob/master/components/log/include/esp_log.h)** |     [espressif](https://github.com/espressif)     | Native method implemented by espressif. [Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/log.html). |   ❕    |   ❌    |      ❕      |

Many other similar libraries exists, but it is clear that **none of the reported libraries** (and of the others available on *GitHub*) **fully match my requirements**.

## How?
The rule is one: *keep it simple*.
I am in no way a advanced coder, and as such I tend to structure and write code to be as streamlined and simple as possible. So, no memory optimization or any of the pointer-magic stuff in C++. *Not my kink*.

### Format
**The format** of the message should **include any information** that could store **any value**. As such, everything related to the time should be logged, along with information about the location of the code in which the log takes place, as well as having different logging levels. 
We can set a custom format by using the function `sprintf` and passing the following as third argument: 
- `[%s] [%lu ms] [%s] [Core %d] [%s] %s` → `[2024-03-13 09:44:10] [1313 ms] [INFO] [Core 1] [main::setup] Setting up ADE7953...`
which will represent the following parts: 
- `[%s]` → `[2024-03-13 09:44:10]`: human-readable ISO8601 format in the format `%Y-%m-%d %H:%M:%S`. Indeed, the time must be set externally via other libraries.
- `[%lu ms]` → `[1313 ms]`: time passed since device boot, in *ms*. Crucial to precisely evaluate the time taken to perform any action.  
- `[%s]` → `[INFO]`: the log level, which could be one of the following 5 levels: *DEBUG, INFO, WARNING, ERROR, FATAL*.
- `[Core %d]` → `[Core 1]`: core in which the operation takes place. Useful for multi-core devices (such as many ESP32 variations).
- `[%s]` → `[main::setup]`: the current function, or in general any name to identify a particular section of the code.
- `%s` → `Setting up ADE7953...`: the actual message that contains all the info needed to be logged. 

### Saving to memory
Saving should be quite straightforward: just copy any message that is sent to the serial to a log file, appending it to the end with a newline. Whenever too many lines are present, the oldest ones should be deleted to keep the memory usage under control.

### Ease of use
Last but not least, the use should be as simple as possible: you just need to create `AdvancedLogger advancedLogger;` and simply use `advancedLogger.info("Setting up ADE7953...", "main::setup");`. Other (more complex) functions and settings should be available, but not necessary for the basic use. As someone once said: *Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away*. 

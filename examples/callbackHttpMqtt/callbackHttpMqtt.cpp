/*
 * File: callbackHttpMqtt.ino
 * ----------------------------
 * This example demonstrates integrating AdvancedLogger with MQTT and HTTP logging.
 * It shows how to:
 * - Send logs to a local MQTT broker and an HTTP endpoint
 * - Track logging performance metrics
 * - Handle network reconnections
 * - Format logs as JSON
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 21/03/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#include "AdvancedLogger.h"

// HTTP configuration
const String serverEndpoint = "http://192.168.1.100:8080/test"; // **** CHANGE THIS TO YOUR SERVER | Look at log_receiver.py ****
HTTPClient http;

// MQTT configuration
const char* mqttServer = "test.mosquitto.org"; // **** CHANGE THIS TO YOUR BROKER ****
const unsigned int mqttPort = 1883;
const char* mainTopic = "advancedlogger"; // To see the messages, use mosquitto_sub -h test.mosquitto.org -p 1883 -t "advancedlogger/+/log/+" -v
const unsigned int bufferSize = 1024;

// **** CHANGE THESE TO YOUR SSID AND PASSWORD ****
const char *ssid = "SSID";
const char *password = "PASSWORD";

const int timeZone = 0; // UTC. In milliseconds
const int daylightOffset = 0; // No daylight saving time. In milliseconds
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.windows.com";

unsigned long lastMillisLogClear = 0;
unsigned long intervalLogClear = 60000;
unsigned int maxLogLines = 100; // Low value for testing purposes

// Add after WiFi client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Modified callback function to make the JSON in the callback faster
char jsonBuffer[1024];  // Pre-allocated buffer
String cachedDeviceId;
String cachedTopicPrefix;

// Get device ID from MAC
String getDeviceId() {
    return String((uint32_t)ESP.getEfuseMac(), HEX);
}

/*
This callback function will be called by the AdvancedLogger
whenever a log is processed. It will pass the log information,
then the function will decide what to do with it (eg. based on
the level, it may decide to send it to an HTTP endpoint or to 
set a flag).

In this example, the function will:
- Format the log as JSON
- Send the log to an HTTP endpoint
- Publish the log to an MQTT topic

The function will also measure the time taken to format the JSON,
send the HTTP request, and publish the MQTT message.
*/
void callback(const LogEntry& entry) {
    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long startJson = micros();

    char levelStr[16];
    snprintf(levelStr, sizeof(levelStr), "%s", AdvancedLogger::logLevelToStringLower(entry.level, true));

    char timestampIso[TIMESTAMP_BUFFER_SIZE];
    AdvancedLogger::getTimestampIsoUtcFromUnixTimeMilliseconds(entry.unixTimeMilliseconds, timestampIso, sizeof(timestampIso));

    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"timestamp\":\"%s\","
         "\"millis\":%llu,"
         "\"level\":\"%s\","
         "\"core\":%d,"
         "\"file\":\"%s\","
         "\"function\":\"%s\","
         "\"message\":\"%s\"}",
        timestampIso,
        entry.millis,
        levelStr,
        entry.coreId,
        entry.file,
        entry.function,
        entry.message);

    unsigned long jsonTime = micros() - startJson;

    // HTTP POST
    unsigned long startHttp = micros();
    HTTPClient http;
    http.begin(serverEndpoint);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonBuffer);
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    unsigned long httpTime = micros() - startHttp;

    // MQTT Publish
    unsigned long startMqtt = micros();
    if (mqttClient.connected()) {
        if (cachedDeviceId.isEmpty()) {
            cachedDeviceId = getDeviceId();
            cachedTopicPrefix = String(mainTopic) + "/" + cachedDeviceId + "/log/";
        }

        String topic = cachedTopicPrefix + String(levelStr);

        if (!mqttClient.publish(topic.c_str(), jsonBuffer)) {
            Serial.printf("MQTT publish failed to %s. Error: %d\n", 
                topic.c_str(), mqttClient.state());
        }
    }
    unsigned long mqttTime = micros() - startMqtt;

    Serial.printf("Durations - JSON: %lu µs, HTTP: %lu µs, MQTT: %lu µs\n",
        jsonTime, httpTime, mqttTime);
}

void reconnectMQTT() {
    while (!mqttClient.connected()) {
        String clientId = "ESP32Client-" + getDeviceId();
        if (mqttClient.connect(clientId.c_str())) {
            LOG_INFO("MQTT Connected with client ID: %s", clientId.c_str());
        } else {
            LOG_ERROR("MQTT Connection failed, rc=%d", mqttClient.state());
        }
    }
}

void setup()
{
    // Initialize Serial and LittleFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!LittleFS.begin(true)) // Setting to true will format the LittleFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    AdvancedLogger::begin();
    AdvancedLogger::setMaxLogLines(maxLogLines);
    AdvancedLogger::setCallback(callback);

    LOG_DEBUG("AdvancedLogger setup done!");
    
    // Connect to WiFi
    // --------------------
    // Connect to the specified SSID
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        LOG_INFO("Connecting to WiFi... SSID: %s | Password: %s", ssid, password);
    }
    
    LOG_INFO(("IP address: " + WiFi.localIP().toString()).c_str());
    LOG_INFO("Device ID: %s", getDeviceId().c_str());

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setBufferSize(bufferSize); // Raise the buffer size as the standard one is only 256 bytes
    reconnectMQTT();

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);
    
    LOG_INFO("Setup done!");
}

void loop()
{
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    // Test a burst of messages to see the performance
    for (int i = 0; i < 10; i++) {
        LOG_VERBOSE("[BURST] This is a verbose message");
        LOG_DEBUG("[BURST] This is a debug message!");
        LOG_INFO("[BURST] This is an info message!!");
        LOG_WARNING("[BURST] This is a warning message!!!");
        LOG_ERROR("[BURST] This is a error message!!!!");
        LOG_FATAL("[BURST] This is a fatal message!!!!!");
    }

    LOG_DEBUG("This is a debug message!");
    delay(500);
    LOG_INFO("This is an info message!!");
    delay(500);
    LOG_WARNING("This is a warning message!!!");
    delay(500);
    LOG_ERROR("This is a error message!!!!");
    delay(500);
    LOG_FATAL("This is a fatal message!!!!!");
    delay(500);
    LOG_INFO("This is an info message!!", true);
    delay(1000);
}
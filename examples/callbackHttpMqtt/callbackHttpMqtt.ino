/*
 * File: callbackHttpMqtt.ino
 * ----------------------------
 * This example demonstrates integrating AdvancedLogger with MQTT and HTTP logging.
 * It shows how to:
 * - Forward logs to an HTTP endpoint
 * - Send logs to a local MQTT broker
 * - Track logging performance metrics
 * - Handle network reconnections
 * - Format logs as JSON
 *
 * Author: Jibril Sharafi, @jibrilsharafi
 * Created: 21/03/2024
 * Last modified: 22/05/2024
 * GitHub repository: https://github.com/jibrilsharafi/AdvancedLogger
 *
 * This library is licensed under the MIT License. See the LICENSE file for more information.
 *
 * This example covers the addition of a simple web server to the basicUsage, which allows
 * the user to explore the log and configuration files remotely.
 * 
 * All the other advanced usage features are reported in the basicUsage example.
 */

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

#include "AdvancedLogger.h"

const String serverEndpoint = "YOUR_IP";
HTTPClient http;

// Add after existing constants
const char* mqttServer = "192.168.1.222";  // Change to your MQTT broker IP
const unsigned int mqttPort = 1883;
const char* mainTopic = "energyme/home";
const unsigned int bufferSize = 1024;

const char *customLogPath = "/customPath/log.txt";
const char *customConfigPath = "/customPath/config.txt";
const char *customTimestampFormat = "%Y-%m-%d %H:%M:%S"; 
AdvancedLogger logger(
    customLogPath,
    customConfigPath,
    customTimestampFormat);

const int timeZone = 0; // UTC. In milliseconds
const int daylightOffset = 0; // No daylight saving time. In milliseconds
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *ntpServer3 = "time.windows.com";

// **** CHANGE THESE TO YOUR SSID AND PASSWORD ****
const char *ssid = "SSID";
const char *password = "PASSWORD";

unsigned long lastMillisLogClear = 0;
unsigned long intervalLogClear = 60000;
unsigned int maxLogLines = 100; // Low value for testing purposes

// Add after WiFi client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Modified callback function
char jsonBuffer[512];  // Pre-allocated buffer
String cachedDeviceId;
String cachedTopicPrefix;

// Get device ID from MAC
String getDeviceId() {
    return String((uint32_t)ESP.getEfuseMac(), HEX);
}

void callback(
    const char* timestamp,
    unsigned long millisEsp,
    const char* level,
    unsigned int coreId,
    const char* function,
    const char* message
) {
    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long startJson = micros();
    
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"timestamp\":\"%s\","
         "\"millis\":%lu,"
         "\"level\":\"%s\","
         "\"core\":%u,"
         "\"function\":\"%s\","
         "\"message\":\"%s\"}",
        timestamp,
        millisEsp,
        level,
        coreId,
        function,
        message);
    
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
        
        String topic = cachedTopicPrefix + String(level);
        
        if (!mqttClient.publish(topic.c_str(), jsonBuffer)) {
            Serial.printf("MQTT publish failed to %s. Error: %d\n", 
                topic.c_str(), mqttClient.state());
        }
    }
    unsigned long mqttTime = micros() - startMqtt;

    Serial.printf("Durations - JSON: %lu µs, HTTP: %lu µs, MQTT: %lu µs\n",
        jsonTime, httpTime, mqttTime);
}

// Add MQTT reconnect function
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        String clientId = "ESP32Client-" + getDeviceId();
        if (mqttClient.connect(clientId.c_str())) {
            logger.info("MQTT Connected", "reconnectMQTT");
        } else {
            logger.error("MQTT Connection failed, rc=%d", "reconnectMQTT", mqttClient.state());
        }
    }
}

void setup()
{
    // Initialize Serial and SPIFFS (mandatory for the AdvancedLogger library)
    // --------------------
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) // Setting to true will format the SPIFFS if mounting fails
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
    }

    logger.begin();
    logger.setMaxLogLines(maxLogLines);
    logger.setCallback(callback);
    logger.setSaveLevel(LogLevel::INFO);

    logger.debug("AdvancedLogger setup done!", "basicServer::setup");
    
    // Connect to WiFi
    // --------------------
    // Connect to the specified SSID
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        logger.info("Connecting to WiFi... SSID: %s | Password: %s", "basicServer::setup", ssid, password);
    }
    
    logger.info(("IP address: " + WiFi.localIP().toString()).c_str(), "basicServer::setup");

    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setBufferSize(bufferSize);
    reconnectMQTT();

    configTime(timeZone, daylightOffset, ntpServer1, ntpServer2, ntpServer3);
    
    logger.info("Server started!", "basicServer::setup");

    logger.info("Setup done!", "basicServer::setup");
}

void loop()
{
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();

    // Test a burst of messages
    for (int i = 0; i < 10; i++) {
        logger.verbose("[BURST] This is a verbose message", "basicServer::loop");
        logger.debug("[BURST] This is a debug message!", "basicServer::loop");
        logger.info("[BURST] This is an info message!!", "basicServer::loop");
        logger.warning("[BURST] This is a warning message!!!", "basicServer::loop");
        logger.error("[BURST] This is a error message!!!!", "basicServer::loop");
        logger.fatal("[BURST] This is a fatal message!!!!!", "basicServer::loop");
    }

    logger.debug("This is a debug message!", "basicServer::loop");
    delay(100);
    logger.info("This is an info message!!", "basicServer::loop");
    delay(100);
    logger.warning("This is a warning message!!!", "basicServer::loop");
    delay(100);
    logger.error("This is a error message!!!!", "basicServer::loop");
    delay(100);
    logger.fatal("This is a fatal message!!!!!", "basicServer::loop");
    delay(100);
    logger.info("This is an info message!!", "basicServer::loop", true);
    delay(500);;

    if (millis() - lastMillisLogClear > intervalLogClear)
    {
        logger.info("Current number of log lines: %d", "basicServer::loop", logger.getLogLines());
        logger.clearLog();
        logger.warning("Log cleared!", "basicServer::loop");

        lastMillisLogClear = millis();
    }
}
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h> // MQTT library
#include <ArduinoJson.h>  // JSON library

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -28800, 60000);
Preferences preferences;
AsyncWebServer server(80);

// Global WiFiClient and WiFiClientSecure
WiFiClient espClient;
WiFiClientSecure sslClient;  // Secure WiFiClient for MQTT

// MQTT client
PubSubClient mqttClient(espClient);

// AWS IoT settings
const char* mqtt_server = "a2hsbhskjdnch7-ats.iot.us-east-2.amazonaws.com"; // Replace with your AWS IoT endpoint
const int mqtt_port = 8883; // Default secure MQTT port
const char* mqtt_topic = "sensor/data"; // MQTT topic to publish sensor data
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 60000; // 1 minute

// AWS IoT certificates (downloaded from the AWS IoT console)
const char* cert = R"KEY(-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)KEY";   // Replace with your certificate
const char* privateKey = R"KEY(-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)KEY"; // Replace with your private key
const char* caCert = R"KEY(-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----)KEY"; // Replace with your CA certificate

const char* wifiSetupPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Setup</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f4; text-align: center; padding: 50px; }
        .container { background: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); display: inline-block; text-align: left; }
        h2 { color: #333; text-align: center; }
        label { display: block; margin: 10px 0 5px; font-weight: bold; }
        input, button { width: 90%; padding: 10px; margin: 10px auto; border-radius: 5px; display: block; border: 1px solid #ccc; }
        button { background: rgb(116, 119, 123); color: white; border: none; cursor: pointer; }
        button:hover { background: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h2>WiFi Setup</h2>
        <form action="/save" method="POST">
            <label>SSID:</label>
            <input type="text" name="ssid" required>
            <label>Password:</label>
            <input type="password" name="password" required>
            <button type="submit">Save & Connect</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

void mqttCallback(char* topic, byte* payload, unsigned int length);

void displayMessage(const char* line1, const char* line2, const char* line3);
void connectToMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");
        if (mqttClient.connect("ESP32_Client")) {
            Serial.println("Connected!");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqttClient.state());
            delay(5000);
        }
    }
}

void displayMessage(const char* line1, const char* line2, const char* line3) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 10);
    display.println(line1);
    display.setCursor(10, 25);
    display.println(line2);
    display.setCursor(10, 40);
    display.println(line3);
    display.display();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Handle incoming messages if necessary
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    
    // Print the received payload
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Setup DHT sensor
    dht.begin();

    // WiFi setup
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");

    if (ssid == "" || password == "") {
        WiFi.softAP("ESP32-Setup");
        displayMessage("Setup WiFi", "Connect to ESP32", "Open 192.168.4.1");
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/html", wifiSetupPage);
        });
        server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
            String new_ssid = request->arg("ssid");
            String new_password = request->arg("password");
            if (!new_ssid.isEmpty() && !new_password.isEmpty()) {
                preferences.putString("ssid", new_ssid);
                preferences.putString("password", new_password);
                request->send(200, "text/html", "<h3>WiFi Saved! Rebooting...</h3>");
                delay(2000);
                ESP.restart();
            }
        });
        server.begin();
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.print("Connecting to WiFi: ");
        Serial.println(ssid);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            displayMessage("WiFi Connected!", WiFi.localIP().toString().c_str(), "");
            server.begin();
        } else {
            displayMessage("False Credential", "Connect to ESP32 WiFi", "Go to 192.168.4.1");
            delay(5000);
            preferences.clear();
            ESP.restart();
        }
    }

    // Set up MQTT
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);

    // Set up secure MQTT client with certificates
    sslClient.setCACert(caCert);
    sslClient.setCertificate(cert);
    sslClient.setPrivateKey(privateKey);
    mqttClient.setClient(sslClient);

    // Connect to MQTT server
    connectToMQTT();
}

String createJSONPayload(float temperature, float humidity, String date, String time) {
    StaticJsonDocument<256> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["date"] = date;
    doc["time"] = time;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void loop() {
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);
    String formattedDate = String(ptm->tm_mon + 1) + "/" + String(ptm->tm_mday) + "/" + String(ptm->tm_year + 1900);
    String formattedTime = timeClient.getFormattedTime();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Debug: Print values to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("DHT Sensor Error!");
        return;
    }

    // Update OLED display with sensor data
    display.clearDisplay();

    // Display Date
    display.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(formattedDate, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 0);  // Center the text
    display.print(formattedDate);
    
   // Ensure variables are properly initialized
    if (formattedTime != nullptr && !formattedTime.isEmpty()) {
        display.setTextSize(2);
        display.getTextBounds(formattedTime, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((SCREEN_WIDTH - w) / 2, 15);  // Center the text
        display.print(formattedTime);
    }

    // Display Temperature and Humidity
    if (!isnan(temperature) && !isnan(humidity)) {
        display.setTextSize(1);
        String tempHum = "T:" + String(temperature) + "C H:" + String(humidity) + "%";
        display.getTextBounds(tempHum, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((SCREEN_WIDTH - w) / 2, 50);  // Center the text
        display.print(tempHum);
    }

    display.display();  // Refresh the display with new data
    // delay(500);  // Update every 2 seconds
// Send MQTT message with sensor data
  // Check if it's time to send data (every 60 seconds)
  if (millis() - lastPublishTime >= publishInterval) {
    lastPublishTime = millis();
    String payload = createJSONPayload(temperature, humidity, formattedDate, formattedTime);
    mqttClient.publish(mqtt_topic, payload.c_str());
  }

    mqttClient.loop();
    // delay(5000);  // Adjust delay as needed
}



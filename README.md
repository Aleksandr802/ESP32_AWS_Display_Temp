# ESP32 AWS IoT Temperature and Humidity Monitor

This project demonstrates how to use an ESP32 microcontroller to monitor temperature and humidity using a DHT11 sensor, display the data on an OLED screen, and send the data to AWS IoT Core via MQTT. The project also includes a WiFi setup interface and secure communication using SSL/TLS certificates.

## Features

- **Temperature and Humidity Monitoring**: Reads data from a DHT11 sensor.
- **OLED Display**: Displays the current date, time, temperature, and humidity.
- **AWS IoT Integration**: Publishes sensor data to AWS IoT Core using MQTT.
- **Secure Communication**: Uses SSL/TLS certificates for secure MQTT communication.
- **WiFi Setup Interface**: Provides a web-based interface for configuring WiFi credentials.
- **Over-the-Air (OTA) Updates**: Supports OTA updates for firmware.

## Components Used

- **ESP32 Development Board**: `esp32doit-devkit-v1`
- **DHT11 Sensor**: For temperature and humidity measurements.
- **OLED Display**: 128x64 SSD1306 display.
- **AWS IoT Core**: For cloud-based data storage and processing.

## Libraries

The following libraries are used in this project:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

## How It Works

1. **WiFi Setup**:
   - If no WiFi credentials are stored, the ESP32 starts in Access Point mode.
   - Users can connect to the ESP32 and configure WiFi credentials via a web interface.

2. **Data Collection**:
   - The DHT11 sensor reads temperature and humidity data.
   - The NTPClient library fetches the current date and time.

3. **Data Display**:
   - The OLED screen displays the date, time, temperature, and humidity.

4. **Data Transmission**:
   - Sensor data is formatted as a JSON payload.
   - The data is sent to AWS IoT Core via MQTT over a secure SSL/TLS connection.

5. **Error Handling**:
   - Handles sensor errors and WiFi connection issues gracefully.

## Setup Instructions

1. **Hardware Setup**:
   - Connect the DHT11 sensor to the ESP32.
   - Connect the OLED display to the ESP32 via I2C.

2. **AWS IoT Setup**:
   - Create an AWS IoT Core thing.
   - Download the device certificate, private key, and root CA certificate.
   - Replace the placeholders in the code with your AWS IoT endpoint and certificates.

3. **PlatformIO Setup**:
   - Install PlatformIO in Visual Studio Code.
   - Clone this repository and open it in PlatformIO.
   - Build and upload the code to the ESP32.

4. **WiFi Configuration**:
   - Connect to the ESP32's WiFi Access Point.
   - Open a browser and navigate to `192.168.4.1` to configure WiFi credentials.

## Usage

- Once connected to WiFi, the ESP32 will start sending sensor data to AWS IoT Core every 60 seconds.
- The OLED display will show the current date, time, temperature, and humidity.

## License

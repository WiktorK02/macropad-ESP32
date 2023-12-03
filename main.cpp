#include <BfButton.h>
#include <Bounce2.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>


int btnPin = 27;        
int DT = 26;           
int CLK = 25;           
BfButton btn(BfButton::STANDALONE_DIGITAL, btnPin, true, LOW);
int counter = 0;
int angle = 0;
int lastAngle = 0;
int aState;
int aLastState;

const char* settingsFile = "/config.json";

const int buttonPin = 32; 
Bounce debouncer = Bounce(); 
WiFiManager wm;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char *mqtt_topic = "con/ok";
const char *mqtt_topic_read = "analog/read";
const char* mqtt_button_topic = "button/click";
const unsigned long RECONNECT_DELAY = 5000;

WiFiManagerParameter mqttServer("mqtt_server", "MQTT Server", "0.0.0.24", 40);
WiFiManagerParameter mqttPort("mqtt_port", "MQTT Port", "1883", 6);
WiFiManagerParameter mqttUser("mqtt_user", "MQTT User", "admin", 20);
WiFiManagerParameter mqttPassword("mqtt_password", "MQTT Password", "admin", 20);
void saveSettingsToJson() {
  DynamicJsonDocument doc(1024);

  doc["mqtt_server"] = mqttServer.getValue();
  doc["mqtt_port"] = mqttPort.getValue();
  doc["mqtt_user"] = mqttUser.getValue();
  doc["mqtt_password"] = mqttPassword.getValue();

  File configFile = SPIFFS.open(settingsFile, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  serializeJson(doc, configFile);
  configFile.close();
  Serial.println("Settings saved to config.json");
}

void loadSettingsFromJson() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  File configFile = SPIFFS.open(settingsFile, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }

  String mqttServerValue = doc["mqtt_server"].as<String>();
  String mqttPortValue = doc["mqtt_port"].as<String>();
  String mqttUserValue = doc["mqtt_user"].as<String>();
  String mqttPasswordValue = doc["mqtt_password"].as<String>();

  mqttServer.setValue(mqttServerValue.c_str(), mqttServerValue.length());
  mqttPort.setValue(mqttPortValue.c_str(), mqttPortValue.length());
  mqttUser.setValue(mqttUserValue.c_str(), mqttUserValue.length());
  mqttPassword.setValue(mqttPasswordValue.c_str(), mqttPasswordValue.length());

  configFile.close();
  Serial.println("Settings loaded from config.json");
}

void connectToMQTT() {
  Serial.print("Attempting to connect to MQTT server: ");
  Serial.print(mqttServer.getValue());
  Serial.print(" on port: ");
  Serial.println(atoi(mqttPort.getValue()));

  mqttClient.setServer(mqttServer.getValue(), atoi(mqttPort.getValue()));

  if (mqttClient.connect("ESP8266Client", mqttUser.getValue(), mqttPassword.getValue())) {
    Serial.println("Connected to MQTT");
    mqttClient.loop();
    mqttClient.publish(mqtt_topic, "222222");
  } else {
    Serial.print("Failed to connect to MQTT, rc=");
    Serial.println(mqttClient.state());
  }
}


void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern) {
  switch (pattern) {
  case BfButton::SINGLE_PRESS:
    Serial.println("Single push");
    mqttClient.publish(mqtt_button_topic, "single_press");
    break;

  case BfButton::DOUBLE_PRESS:
    Serial.println("Double push");
    break;

  case BfButton::LONG_PRESS:
    Serial.println("Long push");
    break;
  }
}

void setup() {
  Serial.println(angle);
  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  aLastState = digitalRead(CLK);

  btn.onPress(pressHandler)
      .onDoublePress(pressHandler) 
      .onPressFor(pressHandler, 1000); 

  //wm.resetSettings();
  pinMode(buttonPin, INPUT_PULLUP);
  debouncer.attach(buttonPin);
  debouncer.interval(50);
  Serial.begin(115200);

    wm.addParameter(&mqttServer);
    wm.addParameter(&mqttPort);
    wm.addParameter(&mqttUser);
    wm.addParameter(&mqttPassword);

    bool res = wm.autoConnect("MacroPad", "password");
    if (!res) {
        Serial.println("Failed to connect");
    } else {
        Serial.println("Connected to WiFi");
    }

    delay(1000);
    connectToMQTT();

    loadSettingsFromJson();

    if (!mqttServer.getValue() || !mqttPort.getValue() || !mqttUser.getValue() || !mqttPassword.getValue()) {
        Serial.println("MQTT settings are missing. Please configure them through the captive portal.");
    } else {

        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }

        if (!mqttClient.connected()) {
            Serial.println("MQTT not connected. Attempting to reconnect...");
            delay(RECONNECT_DELAY); 
            connectToMQTT();
        }
    }
}
void loop() {

    debouncer.update();
    if (debouncer.fell()) {
        Serial.println("Button pressed!");
        saveSettingsToJson(); // Save settings to JSON before restarting
        wm.resetSettings();
        delay(1000);
    }

    mqttClient.loop();
    
    btn.read();
    int clkState = digitalRead(CLK);
    int dtState = digitalRead(DT);
  
    if (clkState != aLastState) {
        if (clkState == HIGH) {
            if (dtState == LOW) {
                angle = 5;
            } else {
                angle = -5;
            }

            Serial.println(angle);

            char angleStr[10];
            itoa(angle, angleStr, 10);
            mqttClient.publish(mqtt_topic_read, angleStr);
            lastAngle = angle;
            Serial.println(angle);
        }
    }

    aLastState = clkState;
}



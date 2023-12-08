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

const int switchPin = 12;
const int switchPin2 = 14;
Bounce debouncer = Bounce();
Bounce debouncer2 = Bounce();

bool isSwitch1On = false;
bool isSwitch2On = false;

WiFiManager wm;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const char *mqtt_topic = "con/ok";
const char *mqtt_topic_read = "analog/read";
const char* mqtt_button_topic = "button/click";
const char* mqtt_switch1_topic = "switch/click/1";
const char* mqtt_switch2_topic = "switch/click/2";
const unsigned long RECONNECT_DELAY = 5000;

WiFiManagerParameter mqttServer("mqtt_server", "MQTT Server", "", 40);
WiFiManagerParameter mqttPort("mqtt_port", "MQTT Port", "", 6);
WiFiManagerParameter mqttUser("mqtt_user", "MQTT User", "", 20);
WiFiManagerParameter mqttPassword("mqtt_password", "MQTT Password", "", 20);

void saveSettingsToJson() {
  DynamicJsonDocument doc(1024);

  if (strlen(mqttServer.getValue()) > 0 &&
      strlen(mqttPort.getValue()) > 0 &&
      strlen(mqttUser.getValue()) > 0 &&
      strlen(mqttPassword.getValue()) > 0) {
    doc["mqtt_server"] = mqttServer.getValue();
    doc["mqtt_port"] = mqttPort.getValue();
    doc["mqtt_user"] = mqttUser.getValue();
    doc["mqtt_password"] = mqttPassword.getValue();

    File configFile = SPIFFS.open(settingsFile, "w");
    if (configFile) {
      serializeJson(doc, configFile);
      configFile.close();
      Serial.println("Settings saved to config.json");
    } else {
      Serial.println("Failed to open config file for writing");
    }
  } else {
    Serial.println("MQTT settings are empty. Not saving to config.json");
  }
}

void loadSettingsFromJson() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  File configFile = SPIFFS.open(settingsFile, "r");
  if (configFile) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (!error) {
      String mqttServerValue = doc["mqtt_server"].as<String>();
      String mqttPortValue = doc["mqtt_port"].as<String>();
      String mqttUserValue = doc["mqtt_user"].as<String>();
      String mqttPasswordValue = doc["mqtt_password"].as<String>();

      if (mqttServerValue.length() > 0 &&
          mqttPortValue.length() > 0 &&
          mqttUserValue.length() > 0 &&
          mqttPasswordValue.length() > 0) {
        mqttServer.setValue(mqttServerValue.c_str(), mqttServerValue.length());
        mqttPort.setValue(mqttPortValue.c_str(), mqttPortValue.length());
        mqttUser.setValue(mqttUserValue.c_str(), mqttUserValue.length());
        mqttPassword.setValue(mqttPasswordValue.c_str(), mqttPasswordValue.length());

        Serial.println("Settings loaded from config.json");
      } else {
        Serial.println("Loaded MQTT settings are empty. Using default values.");
      }
    } else {
      Serial.println("Failed to parse config file");
    }
  } else {
    Serial.println("Failed to open config file for reading");
  }
}

void connectToMQTT() {
  Serial.print("Attempting to connect to MQTT server: ");
  Serial.print(mqttServer.getValue());
  Serial.print(" on port: ");
  Serial.println(atoi(mqttPort.getValue()));

  mqttClient.setServer(mqttServer.getValue(), atoi(mqttPort.getValue()));

  if (mqttClient.connect("ESP8266Client", mqttUser.getValue(), mqttPassword.getValue())) {
    Serial.println("Connected to MQTT");
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
      wm.resetSettings();
      break;

    case BfButton::LONG_PRESS:
      Serial.println("Long push");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Delay for serial monitor connection

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  pinMode(CLK, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  aLastState = digitalRead(CLK);

  btn.onPress(pressHandler)
     .onDoublePress(pressHandler)
     .onPressFor(pressHandler, 1000);

  pinMode(switchPin, INPUT_PULLUP);
  debouncer.attach(switchPin);
  pinMode(switchPin2, INPUT_PULLUP);
  debouncer2.attach(switchPin2);
  debouncer2.interval(50);
  debouncer.interval(50);

  wm.addParameter(&mqttServer);
  wm.addParameter(&mqttPort);
  wm.addParameter(&mqttUser);
  wm.addParameter(&mqttPassword);
  wm.setMinimumSignalQuality();
  wm.setConnectTimeout(180);
  wm.setTimeout(180);

  bool res = wm.autoConnect("MacroPad", "password");

  if (!res) {
    Serial.println("Failed to autoconnect. Retrying...");
  } else {
    Serial.println("Connected to WiFi");
    saveSettingsToJson();
  }
  loadSettingsFromJson();
  delay(1000);
  connectToMQTT();

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
  btn.read(); // Read button state

  // Check and reconnect MQTT
  if (!mqttClient.connected()) {
    Serial.println("MQTT not connected. Attempting to reconnect...");
    delay(RECONNECT_DELAY);
    connectToMQTT();
  }

  debouncer.update();
  mqttClient.loop();
  debouncer2.update();

  if (debouncer.fell()) {
    isSwitch1On = !isSwitch1On;

    if (isSwitch1On) {
      Serial.println("Switch 1 is ON");
    } else {
      Serial.println("Switch 1 is OFF");
    }
    mqttClient.publish(mqtt_switch1_topic, "true");
  }

  if (debouncer2.fell()) {
    isSwitch2On = !isSwitch2On;

    if (isSwitch2On) {
      Serial.println("Switch 2 is ON");
    } else {
      Serial.println("Switch 2 is OFF");
    }
    mqttClient.publish(mqtt_switch2_topic, "true");
  }

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

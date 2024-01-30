#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define DHTPin D5
#define DHTType DHT22

DHT dht = DHT(DHTPin, DHTType);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;

#define MSG_BUFFER_SIZE (500) 
WiFiClient client; 
PubSubClient mqtt_client(client); 
long lastMsg = 0;

String clientID = "ESP8266Client-";

String topicPrefix = "MACK20014481";
String topicAll = topicPrefix + "/#";
String topic_0 = topicPrefix + "/hello";
String message_0 = "NodeMCU Connected!";
String topic_2 = topicPrefix + "/sensor2"; 
String message_2 = "";
String topic_3 = topicPrefix + "/sensor3";
String topic_4 = topicPrefix + "/sensor4";
String msg = "";

byte degree[8] = { B00001100,
                   B00010010,
                   B00010010,
                   B00001100,
                   B00000000,
                   B00000000,
                   B00000000,
                   B00000000,
                 };

 
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  msg = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg += (char)payload[i];
  }

  if (String(topic) == topic_2) {
    message_2 = msg;
  }

  Serial.println();

}

void reconnect() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection…");

    randomSeed(micros());
    clientID += String(random(0xffff), HEX);

    if (mqtt_client.connect(clientID.c_str())) {
      Serial.println("connected");
      mqtt_client.publish(topic_0.c_str(), message_0.c_str());
      mqtt_client.subscribe(topicAll.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void checkTopicTwoMsg(String message){
  if (message.toInt() == 1) {
    lcd.noBacklight();
  } else {
    lcd.backlight();
  }
}

float read_temperature() {
  float temperature = dht.readTemperature();
  float result;

  if (! (isnan(temperature)))
    result = temperature;
  else
    result = -99.99;
  return result;
}

float read_humidity() {
  float humidity = dht.readHumidity();
  float result;

  if (! (isnan(humidity)))
    result = humidity;
  else
    result = -99.99;
  return result;
}

void logHumidity(float humidity) {
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%");
}

void logTemperature(float temperature) {
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C");
}

void printTemperature(float temperature) {
  lcd.setCursor(0, 0);
  lcd.print("Temp : ");
  lcd.print(temperature);
  lcd.print(" ");
  lcd.write((byte)0);
  lcd.print("C");
}

void printHumidity(float humidity) {
  lcd.setCursor(0, 1);
  lcd.print("Umid : ");
  lcd.print(humidity);
  lcd.print(" %");
}

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.clear();
  lcd.createChar(0, degree);
  lcd.backlight();
  dht.begin();
  setup_wifi();

  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
}

void loop() {
  if (!mqtt_client.connected()) {
    reconnect();
  }
  mqtt_client.loop();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  char temperature_str[10] = {0};
  char humidity_str[10] = {0};

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error reading temperature and humidity.");
    return;
  }

  Serial.println();

  printTemperature(temperature);
  printHumidity(humidity);

  sprintf(temperature_str, "%.2f", read_temperature());
  sprintf(humidity_str, "%.2f", read_humidity());

  checkTopicTwoMsg(message_2);

  mqtt_client.publish(topic_3.c_str(), temperature_str);
  mqtt_client.publish(topic_4.c_str(), humidity_str);

  delay(2000);
}

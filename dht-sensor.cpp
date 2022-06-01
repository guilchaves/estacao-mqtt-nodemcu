/*
 * Estacao Metereologica MQTT
 * Autor: Guilherme Chaves | TIA: 20014481
 */


/*
 * Carrega bibliotecas
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

/*
 * DHTPin recebe o pino conectado ao DHT;
 * DHTType recebe o modelo do sensor utilizado;
 * dht instancia objeto DHT;
 * lcd instancia objeto LiquidCrystal_I2Cl 
 */

#define DHTPin D5
#define DHTType DHT22

DHT dht = DHT(DHTPin, DHTType);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
 * @ssid recebe SSID da rede a ser conectada;
 * @password recebe senha da rede a ser conectada;
 * @mqtt_server recebe endereco de broker utilizado;
 * @mqtt_port recebe a porta do broker utilizado;
 */
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const int mqtt_port = 1883;

#define MSG_BUFFER_SIZE (500) //define buffer size para 500;
WiFiClient client; //inicializa client wifi;
PubSubClient mqtt_client(client); //gera objeto mqtt_client que ira se comunicar com broker mqtt;
long lastMsg = 0;

String clientID = "ESP8266Client-";

String topicPrefix = "MACK20014481";
String topicAll = topicPrefix + "/#";
String topic_0 = topicPrefix + "/hello";
String message_0 = "NodeMCU Connected!";
String topic_2 = topicPrefix + "/sensor2"; //recebe mensagem para ligar/desligar a luz de fundo do Display LCD;
String message_2 = "";
String topic_3 = topicPrefix + "/sensor3"; // indica temperatura;
String topic_4 = topicPrefix + "/sensor4"; // indica umidade;
String msg = "";


//cria o simbolo de grau para ser impresso no display;
byte degree[8] = { B00001100,
                   B00010010,
                   B00010010,
                   B00001100,
                   B00000000,
                   B00000000,
                   B00000000,
                   B00000000,
                 };


/*
 * Metodo para iniciar conexao com rede Wi-fi;
 */
 
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

/*
 * Metodo para receber mensagem payload e imprimir no Serial Monitor;
 */
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

/*
 * Metodo para reconectar ao broker caso a conexão seja perdida;
 */
void reconnect() {
  // Loop until we’re reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection…");

    // Create a random client ID
    randomSeed(micros()); //inicializa a semente do gerador de numeros aleatorios
    clientID += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqtt_client.connect(clientID.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt_client.publish(topic_0.c_str(), message_0.c_str());
      // ... and resubscribe
      mqtt_client.subscribe(topicAll.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*
 * Metodo que recebe mensagem para o topico 2 e ativa ou desliga a luz de fundo do Display;
 */
void checkTopicTwoMsg(String message){
  if (message.toInt() == 1) {
    lcd.noBacklight();
  } else {
    lcd.backlight();
  }
}

/*
 * Metodo que le a temperatura via DHT22;
 */
float read_temperature() {
  float temperature = dht.readTemperature();
  float result;

  if (! (isnan(temperature)))
    result = temperature;
  else
    result = -99.99;
  return result;
}

/*
 * Metodo que lê umidade via DHT22
 */
float read_humidity() {
  float humidity = dht.readHumidity();
  float result;

  if (! (isnan(humidity)))
    result = humidity;
  else
    result = -99.99;
  return result;
}

/*
 * Metodo para logar valor de umidade no Serial Monitor
 */
char logHumidity(float humidity) {
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%");
}

/*
 * Metodo para logar valor da temperatura no Serial Monitor
 */
char logTemperature(float temperature) {
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C");
}

/*
 * Metodo para imprimir no display a temperatura na linha 1
 */
void printTemperature(float temperature) {
  lcd.setCursor(0, 0);
  lcd.print("Temp : ");
  lcd.print(temperature);
  lcd.print(" ");
  lcd.write((byte)0);
  lcd.print("C");
}

/*
 * Metodo para imprimir no display a umidade na linha 2
 */
void printHumidity(float humidity) {
  lcd.setCursor(0, 1);
  lcd.print("Umid : ");
  lcd.print(humidity);
  lcd.print(" %");
}

/*
 * Metodo setup com inicialização do dispositivo;
 * 
 * @lcd.createChar gera o simbolo de grau da variavel degree
 * @lcd.backlight liga a luz de fundo do display
 */
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

/*
 * Metodo loop onde leituras de umidade e temperatura são realizadas e publicadas no broker
 */
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

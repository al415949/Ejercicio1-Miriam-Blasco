#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

const char* jugador = "miriam"; 

const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;

const char* topicTX = "instrumentacion/blackjack";
char topicRX[64];

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient client(espClient);


const int PIN_NUEVA  = 33;
const int PIN_CARTA  = 25;
const int PIN_PLANTO = 26;

int antNueva  = HIGH;
int antCarta  = HIGH;
int antPlanto = HIGH;

char estado2letra(const char* e) {
  if (!e) return ' ';
  if (!strcmp(e, "ganaste"))  return 'g';
  if (!strcmp(e, "activa"))   return 'a';
  if (!strcmp(e, "empate"))   return 'e';
  if (!strcmp(e, "perdiste")) return 'p';
  return ' ';
}

void printFixed(uint8_t col, uint8_t row, uint8_t width, const char* txt) {
  lcd.setCursor(col, row);
  for (uint8_t i = 0; i < width; i++) {
    char c = (txt && txt[i]) ? txt[i] : ' ';
    lcd.print(c);
    if (!txt || !txt[i]) {
    
      for (uint8_t j = i + 1; j < width; j++) lcd.print(' ');
      break;
    }
  }
}

void dibujarMarco() {
  lcd.setCursor(0,0); lcd.print("C: ");
  lcd.setCursor(0,1); lcd.print("J: ");
  lcd.setCursor(12,0); lcd.print("$:");
}

void enviarAccion(const char* accion) {
  StaticJsonDocument<128> doc;
  char out[128];

  doc["jugador"] = jugador;
  doc["accion"]  = accion;

  size_t n = serializeJson(doc, out);
  client.publish(topicTX, (uint8_t*)out, n);

  Serial.println(accion);
}

void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, payload, length)) return;

  const char* crup = doc["crupier"] | "";
  const char* jug  = doc["jugador"] | "";
  const char* est  = doc["estado"]  | "";
  int fondos        = doc["fondos"]  | 0;

  dibujarMarco();

  // Cartas: columnas 3..11 (9 chars). 
  printFixed(3, 0, 9, crup);   // C:
  printFixed(3, 1, 12, jug);   // J: 

 
  char money[8];
  snprintf(money, sizeof(money), "%d", fondos);
  printFixed(14, 0, 2, money);

 
  lcd.setCursor(15, 1);
  lcd.print(estado2letra(est));

  
  Serial.print("RX "); Serial.print(topic); Serial.print(" -> ");
  Serial.println(est);
}

void conectarWiFi() {
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) delay(100);
}

void conectarMQTT() {
  while (!client.connected()) {
    if (client.connect(jugador)) {
      client.subscribe(topicRX);
    } else {
      delay(500);
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_NUEVA,  INPUT_PULLUP);
  pinMode(PIN_CARTA,  INPUT_PULLUP);
  pinMode(PIN_PLANTO, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Conectando...");
  lcd.setCursor(0,1); lcd.print("WiFi/MQTT");

  conectarWiFi();

  snprintf(topicRX, sizeof(topicRX), "instrumentacion/%s", jugador);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  conectarMQTT();

  lcd.clear();
  dibujarMarco();
}

void loop() {
  if (!client.loop()) conectarMQTT();

  int bNueva  = digitalRead(PIN_NUEVA);
  int bCarta  = digitalRead(PIN_CARTA);
  int bPlanto = digitalRead(PIN_PLANTO);

  if (bNueva == LOW)  Serial.println("BOTON NUEVA (pin 33)");
  if (bCarta == LOW)  Serial.println("BOTON CARTA (pin 25)");
  if (bPlanto == LOW) Serial.println("BOTON PLANTO (pin 26)");
  delay(200);

  if (bNueva == LOW && antNueva == HIGH)   enviarAccion("nueva");
  if (bCarta == LOW && antCarta == HIGH)   enviarAccion("carta");
  if (bPlanto == LOW && antPlanto == HIGH) enviarAccion("planto");

  antNueva  = bNueva;
  antCarta  = bCarta;
  antPlanto = bPlanto;

  delay(50);
}

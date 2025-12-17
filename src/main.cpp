#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// ===== CAMBIA SOLO ESTO =====
const char* jugador = "miriam";   // nombre único (sin espacios)
// ============================

const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;

const char* topicTX = "instrumentacion/blackjack";
char topicRX[64];

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient client(espClient);

// Pines botones (si tu diagrama usa otros, cámbialos)
const int PIN_NUEVA  = 25;
const int PIN_CARTA  = 26;
const int PIN_PLANTO = 33;

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

void dibujarCabecera() {
  lcd.setCursor(0,0); lcd.print("C: ");
  lcd.setCursor(12,0); lcd.print("$:");
  lcd.setCursor(0,1); lcd.print("J: ");
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

  lcd.clear();
  dibujarCabecera();

  lcd.setCursor(3,0);  lcd.print(crup);
  lcd.setCursor(14,0); lcd.print(fondos);
  lcd.setCursor(3,1);  lcd.print(jug);
  lcd.setCursor(15,1); lcd.print(estado2letra(est));
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
  dibujarCabecera();
}

void loop() {
  if (!client.loop()) conectarMQTT();

  int bNueva  = digitalRead(PIN_NUEVA);
  int bCarta  = digitalRead(PIN_CARTA);
  int bPlanto = digitalRead(PIN_PLANTO);

  if (bNueva == LOW && antNueva == HIGH)  enviarAccion("nueva");
  if (bCarta == LOW && antCarta == HIGH)  enviarAccion("carta");
  if (bPlanto == LOW && antPlanto == HIGH) enviarAccion("planto");

  antNueva  = bNueva;
  antCarta  = bCarta;
  antPlanto = bPlanto;

  delay(50);
}

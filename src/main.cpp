#include <Arduino.h>

#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);

// el NOMBRE debe de ser unico para cada uno
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* jugador = "NOMBRE";
bool nueva = 0, planto = 0, carta = 0;
bool nueva_ant = 0, planto_ant = 0, carta_ant = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void PubMQTT(int estado)
{   
  char str[20];

     client.publish("instrumentacion/blackjack", "{\"jugador\": \"Carlos\", \"accion\": \"nueva\"}");
 
  Serial.println(estado);
}

void lectura(char* topic, byte* payload, unsigned int length) 
{
  Serial.print(topic);
  Serial.print(" : ");
  char message[length+1]={0x00};

  for(int i=0;i<length;i++)
    message[i]=(char)payload[i];

  message[length]=0x00;
  Serial.println(message);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("C: ");
  const char* crup = doc["crupier"];
  lcd.print(crup);
  
 
}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");
  WiFi.begin("Wokwi-GUEST", "", 6);
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(33, INPUT_PULLUP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to ");
  lcd.setCursor(0, 1);
  lcd.print("MQTT...");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(lectura);
  while (!client.connected())
  {    Serial.print("Connecting to MQTT...");
      if (client.connect("blackjack_NOMBRE" )) {
          Serial.println("connected");
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("LISTO");
          lcd.setCursor(0, 1);
          lcd.print("CONECTADO");
      }
      else
        {  Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
        }
  }
  client.subscribe("instrumentacion/NOMBRE");
}

void loop() {
    nueva_ant = nueva;

    nueva = !digitalRead(33);

    if (!client.loop()) {
      Serial.println("Desconectado");
      while (!client.connected())
      {    Serial.print("Connecting to MQTT...");
          if (client.connect("blackjack_NOMBRE" )) {
              Serial.println("connected");
              client.subscribe("instrumentacion/NOMBRE");
          }
          else
            {  Serial.print("failed with state ");
              Serial.print(client.state());
              delay(2000);
            }
      }
    }
    if ((nueva == 1)&&(nueva_ant == 0)) PubMQTT(1);

    delay(100); 
}

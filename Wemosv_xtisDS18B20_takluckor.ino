#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
// #include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Wemos D1/nodemcu pinout
#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0 // 10k pullup
#define D4 2 // 10k pullup Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // 10k pullup SPI Bus SS (CS)
// enbart nodemcu #define D9 3 // RX0 (Serial console)
// enbart nodemcu #define D10 1 // TX0 (Serial console)
#define ONE_WIRE_BUS D1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress ute = { 0x28,  0xAA,  0x47,  0x45, 0x1A,  0x13,  0x2,  0x71 };
DeviceAddress inne = { 0x28,  0xAA,  0xC,  0x57,  0x1A,  0x13,  0x2,  0xD7 };
const int fuktmatn = A0;
const char* fuktijord;
const int tunnan = D3;
const int pumppin = D7;
const int ventpin1 = D4;
int vind = D2;
const int takupp = D5;
const int takner = D6;
const char* ssid     = "willywiny";
const char* password = "willy123";
// const char* host = "192.168.0.15";
IPAddress ip(192, 168, 0, 16);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dnServer(192, 168, 0, 1);
const char* mqttServer = "192.168.0.15";
const int mqttPort = 1883;
const char* mqttuser = "DVES_USER";
const char* mqttpass = "DVES_PASS";
WiFiClient espClient;
PubSubClient client(espClient);
int tunna = 0;
int temp = D1;
int x = 0;
int pulse = 0;
long lastmsg = 0;
float i = 0;
float u = 0;
float t = 0;


void setup() {
  pinMode(vind, INPUT_PULLUP);
  attachInterrupt(vind, count_pulse, FALLING);
  pinMode(temp, INPUT);
  pinMode(tunnan, INPUT);
  pinMode(pumppin, OUTPUT);
  pinMode(takupp, OUTPUT);
  pinMode(takner, OUTPUT);
  digitalWrite(pumppin, HIGH);
  digitalWrite(takupp, HIGH);
  digitalWrite(takner, HIGH);
  pinMode(ventpin1, OUTPUT);
  digitalWrite(ventpin1, HIGH);
  sensors.begin();
  sensors.setResolution(ute, 12);
  sensors.setResolution(inne, 12);
  Serial.begin(115200);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet, dnServer);
  delay(200);
  WiFi.begin(ssid, password);
  delay(1000);
  client.setServer(mqttServer, mqttPort);
  client.connect("vaxtis", mqttuser, mqttpass);
  client.publish("vaxthus/tunnan", "1");
  client.publish("vaxthus/pump", "0");


}
void reconnect() {
  while (!client.connected()) {
    Serial.println("ansluter igen");
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet, dnServer);
    delay(500);
    WiFi.begin(ssid, password);
    delay(1000);
    //    client.setServer(mqttServer, mqttPort);
    if (client.connect("vaxtis", mqttuser, mqttpass));
    else {
      delay(2000);
    }
  }
}

void count_pulse()
{
  pulse++;
}
void vinden()
{
  pulse = 0;
  interrupts();
  delay(10000);
  noInterrupts();
  client.publish("vaxthus/vind", String(pulse).c_str());
  delay(300);
  sensors.requestTemperatures();
  t = sensors.getTempC(inne);
  delay(300);
  if (pulse < 180 && t > 28 && x < 1)
  {
    digitalWrite(takupp, LOW);
    client.publish("vaxthus/taket", "1");
    delay(10000);
    client.publish("vaxthus/taket", "1");
    delay(10000);
    client.publish("vaxthus/taket", "1");
    delay(5000);
    client.publish("vaxthus/taket", "1");
    digitalWrite(takupp, HIGH);
    client.publish("vaxthus/taket", "1");
    x = 2;
  }
  else if (pulse > 220 && x > 1)
  {
    digitalWrite(takner, LOW);
    delay(10000);
    client.publish("vaxthus/taket", "0");
    delay(10000);
    client.publish("vaxthus/taket", "0");
    delay(6000);
    client.publish("vaxthus/taket", "0");
    digitalWrite(takner, HIGH);

    x = 0;
  }
  else if (t < 25 && x > 1)
  {
    digitalWrite(takner, LOW);
    delay(10000);
    client.publish("vaxthus/taket", "0");
    delay(10000);
    client.publish("vaxthus/taket", "0");
    delay(6000);
    client.publish("vaxthus/taket", "0");
    digitalWrite(takner, HIGH);
    x = 0;
  }
}
void tempen()
{
  // Läsa av lufttemp & jordfuktighet
  //  delay(250);
  sensors.requestTemperatures();
  i = sensors.getTempC(inne);
  u = sensors.getTempC(ute);
  int j = analogRead(fuktmatn);
  String temperature = String(i);
  String humidity = String(u);
  String fuktijord = String(j);
  // mqtt sträng jordfuktighet
  fuktijord = map(j, 15, 900, 15, 900);
  int length = fuktijord.length();
  char msgBuffer[length];
  fuktijord.toCharArray(msgBuffer, length + 1);
  client.publish("vaxthus/jordfukt", msgBuffer);
  delay(1000);
  // mqtt sträng för temp & luftfuktighet
  client.publish("vaxthus/innetemp", String(i).c_str());
  delay(1000);
  client.publish("vaxthus/utetemp", String(u).c_str());
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  tunna = digitalRead(tunnan);
  //   client.publish("vaxthus/tunnan", "1");
  delay(300);
  while (tunna == HIGH) {
    if (!client.connected()) {
      reconnect();
    }
    client.publish("vaxthus/tunnan", "0");
    delay(500);
    client.publish("vaxthus/pump", "0");
    vinden();
    client.publish("vaxthus/pump", "0");
    tempen();
    delay(300);
    tunna = digitalRead(tunnan);
  }
  {
    client.publish("vaxthus/pump", "0");
    delay(1000);
    client.publish("vaxthus/tunnan", "1");
    vinden();
    client.publish("vaxthus/tunnan", "1");
    tempen();
    delay(300);
    sensors.requestTemperatures();
    t = sensors.getTempC(inne);
    //    float t = dhtin.readTemperature();
    delay(300);
    int j = analogRead(fuktmatn);
    delay(300);
    if (j < 670 && t < 25 )
    {
      digitalWrite(pumppin, LOW);
      delay(1000);
      digitalWrite(ventpin1, LOW);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      digitalWrite(ventpin1, HIGH);
      client.publish("vaxthus/pump", "1");
      vinden();
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      delay(10000);
      client.publish("vaxthus/pump", "1");
      vinden();
      client.publish("vaxthus/pump", "1");
      //Vilapumpen
      tempen();
      delay(300);
      digitalWrite(pumppin, HIGH);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      vinden();
      client.publish("vaxthus/pump", "0");
      delay(10000);
      client.publish("vaxthus/pump", "0");
      delay(10000);
      sensors.requestTemperatures();
      t = sensors.getTempC(inne);
      //      float t = dhtin.readTemperature();
      delay(300);
      int j = analogRead(fuktmatn);
      delay(300);
    }
    else if (j > 685)
    {
      digitalWrite(pumppin, HIGH);
      //    client.publish("vaxthus/pump", "0");
      //   Serial.print("slutet");
      return;
    }
    //  else {
    //    digitalWrite(pumppin, HIGH);
    //    client.publish("vaxthus/pump", "0");
    //    return;
    //  }
  }
}

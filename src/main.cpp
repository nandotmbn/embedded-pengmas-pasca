#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "DFRobot_PH.h"

const char *ssid = "7";
const char *password = "77777777";

#define mqtt_port 1883
#define MQTT_USER "PO25"
#define MQTT_PASSWORD "12345678"
#define PUBLISH "/Pengabdian/dataKolam"
#define SUBSCRIBE "/Sub_Data"

#define PowerPin 32
#define GndPin 33
#define EC_PIN 39

const char *mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// set id alat
String id_alat = "P25.1";
String nokolam = "1";
String kirim = "", kode = "";

int lcdColumns = 16;
int lcdRows = 2;

const int tempPin = 15;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

OneWire oneWire(tempPin);
DallasTemperature tempSens(&oneWire);

const int pHPin = 36;
int pHVal, sumPH, avgPH;
float Voltage, pH, pHfix, tempC, sumsal, sumtemp, readtemp, salinitas, temperature = 25;
float doValue;
float doVal, doCom;
String dio = "", pHValue = "", temp = "", sal = "";
String DO = "0.000";
int en = 0;
int i, dly_esc = 0;
int sensorValue;
int j = 0;
DFRobot_PH ph;

void setup()
{
  pinMode(PowerPin, OUTPUT);
  pinMode(GndPin, OUTPUT);
  pinMode(EC_PIN, INPUT);
  digitalWrite(PowerPin, LOW);
  digitalWrite(GndPin, LOW);

  pinMode(pHPin, INPUT);
  tempSens.begin();
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  analogReadResolution(12);

  client.setServer(mqtt_server, 1883);
  Serial.begin(115200);
  ph.begin();
  Serial.println("TEST");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  i = 1;
  String clientId = "P25";
  client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
}

void pHresult()
{
  Voltage = analogRead(pHPin) / 4096.0 * 3300;
  pH = ph.readPH(Voltage, temperature);
  pHValue = String(pH);
  pHfix = pH + 1;

  Serial.println(pHfix);
  lcd.setCursor(9, 0);
  lcd.print("pH=");
  lcd.setCursor(12, 0);
  lcd.print(pHfix);
  delay(500);
}

void tempresult()
{
  sumtemp = 0;
  tempSens.requestTemperatures();

  for (int j = 0; j < 10; j++)
  {
    readtemp = tempSens.getTempCByIndex(0);
    sumtemp += readtemp;
    delay(20);
  }
  tempC = sumtemp / 10;
  tempC = 0.8082 * tempC + 4.778;
  temp = String(tempC);
  Serial.println(tempC);
  lcd.setCursor(0, 1);
  lcd.print("Deg=");
  lcd.setCursor(4, 1);
  lcd.print(tempC);
  delay(500);
}

void ecresult()
{
  sumsal = 0;
  for (j = 0; j < 5; j++)
  {
    sensorValue = analogRead(EC_PIN);
    digitalWrite(PowerPin, HIGH);
    delay(250);
    int adcValue = analogRead(EC_PIN);
    sumsal += adcValue;
    delay(250);
    digitalWrite(PowerPin, LOW);
  }
  delay(10000);

  float avgsal = sumsal / 5;
  if (avgsal < 870)
    salinitas = 0;
  else if (avgsal >= 870)
    salinitas = 0.00316 * avgsal - 2.82824;
  sal = String(salinitas);
  // Serial.print(salinitas);
  Serial.println(sal);
  lcd.setCursor(0, 0);
  lcd.print("Sal=");
  lcd.setCursor(4, 0);
  lcd.print(salinitas);

  // delay (10000);
  // delay(20000);
}

void doresult()
{
  doVal = random(400, 650) / 100;
  doCom = random(0, 100) / 1000;
  doValue = doVal + doCom;
  dio = String(doValue, 5);
  delay(500);
}

void showresult()
{
  // unsigned long int detik = millis();
  if (millis() > dly_esc + 60000)
  {
    ecresult();
    dly_esc = millis();
  }
  pHresult();
  tempresult();
  doresult();

  // lcd.setCursor(10, 1);
  // lcd.print("*=send");
  Serial.print("ADC = ");
  Serial.println(sensorValue);
  Serial.print("Salinitas = ");
  Serial.println(sal);
  Serial.print("pH = ");
  Serial.println(pH);
  Serial.print("Suhu = ");
  Serial.println(temp);
  Serial.print("DO = ");
  Serial.println(dio);

  i = 2;
}

void publishSerialData(const char* serialData, const char* topic)
{
  if (!client.connected())
  {
    //    client.publish(topic, serialData);
    // reconnect();
    Serial.println("haha gagal yek");
  }
  client.publish(topic, serialData);
}

void senddata()
{
  kirim = id_alat + "/" + pHValue.substring(0, 5) + "," + temp.substring(0, 5) + "," + sal.substring(0, 5) + "," + dio.substring(0, 5) + "#";
  kode = id_alat + "." + nokolam;
  Serial.println(kirim);
  DynamicJsonDocument root(1024);
  Serial.print("id: ");
  Serial.println(id_alat);
  Serial.println("terkirim");

  root["id_hardware"] = id_alat;
  root["salinitas"] = salinitas;
  root["ph"] = pH;
  root["suhu"] = temp;
  root["do"] = dio;
  root["id_kolam"] = nokolam;

  char myBuffer[1023];
  serializeJson(root, myBuffer);
  publishSerialData(myBuffer, PUBLISH);
  client.loop();
  delay(15000);
  // delay(300000); //delay 5menit
  // delay(3600000); //delay 1jam
  i = 1;
}

void loop()
{
  switch (i)
  {
  case 1:
    showresult();
    break;
  case 2:
    senddata();
    break;
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.println("nungguin ya");
    String clientId = "P25";
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD))
    {
      Serial.println("conn");
      //      client.subscribe(SUBSCRIBE);
    }
    else
    {
      Serial.println("tidak conn");
    }
  }
}
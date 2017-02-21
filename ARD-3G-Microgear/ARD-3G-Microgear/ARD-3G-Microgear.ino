/*
  3G shield UC20 sent data to netpie.io

  SD card attached to SPI bus as follows:
    MOSI  - pin 11
    MISO  - pin 12
    SCK   - pin 13
    CS    - pin 10

*/

#include "TEE_UC20.h"
#include "SoftwareSerial.h"
#include <AltSoftSerial.h>
#include <SPI.h>
#include <SD.h>
#include "internet.h"
#include "uc_mqtt.h"
INTERNET net;
UCxMQTT mqtt;

//SIM AIS or True internet
#define APN "internet"
#define USER ""
#define PASS ""

// MQTT init
#define MQTT_SERVER      "gb.netpie.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          "gRYY1BDQDwDbAhU6"  //"QHlKImJECp2zVDtn"
#define MQTT_USER        "5yixIOa4YKKqo71"
#define MQTT_PASSWORD    "Dvb1hYXuWmrRyYfO8w5Vu8PbxKM=" //"g3saTxS7KYXjRpO+dMHXjTnewC4="  
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   0
#define MQTT_WILL_MESSAGE  0
#define OUTPUT_FILE  "dataLog.csv"
#define LED   13

AltSoftSerial mySerial;
File dataLog;

unsigned long previousmqtt = 0;
const long intervalmqtt = 5000;
byte _hour, _minute, _second, _day, _month, _year, _batt;
int _volume, _temp, _humid, _carbon, _methane, _fall, _light, _press;  
boolean _lidStatus, _flameStatus, _soundStatus;


void debug(String data) {
  Serial.println(data);
}

void firstSave() {
  dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  if (dataLog) {
    dataLog.println("Smart Trash");
    dataLog.print(_day);dataLog.print("/");dataLog.print(_month);dataLog.print("/");dataLog.print(_year);
    dataLog.print(",");
    dataLog.print("Time");
    dataLog.print(",");
    dataLog.print("volume");
    dataLog.print(",");
    dataLog.print("lidStatus");
    dataLog.print(",");
    dataLog.print("Temp");
    dataLog.print(",");
    dataLog.print("Humid");
    dataLog.print(",");
    dataLog.print("flameStatus");
    dataLog.print(",");
    dataLog.print("soundStatus");
    dataLog.print(",");
    dataLog.print("carbon");
    dataLog.print(",");
    dataLog.print("methane");
    dataLog.print(",");
    dataLog.print("falling");
    dataLog.print(",");
    dataLog.print("light");
    dataLog.print(",");
    dataLog.print("location");
    dataLog.print(",");
    dataLog.print("pressure");
    dataLog.print(",");
    dataLog.println("Battery level");
    dataLog.close();
  }
}

void saveSD() {
  dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  if (dataLog) {
    dataLog.println("Smart Trash");
    dataLog.print("Date");
    dataLog.print(",");
    dataLog.print("Time");
    dataLog.print(",");
    dataLog.print("volume");
    dataLog.print(",");
    dataLog.print("lidStatus");
    dataLog.print(",");
    dataLog.print("Temp");
    dataLog.print(",");
    dataLog.print("Humid");
    dataLog.print(",");
    dataLog.print("flameStatus");
    dataLog.print(",");
    dataLog.print("soundStatus");
    dataLog.print(",");
    dataLog.print("carbon");
    dataLog.print(",");
    dataLog.print("methane");
    dataLog.print(",");
    dataLog.print("falling");
    dataLog.print(",");
    dataLog.print("light");
    dataLog.print(",");
    dataLog.print("location");
    dataLog.print(",");
    dataLog.print("pressure");
    dataLog.print(",");
    dataLog.println("Battery level");
    dataLog.close();
  }
}

void callback(String topic , char *playload, unsigned char length)  {
  Serial.println();
  Serial.println(F("%%%%%%%%%%%%%%%%%%%%%%%%%%%%"));
  Serial.print(F("Topic --> "));
  Serial.println(topic);
  playload[length] = 0;
  String str_data(playload);
  Serial.print(F("Playload --> "));
  Serial.println(str_data);
}
void connect_server() {
  do
  {
    Serial.println(F("Connect Server"));
    Serial.println(F("wait connect"));
    if (mqtt.DisconnectMQTTServer())
    {
      mqtt.ConnectMQTTServer(MQTT_SERVER, MQTT_PORT);
    }
    delay(500);
    Serial.println(mqtt.ConnectState());
  }
  while (!mqtt.ConnectState());
  Serial.println(F("Server Connected"));
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  Serial.println(mqtt.ConnectReturnCode(ret));
  //  mqtt.Publish("/SmartTrash/gearname/data", "hello cmmc", false);
  //  mqtt.Subscribe("/SmartTrash/gearname/#");
}

void setup()  {
  Serial.begin(9600);
  gsm.begin(&mySerial, 9600);
  gsm.Event_debug = debug;
  SD.begin(10)

  Serial.println(F("UC20"));
  gsm.PowerOn();
  while (gsm.WaitReady()) {}
  Serial.print(F("GetOperator --> "));
  Serial.println(gsm.GetOperator());
  Serial.print(F("SignalQuality --> "));
  Serial.println(gsm.SignalQuality());

  Serial.println(F("Disconnect net"));
  net.DisConnect();
  Serial.println(F("Set APN and Password"));
  net.Configure(APN, USER, PASS);
  Serial.println(F("Connect net"));
  net.Connect();
  Serial.println(F("Show My IP"));
  Serial.println(net.GetIP());
  mqtt.callback = callback;
  connect_server();

  pinMode(LED, OUTPUT);
  firstSave();
  delay(1000);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousmqtt >= intervalmqtt) {
    float analog = analogRead(A0);
    Serial.println(String(analog, 3));
    digitalWrite(LED, HIGH);
    saveSD();
    mqtt.Publish("/SmartTrash/gearname/data", String(analog), true);
    previousmqtt = currentMillis;
    if (mqtt.ConnectState() == false) {
      Serial.println(F("Reconnect"));
      connect_server();
    }
  }
  digitalWrite(LED, LOW);
  mqtt.MqttLoop();
}


/*
  3G shield UC20 sent data to netpie.io

  SD card attached to SPI bus as follows:
    MOSI  - pin 11
    MISO  - pin 12
    SCK   - pin 13
    CS    - pin 5

*/

#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include <MemoryFree.h>
#include <SPI.h>
#include <SD.h>

#define SD_CS  5
#define OUTPUT_FILE  "dataLog.csv"

INTERNET net;
UCxMQTT mqtt;
File dataLog;
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

AltSoftSerial mySerial;

//unsigned long previous = 0;
//unsigned long previousmqtt = 0;
//const long intervalmqtt = 10000;
//int count = 100;

// init variable
byte _hour, _minute, _second, _day, _month, _year, _batt;
int _volume, _temp, _humid, _carbon, _methane, _pitch, _roll, _light, _press;
float _lat, _lon, _alt;
boolean _lidStatus, _flameStatus, _soundStatus;

void debug(String data) {
  Serial.println(data);
}

void firstSave() {
  dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  if (dataLog) {
    dataLog.println("Smart Trash");
    dataLog.print("Date");
    dataLog.print(",");
    dataLog.print("Time");
    dataLog.print(",");
    dataLog.print("volume(%)");
    dataLog.print(",");
    dataLog.print("lidStatus");
    dataLog.print(",");
    dataLog.print("Temp(C)");
    dataLog.print(",");
    dataLog.print("Humid(%RH)");
    dataLog.print(",");
    dataLog.print("flameStatus");
    dataLog.print(",");
    dataLog.print("soundStatus");
    dataLog.print(",");
    dataLog.print("carbon(ppm)");
    dataLog.print(",");
    dataLog.print("methane(ppm)");
    dataLog.print(",");
    dataLog.print("Pitch(degree)");
    dataLog.print(",");
    dataLog.print("Roll(degree)");
    dataLog.print(",");
    dataLog.print("light(lx)");
    dataLog.print(",");
    dataLog.print("location");
    dataLog.print(",");
    dataLog.print("pressure(Pa)");
    dataLog.print(",");
    dataLog.println("Battery level(%)");
    dataLog.close();
    delay(2000);
  }
}

void saveSD() {
  Serial.println("open sd");
  dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  if (dataLog) {
    dataLog.print(_day); dataLog.print("/"); dataLog.print(_month); dataLog.print("/"); dataLog.print(_year);
    dataLog.print(",");
    dataLog.print(_hour); dataLog.print("/"); dataLog.print(_minute); dataLog.print("/"); dataLog.print(_second);
    dataLog.print(",");
    dataLog.print(_volume);
    dataLog.print(",");
    dataLog.print(_lidStatus);
    dataLog.print(",");
    dataLog.print(_temp);
    dataLog.print(",");
    dataLog.print(_humid);
    dataLog.print(",");
    dataLog.print(_flameStatus);
    dataLog.print(",");
    dataLog.print(_soundStatus);
    dataLog.print(",");
    dataLog.print(_carbon);
    dataLog.print(",");
    dataLog.print(_methane);
    dataLog.print(",");
    dataLog.print(_pitch);
    dataLog.print(",");
    dataLog.print(_roll);
    dataLog.print(",");
    dataLog.print(_light);
    dataLog.print(",");
    dataLog.print(_lat); dataLog.print(":"); dataLog.print(_lon); dataLog.print(":"); dataLog.print(_alt);
    dataLog.print(",");
    dataLog.print(_press);
    dataLog.print(",");
    dataLog.println(_batt);
    dataLog.close();
    delay(2000);
  }
  Serial.println("close sd");
}

void setup()  {
  Serial.begin(9600);
  Serial.print(F("freeRam = "));
  Serial.println(freeMemory());

  SD.begin(SD_CS);
  dataLog = SD.open("data.txt", FILE_WRITE);
//  dataLog.println(random(analogRead(A0)));
//  dataLog.close();
//  SD.exists(SD_CS);
//  Serial.println(F("Save..."));

  pinMode(6, OUTPUT);
  digitalWrite(6, 0);
  Serial.println(F("UC20"));

  gsm.begin(&mySerial, 9600);
  gsm.Event_debug = debug;
  gsm.PowerOn();
  while (gsm.WaitReady()) {}
  Serial.println(F("Ready....."));
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


  //  _hour = 23; _minute = 40; _second = 15;
  //  _day = 16; _month = 2; _year = 2017;  _batt = 100;
  //  _volume = 10; _temp = 25; _humid = 50; _carbon = 300; _methane = 300; _pitch = 90; _roll = 90; _light = 150; _press = 10000;
  //  _lat = 18.783166; _lon = 98.9767063; _alt = 54.30;
  //  _lidStatus = false; _flameStatus = false; _soundStatus = false;
  //  firstSave();

}

void callback(String topic , char *playload, unsigned char length)  {
  Serial.println(F("%%%%%%%%%%%%%%%%%%%%%%%%%%%%"));
  Serial.print(F("Topic --> "));
  Serial.println(topic);
  playload[length] = 0;
  String str_data(playload);
  Serial.print(F("Playload --> "));
  Serial.println(str_data);
}

void connect_server() {
  do  {
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
  //  Serial.println(F("Server Connected"));
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  digitalWrite(6, 1);
  mqtt.Publish("/SmartTrash/gearname/data", String(analogRead(A0), 3), false);
  digitalWrite(6, 0);

  //  Serial.println(mqtt.ConnectReturnCode(ret));
  //  Serial.println("connect server done..");
  //  mqtt.Publish("/SmartTrash/gearname/data", "hello cmmc", false);
  //  mqtt.Subscribe("/SmartTrash/gearname/#");
  //  Serial.print("mqtt sent...");
  //  mqtt.Publish("/SmartTrash/gearname/data", String(analogRead(A0)), false);
  //  Serial.println("done");
  //  gsm.PowerOff();
  //  delay(2000);

  if (mqtt.ConnectState() == false) {
    //    Serial.println(F("Reconnect"));
    connect_server();
  }
}

void loop() {
  //  unsigned long currentMillis = millis();
  //  if (currentMillis - previousmqtt >= 10000) {
  //  Serial.print("open...");
  //  SD.begin(SD_CS);
  //  dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  //  if (dataLog) {
  //    dataLog.println("CMMC Trash");
  //    dataLog.close();
  //    Serial.println("close");
  //  }

  digitalWrite(6, 1);
  delay(100);
  digitalWrite(6, 0);
  delay(100);

  if (mqtt.ConnectState() == false) {
    connect_server();
  }
  //    mqtt.MqttLoop();
}


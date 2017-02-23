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

#define LED 6
AltSoftSerial mySerial;

unsigned long previousmqtt = 0;
const long intervalmqtt = 5000;
byte _hour, _minute, _second, _day, _month, _year, _batt;
int _volume, _temp, _humid, _carbon, _methane, _fall, _light, _press;
boolean _lidStatus, _flameStatus, _soundStatus;


void debug(String data) {
  Serial.println(data);
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
}

void setup()  {
  Serial.begin(9600);
  gsm.begin(&mySerial, 9600);
  gsm.Event_debug = debug;
  
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
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousmqtt >= intervalmqtt) {
    digitalWrite(LED, HIGH);
    mqtt.Publish("/SmartTrash/gearname/millis", String(currentMillis), false);
    previousmqtt = currentMillis;
    if (mqtt.ConnectState() == false) {
      Serial.println(F("Reconnect"));
      connect_server();
    }
  }
  digitalWrite(LED, LOW);
  mqtt.MqttLoop();
}


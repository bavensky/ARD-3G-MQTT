#include "TEE_UC20.h"
#include "SoftwareSerial.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include <MemoryFree.h>
//#include <SPI.h>
#include <SD.h>

#define SD_CS  53 
#define OUTPUT_FILE  "dataLog.csv"

INTERNET net;
UCxMQTT mqtt;
//File dataLog;

//SIM TRUE  internet
#define APN "internet"
#define USER ""
#define PASS ""

#define MQTT_SERVER      "gb.netpie.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          "gRYY1BDQDwDbAhU6"
#define MQTT_USER        "5yixIOa4YKKqo71"
#define MQTT_PASSWORD    "Dvb1hYXuWmrRyYfO8w5Vu8PbxKM="
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   0
#define MQTT_WILL_MESSAGE  0
File dataLog;

unsigned long previousmqtt = 0;
const long intervalmqtt = 10000;
int count = 100;

//AltSoftSerial mySerial;

void debug(String data)
{
  Serial.println(data);
}

void setup()  {
  Serial.begin(9600);
  Serial.print("freeMemory()= ");
  Serial.println(freeMemory());
  SD.begin(SD_CS);
  dataLog = SD.open("data.txt", FILE_WRITE);
  dataLog.println(random(count));
  dataLog.close();
  dataLog = SD.open("data.txt");
  while (dataLog.available()) {
    Serial.write(dataLog.read());
  }
  dataLog.close();
  SD.exists(SD_CS);

  pinMode(6, OUTPUT);
  digitalWrite(6, 0);
  //  pinMode(7);
  //  pinMode(10);

  //  SD.begin(SD_CS);
  //  File dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  //  if (dataLog) {
  //  dataLog.println("CMMC Trashhh");
  //  Serial.println("sd...");
  //  delay(2000);
  //  Serial.print(" done");
  //  dataLog.close();
  //  }
  //  SD.exists(SD_CS);

  gsm.begin(&Serial1, 9600);
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
  Serial.print("freeMemory()= ");
  Serial.println(freeMemory());
  mqtt.callback = callback;
  connect_server();
  Serial.print("freeMemory()= ");
  Serial.println(freeMemory());
}

void callback(String topic , char *playload, unsigned char length)
{
  Serial.println();
  Serial.println(F("%%%%%%%%%%%%%%%%%%%%%%%%%%%%"));
  Serial.print(F("Topic --> "));
  Serial.println(topic);
  playload[length] = 0;
  String str_data(playload);
  Serial.print(F("Playload --> "));
  Serial.println(str_data);
}
void connect_server()
{
  do
  {
    //    Serial.println(F("Connect Server"));
    Serial.println(F("wait connect"));
    if (mqtt.DisconnectMQTTServer())
    {
      mqtt.ConnectMQTTServer(MQTT_SERVER, MQTT_PORT);
    }
    delay(500);
    //    Serial.println(mqtt.ConnectState());
  }
  while (!mqtt.ConnectState());
  //  Serial.println(F("Server Connected"));
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  //  Serial.println(mqtt.ConnectReturnCode(ret));
  //  Serial.print("sent ");
  //  Serial.print(analogRead(A0));

  //  float analog = analogRead(A0);
  digitalWrite(6, 1);
  mqtt.Publish("/SmartTrash/gearname/data", String(analogRead(A0), 3), false);
  //  Serial.println(" done...");
  //  delay(5000);
  digitalWrite(6, 0);

  //  mqtt.Publish("ctrl", "hello world", false);
  //  mqtt.Subscribe("inTopic");

  if (mqtt.ConnectState() == false) {
    //    Serial.println(F("Reconnect"));
    connect_server();
  }
}
void loop() {
  //  unsigned long currentMillis = millis();
  //  if (currentMillis - previousmqtt >= intervalmqtt) {
  //    float analog = analogRead(A0);
  //    mqtt.Publish("/SmartTrash/gearname/data", String(analog, 3), false);
  //    Serial.print(analogRead(A0));
  //  Serial.println("loop ");
  //    previousmqtt = currentMillis;
  //
  //    if (mqtt.ConnectState() == false) {
  //      Serial.println(F("Reconnect"));
  //      connect_server();
  //    }
  //    Serial.println("done...");
  //  }
  //  mqtt.MqttLoop();

  //  count++;
  //  SD.begin(SD_CS);
  //  File dataLog = SD.open(OUTPUT_FILE, FILE_WRITE);
  //  if (dataLog) {
  digitalWrite(6, 1);
  delay(100);
  //    dataLog.print("CMMC Trash ");
  //    dataLog.println(count);
  //    dataLog.close();
  digitalWrite(6, 0);
  delay(100);
  //  }
  //  SD.exists(SD_CS);
  //  delay(2000);

  //  connect_server();

  if (mqtt.ConnectState() == false) {
    connect_server();
  }
}


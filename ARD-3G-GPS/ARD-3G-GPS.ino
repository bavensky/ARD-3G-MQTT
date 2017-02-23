#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include "gnss.h"
#include <Fat16.h>
#include <Fat16util.h>
#include <MemoryFree.h>

GNSS gps;
INTERNET net;
UCxMQTT mqtt;

const uint8_t CHIP_SELECT = 5;
SdCard card;
Fat16 file;
#define OUTPUT_FILE  "dataLog.csv"

//SIM AIS  internet
#define APN "internet"
#define USER ""
#define PASS ""

// MQTT init
#define MQTT_SERVER      "gb.netpie.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          "gRYY1BDQDwDbAhU6"
#define MQTT_USER        "5yixIOa4YKKqo71"
#define MQTT_PASSWORD    "Dvb1hYXuWmrRyYfO8w5Vu8PbxKM="
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   0
#define MQTT_WILL_MESSAGE  0

AltSoftSerial mySerial;

//void debug(String data) {
//  Serial.println(data);
//}

void setup()  {
  Serial.begin(9600);
  Serial.print(F("freeRam = "));
  Serial.println(freeMemory());


  pinMode(6, OUTPUT);
  digitalWrite(6, 0);

  gsm.begin(&mySerial, 9600);
  //  gsm.Event_debug = debug;
  Serial.println(F("UC20"));
  gsm.PowerOn();
  while (gsm.WaitReady()) {}
  Serial.print(F("GetOperator -- > "));
  Serial.println(gsm.GetOperator());
  Serial.print(F("SignalQuality -- > "));
  Serial.println(gsm.SignalQuality());

  Serial.println(F("Disconnect net"));
  net.DisConnect();
  Serial.println(F("Set APN and Password"));
  net.Configure(APN, USER, PASS);
  Serial.println(F("Connect net"));
  net.Connect();
  Serial.println(F("Show My IP"));
  Serial.println(net.GetIP());

  Serial.println(F("GPS Start"));
  gps.Start();
  for (int i = 1; i <= 5; i++) {
    Serial.println(gps.GetPosition());
    digitalWrite(6, 1);
    delay(3000);
    digitalWrite(6, 0);
  }

//  byte _hour, _minute, _second, _day, _month, _year, _batt;
//  byte _volume, _temp, _humid, _pitch, _roll;
//  int _carbon, _methane, _press, _alt;
//  word _light;
//  float _lat, _lon;
//  boolean _lidStatus, _flameStatus, _soundStatus;
//
//  card.begin(CHIP_SELECT);
//  Fat16::init(&card);
//  file.open(OUTPUT_FILE, O_CREAT | O_APPEND | O_WRITE);
//  file.isOpen();
//  Serial.print("Writing to: ");
//  Serial.println(OUTPUT_FILE);
//  file.print("A0 : ");
//  file.print(String(analogRead(A0)));
//  file.print("\r");
//  file.print("A1 : ");
//  file.print(analogRead(A1));
//  file.print("\n");
//  file.print("A00 : ");
//  file.print(String(analogRead(A0)));
//  file.print("\r");
//  file.print("A11 : ");
//  file.print(analogRead(A1));
//  file.print("\n");
//  Serial.println("Done");
//  file.close();
//  delay(2000);

  //  mqtt.callback = callback;
  connect_server();


}

//void callback(String topic , char *playload, unsigned char length)  {
//  Serial.println();
//  Serial.println(F(" % % % % % % % % % % % % % % % % % % % % % % % % % % % % "));
//  Serial.print(F("Topic -- > "));
//  Serial.println(topic);
//  playload[length] = 0;
//  String str_data(playload);
//  Serial.print(F("Playload -- > "));
//  Serial.println(str_data);
//}

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
  //  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  Serial.println(mqtt.ConnectReturnCode(ret));
  digitalWrite(6, 1);
  mqtt.Publish("/SmartTrash/gearname/data", String(analogRead(A0), 3), false);
  digitalWrite(6, 0);
}
void loop() {
  digitalWrite(6, 1);
  delay(100);
  digitalWrite(6, 0);
  delay(100);

  if (mqtt.ConnectState() == false) {
    Serial.println(F("Reconnect"));
    connect_server();
  }

  //  mqtt.MqttLoop();
}


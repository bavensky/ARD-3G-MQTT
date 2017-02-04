#include "TEE_UC20.h"
#include "SoftwareSerial.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
INTERNET net;
UCxMQTT mqtt;

//SIM AIS  internet
#define APN "internet"
#define USER ""
#define PASS ""

#define MQTT_SERVER      "mqtt.cmmc.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          ""
#define MQTT_USER        ""
#define MQTT_PASSWORD    ""
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   1
#define MQTT_WILL_MESSAGE  0


unsigned long previousmqtt = 0;
const long intervalmqtt = 3000;

AltSoftSerial mySerial;

void debug(String data)
{
  Serial.println(data);
}
void setup()
{
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
void loop()
{
  int analog = analogRead(A0);
  unsigned long currentMillis = millis();
  if (currentMillis - previousmqtt >= intervalmqtt)
  {
    mqtt.Publish("SmartTrash/cmmc", String(analog), false);
    Serial.print("Analog = ");
    Serial.println(analog);
    previousmqtt = currentMillis;
    if (mqtt.ConnectState() == false)
    {
      Serial.println(F("Reconnect"));
      connect_server();
    }
  }
  mqtt.MqttLoop();
}


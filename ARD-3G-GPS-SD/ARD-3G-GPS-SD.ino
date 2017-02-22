#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include "gnss.h"
//#include <Fat16.h>
//#include <Fat16util.h>
#include <MemoryFree.h>
#include <ArduinoJson.h>

GNSS gps;
INTERNET net;
UCxMQTT mqtt;

#define CHIP_SELECT 5
#define OUTPUT_FILE  "dataLog.csv"
//SdCard card;
//Fat16 file;

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

void debug(String data) {
  Serial.println(data);
}

void setup()  {
  byte _hour, _minute, _second, _day, _month, _year, _batt;
  byte _binID, _volume, _temp, _humid, _pitch, _roll;
  int _carbon, _methane, _press, _alt;
  word _light;
  //  float _lat, _lon;
  boolean _lidStatus, _flameStatus, _soundStatus;

  Serial.begin(9600);
  Serial.print(F("freeRam = "));
  Serial.println(freeMemory());

  pinMode(6, OUTPUT);
  digitalWrite(6, 0);
  /////////////////////////////3G//////////////////////////////////
  gsm.begin(&mySerial, 9600);
  gsm.Event_debug = debug;
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

  //////////////////////////////GPS//////////////////////////////
  Serial.println(F("Start GPS"));
  gps.Start();
  Serial.println(F("Enable NMEA"));
  gps.EnableNMEA();

  volatile char GNSS_data[75] = "";
  String gps_data = "";
  String gps_lat;
  String gps_lon;
  String gps_alt;

  for (int i = 0; i < 6; i++) {
    gps_data = gps.GetNMEA("GGA");
    digitalWrite(6, 1);
    delay(3000);
    digitalWrite(6, 0);
    //    Serial.print(F("GGA get : "));
    //    Serial.println(gps_data);
  }

  gps_lat = "";
  gps_lon = "";
  gps_alt = "";

  if (gps_data.substring(0, 6) == "$GPGGA")
  {
    gps_data.toCharArray(GNSS_data, 75);

    String gps_cal_s = gps_data.substring(18, 27);
    float gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    uint32_t gps_cal_l = gps_cal_f * 10000;  //10000000

    gps_lat += GNSS_data[16] ;
    gps_lat += GNSS_data[17] ;
    gps_lat += ".";
    gps_lat += gps_cal_l;
    gps_lat += GNSS_data[28] ;

    gps_cal_s = gps_data.substring(33, 42);
    gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    gps_cal_l = gps_cal_f * 10000; //10000000

    gps_lon += GNSS_data[30] ;
    gps_lon += GNSS_data[31] ;
    gps_lon += GNSS_data[32] ;
    gps_lon += ".";
    gps_lon += gps_cal_l;
    gps_lon += GNSS_data[43] ;

    gps_alt += GNSS_data[54];
    gps_alt += GNSS_data[55];
    gps_alt += GNSS_data[56];
    gps_alt += GNSS_data[57];
    gps_alt += GNSS_data[58];

    Serial.print(F("data = "));
    Serial.print(gps_data);
    Serial.print(F("  "));
    Serial.print(gps_lat);
    Serial.print(F("  "));
    Serial.print(gps_lon);
    Serial.print(F("  "));
    Serial.println(gps_alt);
    delay(1000);
  }
  ////////////////////////////JSON////////////////////////////
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["stat"];
  root["data"];
  root["loca"];

  _binID = 1;
  _volume = 5;
  _lidStatus = true;
  _temp = 25;
  _humid  = 40;
  _flameStatus = true;
  _soundStatus = false;
  _carbon = 5000;
  _methane = 5000;
  _light = 150;
  _pitch = 90;
  _roll = 90;
  _press = 10000;
  _batt = 100;
  _hour = 18;
  _minute = 55;
  _second = 30;
  _day = 22;
  _month = 2;
  _year = 2017;

  JsonArray& stat = root.createNestedArray("stat");
  stat.add(String("binId:" + String(_binID)));
  stat.add(String("volum:" + String(_volume)));
  stat.add(String("lid:"  + String(_lidStatus)));
  stat.add(String("flame:"  + String(_flameStatus)));
  stat.add(String("sound:"  + String(_soundStatus)));
  stat.add(String("pitch:" + String(_pitch)));
  stat.add(String("roll:" + String(_roll)));
  JsonArray& data = root.createNestedArray("data");
  data.add(String("temp:"  + String(_temp)));
  data.add(String("huid:" + String(_humid)));
  data.add(String("carbon:" + String(_carbon)));
  data.add(String("methane:" + String(_methane)));
  data.add(String("light:" + String(_light)));
  data.add(String("press:" + String(_press)));
  data.add(String("batt:" + String(_batt)));
  JsonArray& loca = root.createNestedArray("loca");
  loca.add(String("location" + gps_lat + "," + gps_lon + "," + gps_alt));

  //  data.add(String("hour:" + String(_hour)));
  //  data.add(String("minute:"  + String(_minute)));
  //  data.add(String("second:" + String(_second)));
  //  data.add(String("day:" + String(_day)));
  //  data.add(String("month:"  + String(_month)));
  //  data.add(String("year:" + String(_year)));
  root.printTo(Serial);
  ////////////////////////////SDCARD////////////////////////////
  //  card.begin(CHIP_SELECT);
  //  Fat16::init(&card);
  //  file.open(OUTPUT_FILE, O_CREAT | O_APPEND | O_WRITE);
  //  file.isOpen();
  //  Serial.println(F("Writing...."));
  //  file.print(root["data"]);
  //  file.println(gps_lat);
  //  file.print("Lon,");
  //  file.println(gps_lon);
  //  file.print("Alt,");
  //  file.println(gps_alt);
  //  file.close();
  //  delay(2000);
  //////////////////////////////MQTT////////////////////////////////
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
  Serial.println(F("Server Connected"));
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
  Serial.println(mqtt.ConnectReturnCode(ret));
  digitalWrite(6, 1);
  mqtt.Publish("/SmartTrash/gearname/status", root["stat"], false);
  delay(1000);
  mqtt.Publish("/SmartTrash/gearname/data", root["data"], false);
  delay(1000);
  mqtt.Publish("/SmartTrash/gearname/location", root["loca"], false);
  delay(1000);
  digitalWrite(6, 0);
  //  mqtt.callback = callback;
  //  connect_server();
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

//void connect_server() {
//  do  {
//    Serial.println(F("Connect Server"));
//    Serial.println(F("wait connect"));
//    if (mqtt.DisconnectMQTTServer())
//    {
//      mqtt.ConnectMQTTServer(MQTT_SERVER, MQTT_PORT);
//    }
//    delay(500);
//    Serial.println(mqtt.ConnectState());
//  }
//  while (!mqtt.ConnectState());
//  Serial.println(F("Server Connected"));
//  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);
//  Serial.println(mqtt.ConnectReturnCode(ret));
//  digitalWrite(6, 1);
//  mqtt.Publish("/SmartTrash/gearname/data", gps_lat, false);
//  mqtt.Publish("/SmartTrash/gearname/data", gps_lon, false);
//  mqtt.Publish("/SmartTrash/gearname/data", gps_alt, false);
//  digitalWrite(6, 0);
//}

void loop() {
  digitalWrite(6, 1);
  delay(100);
  digitalWrite(6, 0);
  delay(100);

  //  if (mqtt.ConnectState() == false) {
  //    Serial.println(F("Reconnect"));
  //    connect_server();
  //  }

  //  mqtt.MqttLoop();
}


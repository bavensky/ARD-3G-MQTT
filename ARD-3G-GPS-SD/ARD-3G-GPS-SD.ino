#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include "gnss.h"
//#include <Fat16.h>
//#include <Fat16util.h>
#include <MemoryFree.h>
//#include <ArduinoJson.h>

GNSS gps;
INTERNET net;
UCxMQTT mqtt;

//#define CHIP_SELECT 5
//#define OUTPUT_FILE  "dataLog.csv"
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

#define RX_buffer_size 100
volatile uint8_t RX_buffer[RX_buffer_size] = {0};
volatile uint16_t  RX_pointer = 0;
volatile uint16_t  Decode_pointer = 0;

uint32_t time_prev;

void debug(String data) {
  Serial.println(data);
}

void serialEvent() {
  while (Serial.available()) {
    RX_buffer[RX_pointer] = Serial.read();
    RX_pointer++;
    if (RX_pointer >= RX_buffer_size)RX_pointer = 0;
  }
}

void Decode(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  for (int8_t i = 0; i < 3 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }

  if ((tmp_header[0] & tmp_header[1] & tmp_header[2]) == header)
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];

    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (float)(tmp) * 0.01f;
    }
  }
}

void Decode_press(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[3] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  int16_t data_buffer = 0;
  for (int8_t i = 0; i < 3 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }

  if ((tmp_header[0] & tmp_header[1] & tmp_header[2]) == header)
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];

    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (float)(tmp);
    }
  }
}

void setup()  {
  byte _hour, _minute, _second, _day, _month, _year, _batt;
  byte _binID, _volume, _pitch, _roll; //_temp, _humid, ;
  float _temp, _humid; //  float _lat, _lon;
  int _carbon, _methane, _press, _alt;
  word _light;
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
  String gps_lat = "";
  String gps_lon = "";
  String gps_alt = "";

  for (int i = 0; i < 6; i++) {
    gps_data = gps.GetNMEA("GGA");
    digitalWrite(6, 1);
    delay(3000);
    digitalWrite(6, 0);
    //    Serial.print(F("GGA get : "));
    //    Serial.println(gps_data);
  }

  if (gps_data.substring(0, 6) == "$GPGGA") {
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
  } else {
    gps_lat = "";
    gps_lon = "";
    gps_alt = "";
  }

  ////////////////////////////FixVariable////////////////////////////
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
  //////////////////////////////UART////////////////////////////////
  //  while (Decode_pointer != RX_pointer) {
  //    Decode(&_temp, 0xff);
  //    Decode(&_humid, 0xfe);
  //    Decode(&_temp, 0xfd);
  //    Decode(&_temp, 0xfc);
  //    Decode(&_temp, 0xfb);
  //    Decode_press(&_temp, 0xfa);
  //    Decode(&_temp, 0xf9);
  //    Decode(&_temp, 0xf8);
  //    Decode_pointer++;
  //    if (Decode_pointer >= RX_buffer_size)Decode_pointer = 0;
  //  }
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
  mqtt.Publish("/SmartTrash/gearname/binID", String(_binID), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/volume", String(_volume), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/lid",    String(_lidStatus), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/temp",   String(_temp), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/humid",  String(_humid), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/flame",  String(_flameStatus), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/sound",  String(_soundStatus), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/carbon", String(_carbon), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/methane", String(_methane), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/light",  String(_light), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/pitch",  String(_pitch), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/rill",   String(_roll), false);
  //  mqtt.Publish("/SmartTrash/gearname/binID/sensor/press",  String(_press), false);
  mqtt.Publish("/SmartTrash/gearname/binID/sensor/batt",   String(_batt), false);
  delay(2000);
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

  uint32_t time_now = millis();
  if (time_now - time_prev >= 1000) {
    time_prev = time_now;
    while (Decode_pointer != RX_pointer) {
      Decode(&_temp, 0xff);
      Decode(&_humid, 0xfe);
      Decode_pointer++;
      if (Decode_pointer >= RX_buffer_size)Decode_pointer = 0;
    }
    Serial.print("data ");
    Serial.println(_temp);
  }

  //  if (_temp == 20) {
  //    digitalWrite(6, 1);
  //  } else {
  //    digitalWrite(6, 0);
  //  }
  //  Serial.print("Temp ");
  //  Serial.print(_temp);
  //  Serial.print(" Humid ");
  //  Serial.println(_humid);
  //  if (mqtt.ConnectState() == false) {
  //    Serial.println(F("Reconnect"));
  //    connect_server();
  //  }

  //  mqtt.MqttLoop();
}


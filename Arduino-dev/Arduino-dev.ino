#include "TEE_UC20.h"
#include <AltSoftSerial.h>
#include "internet.h"
#include "uc_mqtt.h"
#include "gnss.h"
//#include <MemoryFree.h>

GNSS gps;
INTERNET net;
UCxMQTT mqtt;

//SIM AIS  internet
#define APN "internet"  //"bmta.fleet"
#define USER ""
#define PASS ""

// MQTT init
//#define MQTT_SERVER      "gb.netpie.io"
//#define MQTT_PORT        "1883"
//#define MQTT_ID          "Eae5wlNVzEYdBjt4"
//#define MQTT_USER        "O3FpDaymo8ped6e"
//#define MQTT_PASSWORD    "WGZe92u/E3k2CyU2aFr+mUpCBac="

#define MQTT_SERVER      "gb.netpie.io"
#define MQTT_PORT        "1883"
#define MQTT_ID          "5H67q0xC2MPgrcsI"
#define MQTT_USER        "hCbZaydNBeO7OUo"
#define MQTT_PASSWORD    "VivNSv6YMDKmmVx+DDWPTYR1ZJM="
#define MQTT_WILL_TOPIC    0
#define MQTT_WILL_QOS      0
#define MQTT_WILL_RETAIN   0
#define MQTT_WILL_MESSAGE  0

//#define BINID 1150
//#define TOPIC "/SmartTrash/gearname/"
//String dataTopic = TOPIC + String(BINID);

#define BINID 0001
#define TOPIC "/DevByNat/gearname/"
String dataTopic = TOPIC + String(BINID);

AltSoftSerial mySerial;

#define SS_pin  A5
#define _volume_OK            1<<0
#define _lidStatus_OK         1<<1
#define _temp_OK              1<<2
#define _humid_OK             1<<3
#define _flameStatus_OK       1<<4
#define _soundStatus_OK       1<<5
#define _carbon_OK            1<<6
#define _methane_OK           1<<7
#define _light_OK             1<<8
#define _pitch_OK             1<<9
#define _roll_OK              1<<10
#define _press_OK             1<<11
#define _batt_OK              1<<12

#define LED 13
uint16_t is_data_OK = 0;

#define RX_buffer_size 140
volatile uint8_t RX_buffer[RX_buffer_size] = {0};
volatile uint16_t  RX_pointer = 0;
volatile uint16_t  Decode_pointer = 0;
volatile uint16_t prev = 0;

volatile char GNSS_data[75] = "";
String gps_data = "";
String gps_lat = "";
String gps_lon = "";
String gps_alt = "";

float _volume, _pitch, _roll, _batt, V_batt;
float _temp, _humid, _lat, _lon, _alt, _soundStatus;
uint16_t _lidStatus, _flameStatus, _press, _light, _carbon, _methane;

boolean subState = false;

byte count = 0;
byte countSub = 0;
float subTime = 0;

unsigned long preMillis = 0;

String data_s = "";

//////////////////////////////UART////////////////////////////////
void serialEvent() {
  while (Serial.available()) {
    RX_buffer[RX_pointer] = Serial.read();
    RX_pointer++;
    if (RX_pointer >= RX_buffer_size)RX_pointer = 0;
  }
}
uint8_t Decode(float* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[2] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  uint8_t OK = 0;
  for (int8_t i = 0; i < 2 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }
  if ((tmp_header[0] == header && tmp_header[1] == header))
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];
    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (float)(tmp) * 0.1f;
      OK = 1;
    }
  }
  return OK;
}
uint8_t Decode_press(uint16_t* value, uint8_t header)
{
  uint8_t tmp_pointer = Decode_pointer;
  uint8_t tmp_header[2] = {0};
  uint8_t tmp_data_pointer[3] = {0};
  uint8_t OK = 0;
  for (int8_t i = 0; i < 2 ; i++)
  {
    tmp_header[i] = RX_buffer[tmp_pointer];
    tmp_pointer++;
    if (tmp_pointer >= RX_buffer_size)tmp_pointer = 0;
  }
  if ((tmp_header[0] == header && tmp_header[1] == header))
  {
    tmp_data_pointer[0] = (uint8_t)RX_buffer[tmp_pointer + 0];
    tmp_data_pointer[1] = (uint8_t)RX_buffer[tmp_pointer + 1];
    tmp_data_pointer[2] = (uint8_t)RX_buffer[tmp_pointer + 2];
    if ((uint8_t)tmp_data_pointer[0] + (uint8_t)tmp_data_pointer[1] == (uint8_t)tmp_data_pointer[2])
    {
      uint16_t tmp = (uint16_t)tmp_data_pointer[0] << 8 | (uint16_t)tmp_data_pointer[1];
      * value = (uint16_t)(tmp);
      OK = 1;
    }
  }
  return OK;
}



//////////////////////////////calVoltage////////////////////////////////
float calculate_initial_capacity_percentage(float voltage)
{
  float capacity = 0;
  if (voltage > 4.2) {
    capacity = 100;
  } else if (voltage < 3.2 ) {
    capacity = 0;
  } else if (voltage > 4 ) {
    capacity = 80 * voltage - 236;
  } else if (voltage > 3.67 ) {
    capacity = 212.53 * voltage - 765.29;
  } else {
    capacity = 29.787234 * voltage - 95.319149 ;
  }
  return capacity;
}



//////////////////////////////callback////////////////////////////////
void callback(String topic , char *playload, unsigned char length)  {
  //  Serial.println();
  //  Serial.println(F("%%%%%%%%%%%%%%%%%%%%%%%%%%%%"));
  //  Serial.print(F("Topic --> "));
  //  Serial.println(topic);
  playload[length] = 0;
  String str_data(playload);
  //  Serial.print(F("Playload --> "));
  Serial.println(str_data);
  subTime = str_data.toFloat();
}



//////////////////////////////SentValue////////////////////////////////
void Sent_value(uint8_t header, float* tmp) {
  uint16_t data_buffer = (float) * tmp * 100.f;
  uint8_t sant_tmp[2] = {0};
  sant_tmp[0] = (uint16_t)data_buffer >> 8;
  sant_tmp[1] = (uint16_t)data_buffer >> 0;
  Serial.write(header);
  Serial.write(header);
  Serial.write(header);
  Serial.write(sant_tmp[0]);
  Serial.write(sant_tmp[1]);
  Serial.write(sant_tmp[0] + sant_tmp[1]);
  Serial.println();
}



//////////////////////////////mainSETUP////////////////////////////////
void setup()  {
  Serial.begin(9600);
  pinMode(SS_pin, OUTPUT);
  pinMode(LED, OUTPUT);

  /////////////////////////////3G//////////////////////////////////
  gsm.begin(&mySerial, 9600);
  gsm.PowerOn();
  while (gsm.WaitReady()) {}

  //////////////////////////////GPS//////////////////////////////
  gps.Start();
  gps.EnableNMEA();
  gps_data = gps.GetNMEA("GGA");

  while ((gps_data.substring(0, 8) == "$GPGGA,," || gps_data.substring(0, 8) == "Please W") && count <= 15) {
    gps_data = gps.GetNMEA("GGA");
    Serial.println(gps_data);
    delay(1000);
    count += 1;
  }

  if (gps_data.substring(0, 6) == "$GPGGA" && count != 30) {
    gps_data.toCharArray(GNSS_data, 75);

    String gps_cal_s = gps_data.substring(18, 27);
    float gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    uint32_t gps_cal_l = gps_cal_f * 10000000;

    gps_lat += GNSS_data[16] ;
    gps_lat += GNSS_data[17] ;
    gps_lat += ".";
    gps_lat += gps_cal_l;
    gps_lat += GNSS_data[28] ;

    gps_cal_s = gps_data.substring(33, 42);
    gps_cal_f = gps_cal_s.toFloat();
    gps_cal_f /= 60.0f;
    gps_cal_l = gps_cal_f * 10000000;

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

    //    Serial.print(gps_lat);
    //    Serial.print(F("  "));
    //    Serial.print(gps_lon);
    //    Serial.print(F("  "));
    //    Serial.println(gps_alt);
    //    delay(1000);
    //    Serial.println(F("Stop GPS"));
  }
  gps.Stop();
  gps.DisableNMEA();



  //////////////////////////////MQTT////////////////////////////////
  mqtt.callback = callback;
  //  Serial.println(F("MQTT Connected"));
  net.DisConnect();
  net.Configure(APN, USER, PASS);
  net.Connect();

  do  {
    if (mqtt.DisconnectMQTTServer())
    {
      mqtt.ConnectMQTTServer(MQTT_SERVER, MQTT_PORT);
    }
  }
  while (!mqtt.ConnectState());
  unsigned char ret = mqtt.Connect(MQTT_ID, MQTT_USER, MQTT_PASSWORD);

  data_s = gps_lat + ","  + gps_lon + "," + gps_alt;
  mqtt.Publish(dataTopic + "/gps", data_s, false);

  mqtt.Subscribe("/DevByNat/gearname/config");

  count = 0;  // clear count state
}



//////////////////////////////mainLOOP////////////////////////////////
void loop() {
  unsigned long nowMillis = millis();
  if (nowMillis - preMillis >= 500) {

    if ((subTime != 0 || countSub >= 20) && subState == false) {
      subState = true;
    }

//    mqtt.MqttLoop();
    preMillis = nowMillis;
    countSub += 1;
  }

  if (subState == true) {
    Sent_value(0xf1, &subTime);
    delay(400);
    count += 1; //  count time out uart data

    while (Decode_pointer != RX_pointer) {
      if ((is_data_OK & _volume_OK) == 0) {
        if (Decode(&_volume, 0xff)) is_data_OK |= _volume_OK;
      }
      if ((is_data_OK & _lidStatus_OK) == 0) {
        if (Decode_press(&_lidStatus, 0xfe))  is_data_OK |= _lidStatus_OK;
      }
      if ((is_data_OK & _temp_OK) == 0) {
        if (Decode(&_temp, 0xfd)) is_data_OK |= _temp_OK;
      }
      if ((is_data_OK & _humid_OK) == 0) {
        if (Decode(&_humid, 0xfc))   is_data_OK |= _humid_OK;
      }
      if ((is_data_OK & _flameStatus_OK) == 0) {
        if (Decode_press(&_flameStatus, 0xfb))  is_data_OK |= _flameStatus_OK;
      }
      if ((is_data_OK & _soundStatus_OK) == 0) {
        if (Decode(&_soundStatus, 0xfa))  is_data_OK |= _soundStatus_OK;
      }
      if ((is_data_OK & _carbon_OK) == 0) {
        if (Decode_press(&_carbon, 0xf9))  is_data_OK |= _carbon_OK;
      }
      if ((is_data_OK & _methane_OK) == 0) {
        if (Decode_press(&_methane, 0xf8))  is_data_OK |= _methane_OK;
      }
      if ((is_data_OK & _light_OK) == 0) {
        if (Decode_press(&_light, 0xf7))  is_data_OK |= _light_OK;
      }
      if ((is_data_OK & _pitch_OK) == 0) {
        if (Decode(&_pitch, 0xf6))  is_data_OK |= _pitch_OK;
      }
      if ((is_data_OK & _roll_OK) == 0) {
        if (Decode(&_roll, 0xf5)) is_data_OK |= _roll_OK;
      }
      if ((is_data_OK & _press_OK) == 0) {
        if (Decode_press(&_press, 0xf4))  is_data_OK |= _press_OK;
      }
      if ((is_data_OK & _batt_OK) == 0) {
        if (Decode(&V_batt, 0xf3))  is_data_OK |= _batt_OK;
      }

      //      Serial.println(is_data_OK, BIN);
      Decode_pointer++;
      if (Decode_pointer >= RX_buffer_size)Decode_pointer = 0;
    }

    //    Serial.print(F("_batt ")); Serial.print(V_batt, 1);
    //    Serial.print(F("_volume ")); Serial.print(_volume, 1);
    //    Serial.print(F(" _lidStatus "));  Serial.print(_lidStatus, 1);
    //    Serial.print(F(" _temp "));  Serial.print(_temp, 1);
    //    Serial.print(F(" _humid "));  Serial.print(_humid, 1);;
    //    Serial.print(F(" _flameStatus "));  Serial.print(_flameStatus, 1);
    //    Serial.print(F(" _soundStatus "));  Serial.print(_soundStatus, 1);
    //    Serial.print(F(" _carbon "));  Serial.print(_carbon, 1);
    //    Serial.print(F(" _meth "));  Serial.print(_methane, 1);
    //    Serial.print(F(" _light "));  Serial.print(_light, 1);
    //    Serial.print(F(" _pitch "));  Serial.print(_pitch, 1);
    //    Serial.print(F(" _roll "));  Serial.print(_roll, 1);
    //    Serial.print(F(" _press "));  Serial.print(_press, 1);

    Serial.print(F("is_data_OK "));  Serial.println(is_data_OK, BIN);

    if ((is_data_OK == 0x1fff || count > 20) &&  subState == true)
    {
      _batt = calculate_initial_capacity_percentage(V_batt);

      data_s = String(_volume) + "," + String(_lidStatus) + "," + String(_temp) + ","
               + String(_humid) + "," + String(_flameStatus);
      mqtt.Publish(dataTopic + "/data1", data_s, false);

      data_s = String(_pitch) + "," + String(_roll) + "," + String(_press) + "," + String(_batt);
      mqtt.Publish(dataTopic + "/data2", data_s, false);

      data_s = String(_soundStatus) + "," + String(_carbon) + "," + String(_methane) + "," + String(_light);
      mqtt.Publish(dataTopic + "/data3", data_s, false);

      //      gsm.PowerOff();
      //      digitalWrite(SS_pin, HIGH); // tell STM32 Mission Complete
      //      Serial.println(F("END..."));
      //      while (1);

      digitalWrite(LED, LOW);
//            delay(15000);

      count = 0;
      countSub = 0;
      subState = false;
      subTime = 0;
    }
  }
  digitalWrite(LED, HIGH);
  mqtt.MqttLoop(); // wating for subscribe
  delay(1000);
}


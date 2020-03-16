#include <ESP8266WiFi.h>
#include <WiFiManager.h>  
#include <DNSServer.h>
#include <PubSubClient.h>
#include <SparkFunCCS811.h>
#include <ArduinoJson.h>
#include <ClosedCube_HDC1080.h>
#include <Ticker.h>
#include <RCSwitch.h>

Ticker secondTick;
Ticker thirdTick;

#define CCS811_ADDR 0x5A    //Alternate I2C Address
CCS811 myCCS811(CCS811_ADDR);
ClosedCube_HDC1080 myHDC1080;

#define DEBUG 0
#if DEBUG
#define  P_R_I_N_T(x)   Serial.println(x)
#else
#define  P_R_I_N_T(x)
#endif

RCSwitch mySwitch = RCSwitch();

// Ticker for watchdog


//MQTT  credentials

const char *mqtt_server = "139.59.80.128";
const char *device_data   = "dsiot/gphouse/dbox/dev_data";
const char *device_config   = "dsiot/gphouse/dbox/dev_config";
const char *device_response = "dsiot/gphouse/dbox/dev_response";
const char *mqtt_user = "dsgp";
const char *mqtt_pass = "Gp@2o2o";
const int mqttPort = 1883;
int mqttTryCounter=0;

//-----device ID----------//
const char* dmac = "GP SensBoard 101";
const char* did = "gp101";


WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousMillis3 = 0;
long wifi_check_interval = 5000;

unsigned long previousMillis = 0;
long publish_interval = 300000;

unsigned long previousMillis2 = 0;
long deep_sleep_interval = 65000;



//char sleep_time[] = "20";
int sleep_time = 20; //in seconds

//---------------Difining Sensor Pin----------------------------------------------//
int button = 4;
int analogPin = A0;
int sensorValue;
int Temperature = 0;
int Humidity = 0;
int carbon_dioxide = 0;
int total_voc = 0;
int methane = 0;
//led pin

//-------------------------Variable to store sensor data----------------------------//
int data1 = 0;
int data2 = 0;
int data3 = 0;
int data4 = 0;
int data5 = 0;


//int sleep_time = 20; //in seconds
int tgs_upper_limit = 800;
int temp_upper_limit = 45;

int temp_alarm = 0;
int tgs_alarm = 0;
int alarm_flag = 0;


String data = "";

volatile int watchdogCount = 0;
char sensorData[120];


extern "C" {
#include "user_interface.h"
}



//--------------------Watchdog----------------------//

void ISRwatchdog() {
  watchdogCount++;
  if (watchdogCount == 180) {
    P_R_I_N_T("Watchdog bites!!!");
    ESP.reset();
  }
}




//--------------------------------Main Setup----------------------------------------------------//

void setup() {
//  int D4 = 2;
  pinMode(D4, OUTPUT);
  secondTick.attach(1, ISRwatchdog);// Attaching ISRwatchdog function
  Serial.begin(115200);
  myHDC1080.begin(0x40);
  delay(10);
  mySwitch.enableTransmit(D6); //gpio 0 // RF pin
  //It is recommended to check return status on .begin(), but it is not required.
  CCS811Core::status returnCode = myCCS811.begin();
  delay(10);
  if (returnCode != CCS811Core::SENSOR_SUCCESS) 
  {
//    P_R_I_N_T(".begin() returned with an error.");
//    while (1);      //No reason to go further
    client.publish(device_response ,"sensor error");
    P_R_I_N_T(".begin() returned with an error.");
  }
  pinMode(analogPin, INPUT);
 
  
  client.setServer(mqtt_server,mqttPort);//Connecting to broker
 
  client.setCallback(callback); // Attaching callback for subscribe mode
  wifi_manager();
  delay(50);
  
}//setup ends




//------------------------------------Main Loop--------------------------------------------------//
void loop(){

  watchdogCount = 0;
    
  if (myCCS811.dataAvailable()) {
      myCCS811.readAlgorithmResults();
    }
  thirdTick.attach(1.5, tick);  
 
  unsigned long currentMillis = millis();
//  wifi_manager();
  if(currentMillis - previousMillis3 > wifi_check_interval) {
    previousMillis3 = currentMillis;
    
//    P_R_I_N_T(previousMillis3); 
    
    if (WiFi.status() != WL_CONNECTED){ 
      //  set_wifi();
//        wifi_manager();
      P_R_I_N_T(" unable to connect wifi, returning to loop");
      return loop();
    }
    else {
      P_R_I_N_T("Wifi already connected");
    }
}

  if (!client.connected() && (WiFi.status() == WL_CONNECTED)){

    reconnect();
  }

   client.loop();

    //need to work here  
    data = sensor_data();
    
    
    data.toCharArray(sensorData,120);
    P_R_I_N_T("Sendor data: " + data);
    
    if(currentMillis - previousMillis > publish_interval) {
    
        previousMillis = currentMillis;
        P_R_I_N_T("Ticking every 50 seconds"); 
        P_R_I_N_T("Inside sensor data publish");
        
        if (!client.connected()){ reconnect();}
        
        int result = client.publish(device_data,sensorData); 
        
//        P_R_I_N_T(result);
        if (result ==1){
          P_R_I_N_T("Pulished successfully");
        }else{
            P_R_I_N_T("Unable to publish");
        }
    }// Timer ends

}//LOOP ENDS

void rf_alarm(){
      
      delay(100);
      mySwitch.send(5555, 24);
      delay(500);
      P_R_I_N_T("RF is Triggered");
      
  }


//---------------------------------------Read Sensor Data---------------------------------------//
String sensor_data()
{ 
  P_R_I_N_T("Inside Sensor data read");
  data1 = temp(); 
  data2 = hum(); 
  data3 = co2();
  data4 = tvoc();
  data5 = tgsVal();

if ( data5 > tgs_upper_limit) {
      tgs_alarm= 1;
      delay(100);
      rf_alarm();
      P_R_I_N_T("GAS out of LIMIT"); 
      alarm_flag = 1; 
    }

if ( data1 > temp_upper_limit) {
      temp_alarm= 1;
      delay(100);
      rf_alarm();
      P_R_I_N_T("TEMP out of LIMIT"); 
      alarm_flag = 1; 
    }

  String msg2 = "";
  msg2 = msg2 + "{\"did\":" + "\""+ did + "\""+ "," + "\"tmp\":" + data1 + "," + "\"hum\":" + data2 + "," + "\"co2\":" + data3 + "," + "\"voc\":" + data4 + "," + "\"ch4\":" + data5 + "," + "\"tmp_alrt\":" + temp_alarm +  "," + "\"tgs_alrt\":" + tgs_alarm + "," + "\"src\":" + "\""+ "sensor" + "\""+  "}";

  if (alarm_flag == 1){
//  data = msg2;
  msg2.toCharArray(sensorData,120);
  int result = client.publish(device_data,sensorData);    
     if (result ==1){
          P_R_I_N_T("Pulished successfully");
          delay(500);
        }else{
            P_R_I_N_T("Unable to publish");
        }
  }

  temp_alarm = 0;
  tgs_alarm = 0;
  alarm_flag = 0;
  
  delay(200);       //........ 0.2 sec delay...........//
  return msg2;

}



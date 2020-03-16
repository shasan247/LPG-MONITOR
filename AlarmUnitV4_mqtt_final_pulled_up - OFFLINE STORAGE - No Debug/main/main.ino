/*
 Since my debouncing code works on interrupt on CHANGE, so whenever it was missing the second interrupt(change of level shifting), it would make the "state" variable remain stuck as TRUE and long press detection loop runs endlessly making the device unstable.
 So to solve this, here i am forcefully altering the state to false=0(Depressed) inside the long press detecton loop to stop it from running endlessly.
 Device behaviour: Now any infirm or improper button press will set off the long press detection loop and once counter reaches the threshold(after 3 seconds), it stops the siren that was mistakenly fired up.
*/

/*Sketch uses 320444 bytes (33%) 
Global variables use 33564 bytes (41%)*/

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include <Ticker.h>
#include<FS.h>



#define DEBUG 1

#if DEBUG
#define  P_R_I_N_T(x)   Serial.println(x)
#else
#define  P_R_I_N_T(x)
#endif


Ticker watchDogTicker, toggleTicker, sirenTicker;
RCSwitch mySwitch = RCSwitch();
//for LED status

//enter your file name
const char* CONFIG_FILE = "/myconfig.json";


// Connect to the WiFi
//const char* ssid = "DataSoft_WiFi";//DataSoft_WiFi
//const char* password = "support123";
//const char* mqtt_server = "broker.datasoft-bd.com";
const char* mqtt_server = "139.59.80.128";
const int mqttPort = 1883;
const char* did = "GPALARM001";

//mqtt OUT topic
const char* responseTopic = "dsiot/gphouse/dbox/gas";
//mqtt IN topic
const char* configTopic = "dsiot/gphouse/dbox/config";






WiFiClient espClient;
PubSubClient client(espClient);


//For Timer
unsigned long lastWiFiCheckTime = 0;
unsigned long lastReconnectTime = 0;//MQTT reconnect

unsigned long wifi_check_interval = 5000;



#define alarmPin 0 //D3
#define dataIn 2 //D4  ONBOARD LED
#define ledPin 14//D5 siren
#define STATUS_LED 4//D2
const int  sosButton = 12;//D6


//for RF msg
int msgCounter=0;



// Holds the current button state.
volatile int state = 1;
//volatile bool isPRESSED = false;

// Holds the last time debounce was evaluated (in millis).
volatile long lastDebounceTime = 0;

// The delay threshold for debounce checking.
const int debounceDelay = 30;// 30ms works good for push button


//for long press detection
unsigned long lastPressedTime = 0;

//for alarm pause timeout detection
unsigned long lastStopTime;
volatile bool SNOOZE = false;
unsigned long snoozeTime = 300000; //in ms.  5 minutes snooze time default

//For preventing realarming
bool isAlarming = false;




//--------------------Watchdog----------------------//
volatile unsigned int watchdogCount = 0;

void ISRwatchdog() {
  watchdogCount++;
//  Serial.println(watchdogCount);    
  if (watchdogCount >= 60) {
    Serial.println("Watchdog bites!!!");    
    ESP.reset();    
  }  
}




// Gets called by the interrupt.
void   ICACHE_RAM_ATTR   ISR() {
  // Get the pin reading.
  int reading = digitalRead(sosButton);

  // Ignore dupe readings.
  if(reading == state) return;

  boolean debounce = false;
  
  // Check to see if the change is within a debounce delay threshold.
  if((millis() - lastDebounceTime) <= debounceDelay) {
    debounce = true;
  }

  // This update to the last debounce check is necessary regardless of debounce state.
  lastDebounceTime = millis();

  // Ignore reads within a debounce delay threshold.
  if(debounce) return;

  // All is good, persist the reading as the state.
  state = reading;
//  isPRESSED = state;// if TRUE(HIGH),  then start counting after resetting counter
//  counter = 0;

  if(!state){//if button pin is read low
    if(isAlarming){P_R_I_N_T("Already Alarming!");}else{setAlarm_and_Publish("SOS",1); isAlarming = true;}         
    lastPressedTime = millis();    
  }
  
// Work with the value now.
//  Serial.println("button: " + String(reading));
  
}//ISR ENDS




void setup() {
    pinMode(alarmPin, OUTPUT);
    pinMode(ledPin, OUTPUT);//siren indicator
    pinMode(STATUS_LED, OUTPUT);
    pinMode(sosButton, INPUT);
  //  pinMode(sosButton, INPUT_PULLUP);

   //    pinMode(alarmPin, LOW);//to avoid making sound at startup but it stays alway HIGH, so can't use it here
  
    attachInterrupt(sosButton, ISR,  CHANGE);
    Serial.begin(115200);
    P_R_I_N_T("RX READY");
    delay(500);
    
    mySwitch.enableReceive(dataIn);  // Receiver on interrupt 0 => that is pin #2 

//    setup_wifi();
//    checkWiFi();
    callWiFiManager();
    setup_mqtt();

    watchDogTicker.attach(1, ISRwatchdog);// ISRwatchdog ticking every 1 second
    
    if(mountSPIFFS()){      loadConfigFile();    }else{Serial.println("An Error has occurred while mounting SPIFFS");}
}


void loop() {
    
    watchdogCount = 0;
    

    while(!state){          
      setLongPressTimer();
      yield();
    }

    if(SNOOZE){checkResumeTimer();}else{readRadioData();};
    
    
    setMqttReconnectInterval();
    
}//LOOP ENDS





void setAlarm_and_Publish(const char *type, const int state){
  
  digitalWrite(alarmPin, state);  
//  digitalWrite(ledPin, state);

  if(state){   sirenTicker.attach(0.25, rapidBlinkAlarmLed);  } else { sirenTicker.detach();    turnOffAlarmLed(); }
  
  
  
  StaticJsonDocument<128> doc;
  char buffer[128];
  doc["did"] = did;
  doc["alarm"] = state;
  doc["src"] = type;
  size_t n = serializeJson(doc, buffer);
  client.publish(responseTopic, buffer, n);  
}





void checkWiFi(){
    unsigned long now = millis();
    if(now - lastWiFiCheckTime > wifi_check_interval) {
      lastWiFiCheckTime = now;

      Serial.println(WiFi.status());
      
      if (WiFi.status() != WL_CONNECTED){
          Serial.println(" calling wifi manager");
          wifi_manager();
          //set_wifi();
      }
      else {
        Serial.println(" Wifi already connected");
      }
    }
}



 //Warning! it is being called every time device reboots. If NO WiFi at the time of reboot, it falls back to AP Mode which is blocking code.
void callWiFiManager(){   
      
      if (WiFi.status() != WL_CONNECTED){
          P_R_I_N_T("Calling wifi manager . . .");
          wifi_manager();          
          //set_wifi();
      }
      else {
        P_R_I_N_T(" Wifi already connected");        
        reconnect();        
      }
    
}





//****************************NOT REQUIRED FOR WIFI MANAGER***************************
/*
void setup_wifi() {
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    P_R_I_N_T(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  P_R_I_N_T("");
  P_R_I_N_T("WiFi connected");
  P_R_I_N_T("IP address: ");
  P_R_I_N_T(WiFi.localIP());
}*/


void setup_mqtt(){      
   client.setServer(mqtt_server, mqttPort);
   client.setCallback(callback);
}


void setMqttReconnectInterval(){      
    if (!client.connected()) {
      
      unsigned long now = millis();
      if (now - lastReconnectTime > 30000) {
          lastReconnectTime = now;
          P_R_I_N_T("Ticking every 30 seconds");
          

//          P_R_I_N_T(WiFi.status());

          if(WiFi.status() == WL_CONNECTED){
              // Attempt to mqtt reconnect
              if (reconnect()) {
                
                lastReconnectTime = 0;//GOOD to make sure "now - lastReconnectTime" this line becomes true
              }
          }
          
          if(WiFi.status() != WL_CONNECTED){toggleTicker.attach(0.25, toggleLed);}//end of WL_CONNECTED       
 
      }//end of interval check
   }else{//if client connected
      client.loop();
   }
  
}//setMqttReconnect




void readRadioData(){
    if (mySwitch.available()) {
    
    Serial.print("Received ");

    unsigned long data = mySwitch.getReceivedValue();
    
    Serial.print( data );
    Serial.print(" / ");
    Serial.print( mySwitch.getReceivedBitlength() );
    Serial.print("bit ");
    Serial.print("Protocol: ");
    P_R_I_N_T( mySwitch.getReceivedProtocol() );


    msgCounter++;
    P_R_I_N_T(msgCounter);
    if(msgCounter >= 1){
        if(data == 5555){
          P_R_I_N_T("Matched");
          
          if(isAlarming){P_R_I_N_T("Already Alarming!");}else{setAlarm_and_Publish("RF",1);   isAlarming = true;}         
          
        }
        else{
          P_R_I_N_T("Didn't match");
        }

        msgCounter = 0;
    }
    mySwitch.resetAvailable();    
  } 
}





void setLongPressTimer(){

  unsigned long currentTime = millis();
  
  if(currentTime - lastPressedTime > 2000){
      
      setAlarm_and_Publish("SOS",0);
      Serial.printf("ALARM STOPPED FOR %d seconds\n",snoozeTime/1000);
      lastStopTime = millis();
      state = 1;//button state made UNPRESSED to make sure while loop does not go on if MCU missed the state change
      SNOOZE = true;
      isAlarming = false;
  }
        
}


void checkResumeTimer(){
  unsigned long now = millis();
  if(now - lastStopTime > snoozeTime){
      SNOOZE = false;//resume radio by cancelling snooze
  }
}




void onDemandAP(){
    Serial.println("OnDemandAP Starting...");

    // start ticker with 0.5 because we start in AP mode and try to connect
    toggleTicker.attach(1, toggleLed);
    
  //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(40);

    //it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);
    char buff[10];//max 9 digit AP name
//    did.toCharArray(buff, 10);  
    strcpy(buff, did);
    
    if (!wifiManager.startConfigPortal(buff,"support123")) {
      P_R_I_N_T("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    P_R_I_N_T("connected...yeey :)");
    toggleTicker.detach();
}

void rapidBlinkAlarmLed(){
    digitalWrite(ledPin, !digitalRead(ledPin));
}

void turnOffAlarmLed(){
    digitalWrite(ledPin, LOW);
}

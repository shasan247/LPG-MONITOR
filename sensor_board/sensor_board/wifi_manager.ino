//for LED status
Ticker ticker;
Ticker thirdticker;
void tick()
{
  int state = digitalRead(D4);  // get the current state of GPIO1 pin
  digitalWrite(D4, !state);     // set pin to the opposite state
}


void configModeCallback (WiFiManager *myWiFiManager) {
  P_R_I_N_T("Entered config mode");
  P_R_I_N_T(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  P_R_I_N_T(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.15, tick);
}


void wifi_manager (){
  
    // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.25, tick);
  P_R_I_N_T("AP Mode");
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
//  wifiManager.autoConnect("DS LPG Gas Detector");
  //reset settings - for testing
  //    wifiManager.resetSettings();

  char buff[10];//max 9 digit AP name
//  did.toCharArray(buff, 10);
  strcpy(buff, dmac);
  
  wifiManager.autoConnect(buff, "support123");//START AP AS DEVICE ID NAME
  
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(60);


  if (!wifiManager.autoConnect()) {
    P_R_I_N_T("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  P_R_I_N_T("CONNECTED TO WIFI...");
  ticker.detach();
  //keep LED on
  digitalWrite (D4, LOW);
  
  }

void wifi_manager_reset (){
  
    // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.25, tick);
  P_R_I_N_T("AP Mode");
  delay(100);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  delay(100);
  //reset settings - for testing
  wifiManager.resetSettings();
  Serial.println("EEPROM CLEARED");
  delay(100);

//  wifiManager.autoConnect("DS LPG Gas Detector");

  char buff[10];//max 9 digit AP name
//  did.toCharArray(buff, 10);
  strcpy(buff, dmac);
  
  wifiManager.autoConnect(buff, "support123");//START AP AS DEVICE ID NAME
  
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(60);


  if (!wifiManager.autoConnect()) {
    P_R_I_N_T("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  P_R_I_N_T("connected...");
  ticker.detach();
  //keep LED on
  digitalWrite (D4, LOW);
  
  }



//------------------------------------------------While client not conncected---------------------------------//



boolean reconnect() {
  
    toggleTicker.attach(0.25, toggleLed);//end of WL_CONNECTED

    P_R_I_N_T("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
  
    if (client.connect(clientId.c_str(), "dsgp", "Gp@2o2o")) {
      P_R_I_N_T("connected");        
      client.subscribe(configTopic);
      toggleTicker.detach();
      alwaysOnLed();
      //      delay(250);   
    }else{  
      Serial.print("failed, rc=");
      Serial.print(client.state());
      P_R_I_N_T(" try again");
//      mqttTryCounter++;
//      Serial.println(mqttTryCounter);
      // Wait 6 seconds before retrying
//      delay(5000); //blocking
    }
  return client.connected();
}// reconnect() ends

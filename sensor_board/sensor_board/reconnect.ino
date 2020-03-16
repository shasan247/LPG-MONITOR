//------------------------------------------------While client not conncected---------------------------------//

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  { 
    Serial.println("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if your MQTT broker has clientID,username and password
    //please change following line to if (client.connect(clientId,userName,passWord))

    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      P_R_I_N_T("connected to mqtt ");
//      delay(1500);
     //once connected to MQTT broker, subscribe command if any

      client.subscribe(device_config);
      delay(500);
      client.subscribe(device_response);
      delay(500);

                   
    }else {     
      Serial.println("failed, rc=");
      Serial.println(client.state());
      P_R_I_N_T(" try again");
      mqttTryCounter++;
      P_R_I_N_T(mqttTryCounter);    
    }


    if(mqttTryCounter==5){
      
      P_R_I_N_T("MQTT connection Failled ...");
//      client.setServer(public_mqtt_server,mqttPort);//Connecting to broker
      P_R_I_N_T(mqttTryCounter);
      mqttTryCounter=0;
//      break;
    }
    else{
      return loop();
      }
  }
}

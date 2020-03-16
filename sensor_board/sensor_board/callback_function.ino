//-----------------------Callback function-------------------------------------//

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.println("Message arrived in topic: ");
  P_R_I_N_T(topic);
  
//-------------------------------------resetting the device---------------------//

if(strcmp(topic, device_config) == 0){
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);

  String devId = doc["did"];
  int rst = doc["reset"];
  int g_limit = doc["gas_limit"];
  int t_limit = doc["temp_limit"];
  int pub_intrvl = doc["publish_interval"];
  
  P_R_I_N_T(devId);
  P_R_I_N_T(rst);
  P_R_I_N_T(g_limit);
  P_R_I_N_T(t_limit);
  P_R_I_N_T(pub_intrvl);

  
  if(( devId == did) || (devId == "gp000")){
      if(rst == 1){
        P_R_I_N_T("RESET");

        char buffer[256];
        doc["status"] = "success";
        doc["did"] = did;
        size_t n = serializeJson(doc, buffer);
        client.publish(device_response,buffer, n);
        delay(2000);
        ESP.reset();
      }
      else if(rst == 0){
        P_R_I_N_T("DEVICE NOT RESETED");
        }
      else{
        P_R_I_N_T("NO RESET COMMAND");

        char buffer[256];
        doc["did"] = did;
        doc["status"] = "failed";
        size_t n = serializeJson(doc, buffer);
        client.publish(device_response,buffer, n);
        delay(1500);
      }

  if( g_limit >=5 && g_limit <= 1000){ //sleep time within 2 hours
          P_R_I_N_T("GAS LIMIT UPDATED");
          tgs_upper_limit = g_limit;
          delay(2000);
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "success";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);  
        }
        else{
          P_R_I_N_T("INVALID GAS LIMIT");
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "failed";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);
          delay(1000);
        }
  if( t_limit >=5 && t_limit <= 60){ //sleep time within 2 hours
          P_R_I_N_T("TEMPARETURE LIMIT UPDATED");
          temp_upper_limit = t_limit;
          delay(2000);
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "success";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);  
        }
        else{
          P_R_I_N_T("INVALID TEMPARETURE LIMIT");
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "failed";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);
          delay(1000);
        }

  if( pub_intrvl >=15 && pub_intrvl <= 7200){ //sleep time within 2 hours
          P_R_I_N_T("DATA PUBLISH INTERVAL UPDATED");
          publish_interval = ((pub_intrvl*1000) -2.5);
          delay(2000);
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "success";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);  
        }
        else{
          P_R_I_N_T("INVALID DATA PUBLISH INTERVAL");
          char buffer[256];
          doc["did"] = did;
          doc["status"] = "failed";
          size_t n = serializeJson(doc, buffer);
          client.publish(device_response,buffer, n);
          delay(1000);
        }
  }

else{
      //do nothing
  }
  
}//strcomp ends
}//callback function ends


//-----------------------Callback function-------------------------------------//

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  P_R_I_N_T(topic);
  

  
//-------------------------------------Getting CONFIG data---------------------//

  if(strcmp(topic, configTopic) == 0){
    
      StaticJsonDocument<128> doc;
      deserializeJson( doc,payload,length );
      const char* newdid =  doc["did"];
      unsigned long newStime = doc["stime"];
      int rst = doc["reset"];
      int ap = doc["ap"];
      
      P_R_I_N_T(newdid);
      P_R_I_N_T(newStime); 
      P_R_I_N_T(rst);
      P_R_I_N_T(ap);
    

      if(strcmp(did,newdid) == 0){
        
        //RESET cmd check
        if(rst == 1){
        P_R_I_N_T("RESET");

        char buffer[256];
        doc["status"] = "success";
        size_t n = serializeJson(doc, buffer);//stringify
        client.publish(responseTopic,buffer, n);
        delay(250);
        ESP.reset();
      }else{
        P_R_I_N_T("INVALID RESET COMMAND");

        char buffer[256];
        doc["status"] = "failed";
        size_t n = serializeJson(doc, buffer);
        client.publish(responseTopic,buffer, n);
        delay(250);
      }//rst validation done



      //AP command check
      if(ap == 1){
     
        char buffer[256];
        doc["status"] = "success";
        size_t n = serializeJson(doc, buffer);
        client.publish(responseTopic,buffer, n);
        delay(200);

        onDemandAP();
        
      }else{
        P_R_I_N_T("INVALID ON-DEMAND AP COMMAND");

        char buffer[256];
        doc["status"] = "failed";
        size_t n = serializeJson(doc, buffer);
        client.publish(responseTopic,buffer, n);
        delay(100);
      }//AP validation done


      //SNOOZE TIME NULL CHECK
      if(newStime != NULL){
        snoozeTime = newStime*1000;//in ms
        Serial.printf("Snooze time updated by %d seconds\n", newStime);
        writeConfigFile(snoozeTime);
        doc["status"] = "success"; 
        char buffer[128];
        size_t n = serializeJson(doc, buffer);         
        client.publish(responseTopic, buffer, n);    
      }
      else{
        P_R_I_N_T("Invalid time");
        doc["status"] = "failed";   //notice that using the same json doc
        char buffer[128]; 
        size_t n = serializeJson(doc, buffer);          
        client.publish(responseTopic, buffer, n);
      }//stime validation done
      
    }//did MATCHING DONE
    
    else{
          doc["status"] = "failed";   
          char buffer[128];
          size_t n = serializeJson(doc, buffer);
          P_R_I_N_T("did mismatch");          
          client.publish(responseTopic, buffer, n);
      }
  }//config topic validation ends
     
}//Callback ends

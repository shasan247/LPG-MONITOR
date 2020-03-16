bool mountSPIFFS(){
    // Mount the filesystem
  bool result = SPIFFS.begin();
//  Serial.println("SPIFFS opened: " + result);
  return result;
}


bool loadConfigFile() {

  // this opens the config file in read-mode
  File f = SPIFFS.open(CONFIG_FILE, "r");
  
  if (!f) {
    P_R_I_N_T("Configuration file not found");
    return false;
  } else {
    // we could open the file
    size_t size = f.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // Read and store file contents in buf
    f.readBytes(buf.get(), size);
    // Closing file
    f.close();
    // Using dynamic JSON buffer which is not the recommended memory model, but anyway
    // See https://github.com/bblanchon/ArduinoJson/wiki/Memory%20model
    StaticJsonDocument<64> jsondoc;
    deserializeJson(jsondoc, buf.get(), size);//jsonify
    
    if (jsondoc.containsKey("stime")) {
      unsigned long storedSnoozeTime = jsondoc["stime"];
      P_R_I_N_T(storedSnoozeTime);
      if(storedSnoozeTime != NULL){ snoozeTime = storedSnoozeTime;}//Update the SnoozeTime from the storedSnoozeTime in SPIFFs
    }
  }
  P_R_I_N_T("\nConfig file was successfully parsed");
  return true;
}

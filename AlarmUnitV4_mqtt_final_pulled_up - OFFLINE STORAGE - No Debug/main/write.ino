

bool writeConfigFile(const unsigned long stime ) {

  StaticJsonDocument<64> jsondoc;
  
  // JSONify local configuration parameters
  jsondoc["stime"] = stime;//Write the new snooze time sent OTA
  

  // Open file for writing
  File f = SPIFFS.open(CONFIG_FILE, "w");
  if (!f) {
    P_R_I_N_T("Failed to open config file for writing");
    return false;
  }


  // Stringify and write the json string to file and close it
  char buffer[64];
  size_t n = serializeJson(jsondoc, buffer);
  f.print(buffer);
  f.close();

  P_R_I_N_T("\nConfig file was successfully saved");
  return true;
}

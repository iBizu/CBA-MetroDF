void checkNVS() {

  
  // Adiciona um pequeno delay para estabilização
  delay(100);
  
  Preferences prefs;
  bool needsRestart = false;

  // 1. Tenta abrir a NVS em modo leitura
  if (!prefs.begin("config", true)) {
    Serial.println("[ERRO] Falha ao abrir NVS! Tentando recuperar...");
    prefs.end();
    delay(100);
    
    // Tenta reparar a NVS
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) {
      Serial.printf("[ERRO] Falha ao apagar NVS: 0x%x\n", err);
    }
    err = nvs_flash_init();
    if (err != ESP_OK) {
      Serial.printf("[ERRO] Falha ao inicializar NVS: 0x%x\n", err);
      Serial.println("[ERRO] Reiniciando para tentar recuperar...");
      delay(1000);
      ESP.restart();
    }
    
    // Cria nova estrutura
    if (prefs.begin("config", false)) {
      prefs.putInt("checksum", 12345);
      prefs.putString("wifi_ssid", "");
      prefs.putString("wifi_pass", "");
      prefs.end();
      Serial.println("[NVS] Recriada com sucesso!");
      needsRestart = true;
    }
  } 
  else {
    // 2. Verifica o checksum
    if (prefs.getInt("checksum", 0) != 12345) {
      Serial.println("[NVS] Checksum inválido! Formatando...");
      prefs.end();
      
      nvs_flash_erase();
      nvs_flash_init();
      
      if (prefs.begin("config", false)) {
        prefs.putInt("checksum", 12345);
        prefs.putString("wifi_ssid", "");
        prefs.putString("wifi_pass", "");
        prefs.end();
        Serial.println("[NVS] Formatada e reinicializada!");
        needsRestart = true;
      }
    } else {
      Serial.println("[NVS] Verificação OK!");
      prefs.end();
    }
  }

  if (needsRestart) {
    Serial.println("Reiniciando para aplicar mudanças na NVS...");
    delay(1000);
    ESP.restart();
  }
}
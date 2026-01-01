#include <Arduino.h>
#include "SPIFFS.h"
#include "config.h"
#include "rf_handler.h"

void setup() {
  Serial.begin(115200);
  
  Serial.println("Inicializando ESP32-2 (Receptor RF)...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar SPIFFS");
    return;
  }
  
  initRF();
  
  Serial.println("ESP32-2 pronto para receber dados via RF");
}

void loop() {
  receiveDataViaRF();
  delay(10);
}


#include <Arduino.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include "config.h"
#include "rf_handler.h"
#include "gemini_handler.h"

void setup() {
  Serial.begin(115200);
  
  Serial.println("Inicializando ESP32-2 (Receptor RF)...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRO: Falha ao montar SPIFFS");
    return;
  }
  
  initRF();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("ESP32-2 pronto para receber dados via RF");
}

void loop() {
  if (receiveDataViaRF()) {
    // Se recebeu dados com sucesso, enviar para Gemini
    delay(500); // Pequeno delay para garantir que o arquivo foi fechado
    sendAudioToGemini();
  }
  delay(10);
}


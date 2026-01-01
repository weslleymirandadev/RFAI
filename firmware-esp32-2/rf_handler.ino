#include "rf_handler.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "SPIFFS.h"

RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Endereço do pipe (deve ser o mesmo no transmissor e receptor)
const byte address[6] = "00001";

void initRF() {
  Serial.println("Inicializando nRF24L01...");
  
  if (!radio.begin()) {
    Serial.println("ERRO: Falha ao inicializar nRF24L01!");
    return;
  }
  
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.startListening();
  
  Serial.println("nRF24L01 configurado como receptor");
}

void receiveDataViaRF() {
  if (!radio.available()) {
    return;
  }
  
  // Receber tamanho do arquivo primeiro
  uint32_t fileSize = 0;
  uint8_t sizeBuffer[sizeof(uint32_t)];
  
  if (radio.read(sizeBuffer, sizeof(uint32_t))) {
    memcpy(&fileSize, sizeBuffer, sizeof(uint32_t));
    Serial.printf("Recebendo arquivo de %d bytes...\n", fileSize);
    
    // Remover arquivo anterior se existir
    if (SPIFFS.exists("/received.wav")) {
      SPIFFS.remove("/received.wav");
    }
    
    File audioFile = SPIFFS.open("/received.wav", FILE_WRITE);
    if (!audioFile) {
      Serial.println("ERRO: Nao foi possivel criar arquivo para receber dados");
      return;
    }
    
    uint8_t buffer[32];
    size_t totalReceived = 0;
    unsigned long lastReceiveTime = millis();
    const unsigned long timeout = 5000; // 5 segundos de timeout
    
    while (totalReceived < fileSize) {
      if (radio.available()) {
        size_t bytesRead = radio.getPayloadSize();
        if (bytesRead > sizeof(buffer)) {
          bytesRead = sizeof(buffer);
        }
        
        if (radio.read(buffer, bytesRead)) {
          size_t bytesToWrite = (totalReceived + bytesRead <= fileSize) ? bytesRead : (fileSize - totalReceived);
          size_t bytesWritten = audioFile.write(buffer, bytesToWrite);
          totalReceived += bytesWritten;
          lastReceiveTime = millis();
          
          if (totalReceived % 1024 == 0 || totalReceived == fileSize) {
            Serial.printf("Recebidos %d/%d bytes...\n", totalReceived, fileSize);
          }
        }
      } else {
        // Timeout se não receber dados por muito tempo
        if (millis() - lastReceiveTime > timeout) {
          Serial.println("ERRO: Timeout ao receber dados");
          break;
        }
        delay(5);
      }
    }
    
    audioFile.close();
    
    if (totalReceived == fileSize) {
      Serial.printf("Arquivo recebido com sucesso: %d bytes salvos em /received.wav\n", totalReceived);
    } else {
      Serial.printf("AVISO: Recebido apenas %d de %d bytes\n", totalReceived, fileSize);
    }
  }
}


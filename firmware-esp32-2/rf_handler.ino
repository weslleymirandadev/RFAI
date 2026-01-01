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
  
  // Inicializar SPI com pinos customizados
  SPI.begin(RF_SCK_PIN, RF_MISO_PIN, RF_MOSI_PIN, RF_CSN_PIN);
  
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
  
  // Receber tamanho da string base64 primeiro
  uint32_t base64Length = 0;
  uint8_t sizeBuffer[sizeof(uint32_t)];
  
  if (radio.read(sizeBuffer, sizeof(uint32_t))) {
    memcpy(&base64Length, sizeBuffer, sizeof(uint32_t));
    Serial.printf("Recebendo string base64 de %d bytes...\n", base64Length);
    
    // Remover arquivo anterior se existir
    if (SPIFFS.exists("/audio_base64.txt")) {
      SPIFFS.remove("/audio_base64.txt");
    }
    
    File base64File = SPIFFS.open("/audio_base64.txt", FILE_WRITE);
    if (!base64File) {
      Serial.println("ERRO: Nao foi possivel criar arquivo para receber dados base64");
      return;
    }
    
    uint8_t buffer[32];
    size_t totalReceived = 0;
    unsigned long lastReceiveTime = millis();
    const unsigned long timeout = 10000; // 10 segundos de timeout (base64 pode ser maior)
    
    while (totalReceived < base64Length) {
      if (radio.available()) {
        size_t bytesRead = radio.getPayloadSize();
        if (bytesRead > sizeof(buffer)) {
          bytesRead = sizeof(buffer);
        }
        
        if (radio.read(buffer, bytesRead)) {
          // Calcular quantos bytes escrever (não exceder o tamanho total)
          size_t remaining = base64Length - totalReceived;
          size_t bytesToWrite = (bytesRead < remaining) ? bytesRead : remaining;
          
          size_t bytesWritten = base64File.write(buffer, bytesToWrite);
          totalReceived += bytesWritten;
          lastReceiveTime = millis();
          
          if (totalReceived % 512 == 0 || totalReceived == base64Length) {
            Serial.printf("Recebidos %d/%d bytes de base64...\n", totalReceived, base64Length);
          }
        }
      } else {
        // Timeout se não receber dados por muito tempo
        if (millis() - lastReceiveTime > timeout) {
          Serial.println("ERRO: Timeout ao receber dados base64");
          break;
        }
        delay(5);
      }
    }
    
    base64File.close();
    
    if (totalReceived == base64Length) {
      Serial.printf("Base64 recebido com sucesso: %d bytes salvos em /audio_base64.txt\n", totalReceived);
      Serial.println("Arquivo pronto para ser enviado ao Gemini API");
      return true; // Indica que recebeu com sucesso
    } else {
      Serial.printf("AVISO: Recebido apenas %d de %d bytes de base64\n", totalReceived, base64Length);
    }
  }
  
  return false; // Não recebeu dados ou recebeu incompleto
}


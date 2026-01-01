#include "rf_handler.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "SPIFFS.h"
#include "recorder.h"

RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Endereço do pipe (deve ser o mesmo no transmissor e receptor)
const byte address[6] = "00001";

void initRF() {
  Serial.println("Inicializando nRF24L01...");
  
  if (!radio.begin()) {
    Serial.println("ERRO: Falha ao inicializar nRF24L01!");
    return;
  }
  
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.stopListening();
  
  Serial.println("nRF24L01 configurado como transmissor");
}

void sendAudioDataViaRF() {
  if (!SPIFFS.exists("/record.wav")) {
    Serial.println("Nenhum arquivo de audio para enviar");
    return;
  }
  
  File audioFile = SPIFFS.open("/record.wav", FILE_READ);
  if (!audioFile) {
    Serial.println("ERRO: Nao foi possivel abrir arquivo de audio");
    return;
  }
  
  Serial.println("Enviando dados de audio via RF...");
  
  // Enviar tamanho do arquivo primeiro
  uint32_t fileSize = audioFile.size();
  if (!radio.write(&fileSize, sizeof(fileSize))) {
    Serial.println("ERRO: Falha ao enviar tamanho do arquivo");
    audioFile.close();
    return;
  }
  delay(10);
  
  // Enviar dados em chunks
  uint8_t buffer[32]; // nRF24L01 suporta até 32 bytes por pacote
  size_t bytesRead;
  size_t totalSent = 0;
  
  while (audioFile.available()) {
    bytesRead = audioFile.readBytes((char*)buffer, sizeof(buffer));
    
    if (!radio.write(buffer, bytesRead)) {
      Serial.println("ERRO: Falha ao enviar dados");
      break;
    }
    
    totalSent += bytesRead;
    delay(5); // Pequeno delay entre pacotes
    
    if (totalSent % 1024 == 0) {
      Serial.printf("Enviados %d bytes...\n", totalSent);
    }
  }
  
  audioFile.close();
  Serial.printf("Envio concluido: %d bytes enviados\n", totalSent);
}

bool sendData(uint8_t* data, size_t length) {
  return radio.write(data, length);
}


#include "rf_handler.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "SPIFFS.h"
#include "recorder.h"
#include <Arduino.h>

RF24 radio(RF_CE_PIN, RF_CSN_PIN);

// Endereço do pipe (deve ser o mesmo no transmissor e receptor)
const byte address[6] = "00001";

// Função para codificar base64 (adaptada do web_server.ino)
static const char base64_chars[] = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64_encode(const uint8_t* data, size_t length) {
  String result = "";
  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];

  while (length--) {
    char_array_3[i++] = *(data++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) {
        result += base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) {
      result += base64_chars[char_array_4[j]];
    }

    while ((i++ < 3)) {
      result += '=';
    }
  }

  return result;
}

void initRF() {
  Serial.println("Inicializando nRF24L01...");
  
  // Inicializar SPI com pinos customizados
  SPI.begin(RF_SCK_PIN, RF_MISO_PIN, RF_MOSI_PIN, RF_CSN_PIN);
  
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
  
  Serial.println("Convertendo audio para base64 e enviando via RF...");
  
  // Ler todo o arquivo e converter para base64
  size_t fileSize = audioFile.size();
  uint8_t* fileBuffer = (uint8_t*)malloc(fileSize);
  if (!fileBuffer) {
    Serial.println("ERRO: Falha ao alocar memoria para o arquivo");
    audioFile.close();
    return;
  }
  
  size_t bytesRead = audioFile.readBytes((char*)fileBuffer, fileSize);
  audioFile.close();
  
  if (bytesRead != fileSize) {
    Serial.println("ERRO: Falha ao ler arquivo completo");
    free(fileBuffer);
    return;
  }
  
  // Converter para base64
  String base64Data = base64_encode(fileBuffer, fileSize);
  free(fileBuffer);
  
  uint32_t base64Length = base64Data.length();
  Serial.printf("Arquivo convertido para base64: %d bytes (original: %d bytes)\n", base64Length, fileSize);
  
  // Enviar tamanho da string base64 primeiro
  if (!radio.write(&base64Length, sizeof(uint32_t))) {
    Serial.println("ERRO: Falha ao enviar tamanho do base64");
    return;
  }
  delay(10);
  
  // Enviar dados base64 em chunks de 32 bytes (limite do nRF24L01)
  const size_t chunkSize = 32;
  size_t totalSent = 0;
  const char* base64Str = base64Data.c_str();
  
  for (size_t i = 0; i < base64Length; i += chunkSize) {
    size_t remaining = base64Length - i;
    size_t currentChunk = (remaining < chunkSize) ? remaining : chunkSize;
    
    uint8_t chunk[chunkSize];
    memcpy(chunk, base64Str + i, currentChunk);
    
    if (!radio.write(chunk, currentChunk)) {
      Serial.println("ERRO: Falha ao enviar chunk de dados");
      break;
    }
    
    totalSent += currentChunk;
    delay(5); // Pequeno delay entre pacotes
    
    if (totalSent % 256 == 0 || totalSent == base64Length) {
      Serial.printf("Enviados %d/%d bytes de base64...\n", totalSent, base64Length);
    }
  }
  
  Serial.printf("Envio concluido: %d bytes de base64 enviados\n", totalSent);
}

bool sendData(uint8_t* data, size_t length) {
  return radio.write(data, length);
}


#include "recorder.h"

bool recording = false;
unsigned long lastVoiceTime = 0;
uint32_t dataBytesWritten = 0;
File audioFile;
bool vadMode = false;

void startRecording() {
  if (recording) {
    return;
  }

  Serial.println("Voz detectada!");

  if (audioFile) {
    audioFile.close();
    delay(10);
  }

  if (SPIFFS.exists("/record.wav")) {
    SPIFFS.remove("/record.wav");
    delay(10);
  }

  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;
  
  Serial.printf("SPIFFS: %d KB livres de %d KB total\n", freeBytes / 1024, totalBytes / 1024);
  
  if (freeBytes < 50000) {
    Serial.println("AVISO: Pouco espaco no SPIFFS!");
  }

  audioFile = SPIFFS.open("/record.wav", FILE_WRITE);
  if (!audioFile) {
    Serial.println("ERRO: Erro ao abrir arquivo! Verifique espaco no SPIFFS.");
    recording = false;
    return;
  }

  recording = true;
  dataBytesWritten = 0;
  audioFile.seek(44);
  lastVoiceTime = millis();
  Serial.println("Gravacao iniciada");
}

void stopRecording() {
  if (!recording || !audioFile) {
    return;
  }

  Serial.println("Fim da gravacao");

  audioFile.flush();
  writeWavHeader(audioFile, dataBytesWritten);

  Serial.printf("Arquivo salvo: %d bytes de dados de audio\n", dataBytesWritten);
  Serial.printf("Duracao aproximada: %.2f segundos\n", (float)dataBytesWritten / (SAMPLE_RATE * 2));

  audioFile.close();
  recording = false;
  delay(50);
  Serial.println("Gravacao finalizada e arquivo fechado");
}

bool isRecording() {
  return recording;
}

void updateLastVoiceTime() {
  lastVoiceTime = millis();
}

void checkSilenceTimeout(uint32_t rms) {
  if (recording) {
    if (!audioFile) {
      Serial.println("AVISO: Arquivo foi fechado inesperadamente!");
      recording = false;
    } else {
      if (rms <= VAD_THRESHOLD) {
        if (millis() - lastVoiceTime > SILENCE_TIMEOUT_MS) {
          stopRecording();
        }
      }
    }
  }
}

uint32_t getDataBytesWritten() {
  return dataBytesWritten;
}

void writeAudioData(int16_t* buffer, int samples) {
  if (!recording || !audioFile) {
    return;
  }

  size_t bytesToWrite = samples * sizeof(int16_t);
  size_t bytesWritten = audioFile.write((uint8_t*)buffer, bytesToWrite);
  
  if (bytesWritten != bytesToWrite) {
    Serial.println("ERRO: Erro ao escrever no arquivo!");
  } else {
    dataBytesWritten += bytesWritten;
  }
}

void setVADMode(bool enabled) {
  vadMode = enabled;
  Serial.printf("Modo alterado para: %s\n", enabled ? "VAD (detecção de voz)" : "Botão");
}

bool isVADMode() {
  return vadMode;
}


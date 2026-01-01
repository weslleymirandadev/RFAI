#include <Arduino.h>
#include "SPIFFS.h"
#include "driver/i2s.h"
#include <WiFi.h>
#include <WebServer.h>

#include "config.h"
#include "wav_handler.h"
#include "audio_processor.h"
#include "i2s_handler.h"
#include "recorder.h"
#include "web_server.h"

WebServer server(80);

int32_t rawI2sBuffer[BUFFER_SAMPLES];
int16_t processedBuffer[BUFFER_SAMPLES];

unsigned long lastDebugTime = 0;
void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  initAudioProcessor();
  SPIFFS.begin(true);
  initI2S();
  delay(100);
  calibrateDCOffset();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  Serial.println(WiFi.localIP());
  Serial.println("Sistema de gravacao VAD inicializado");
  Serial.printf("Threshold: %d\n", VAD_THRESHOLD);
  Serial.printf("Audio Gain: %dx\n", AUDIO_GAIN);
  Serial.printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("Conversao: %s\n", USE_MSB_CONVERSION ? "MSB (bits mais significativos)" : "LSB (bits menos significativos)");
  if (DEBUG_VAD) {
    Serial.println("Debug VAD ativado - valores RMS serao exibidos");
  }
  Serial.println("Dica: Se audio estiver estourado, diminua AUDIO_GAIN. Se estiver baixo, aumente.");

  initWebServer(&server);
}

void loop() {
  size_t bytesRead;
  
  i2s_read(I2S_NUM_0, rawI2sBuffer, sizeof(int32_t) * BUFFER_SAMPLES, &bytesRead, portMAX_DELAY);
  int samplesRead = bytesRead / sizeof(int32_t);
  
  if (samplesRead == 0) {
    server.handleClient();
    return;
  }

  convert32to16(rawI2sBuffer, processedBuffer, samplesRead);
  uint32_t currentRMS = computeRMS(processedBuffer, samplesRead);
  uint32_t rms = computeSmoothedRMS(currentRMS);

  bool buttonPressed = !digitalRead(BUTTON_PIN);
  
  if (isVADMode()) {
    if (DEBUG_VAD && millis() - lastDebugTime > 500) {
      int16_t minVal = 32767, maxVal = -32768;
      for (int i = 0; i < samplesRead && i < 10; i++) {
        if (processedBuffer[i] < minVal) minVal = processedBuffer[i];
        if (processedBuffer[i] > maxVal) maxVal = processedBuffer[i];
      }
      
      Serial.printf("RMS: %d (threshold: %d) DC_offset: %d Range: [%d, %d] %s\n", 
                    rms, VAD_THRESHOLD, getDCOffset(), minVal, maxVal,
                    rms > VAD_THRESHOLD ? "VOZ!" : "");
      lastDebugTime = millis();
    }

    if (rms > VAD_THRESHOLD) {
      if (!isRecording()) {
        digitalWrite(2, HIGH);
        startRecording();
      }
      updateLastVoiceTime();
    }

    if (isRecording()) {
      writeAudioData(processedBuffer, samplesRead);
    }

    checkSilenceTimeout(rms);
  } else {
    if (buttonPressed) {
      if (!isRecording()) {
        digitalWrite(2, HIGH);
        startRecording();
      }
      if (isRecording()) {
        writeAudioData(processedBuffer, samplesRead);
      }
    } else {
      if (isRecording()) {
        stopRecording();
        digitalWrite(2, LOW);
      }
    }
  }
  
  if (!isRecording()) {
    digitalWrite(2, LOW);
  }

  server.handleClient();
}

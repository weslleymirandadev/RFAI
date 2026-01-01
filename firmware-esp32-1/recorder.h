#ifndef RECORDER_H
#define RECORDER_H

#include <Arduino.h>
#include "SPIFFS.h"
#include "config.h"
#include "wav_handler.h"

void startRecording();
void stopRecording();
bool isRecording();
void updateLastVoiceTime();
void checkSilenceTimeout(uint32_t rms);
uint32_t getDataBytesWritten();
void writeAudioData(int16_t* buffer, int samples);
void setVADMode(bool enabled);
bool isVADMode();

#endif


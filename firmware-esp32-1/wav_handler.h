#ifndef WAV_HANDLER_H
#define WAV_HANDLER_H

#include <Arduino.h>
#include "SPIFFS.h"
#include "config.h"

void writeUint16(File file, uint16_t value);
void writeUint32(File file, uint32_t value);
void writeWavHeader(File file, uint32_t dataSize);

#endif


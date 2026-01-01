#include "wav_handler.h"

void writeUint16(File file, uint16_t value) {
  uint8_t bytes[2];
  bytes[0] = value & 0xFF;
  bytes[1] = (value >> 8) & 0xFF;
  file.write(bytes, 2);
}

void writeUint32(File file, uint32_t value) {
  uint8_t bytes[4];
  bytes[0] = value & 0xFF;
  bytes[1] = (value >> 8) & 0xFF;
  bytes[2] = (value >> 16) & 0xFF;
  bytes[3] = (value >> 24) & 0xFF;
  file.write(bytes, 4);
}

void writeWavHeader(File file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;
  file.seek(0);

  file.write((const uint8_t*)"RIFF", 4);
  writeUint32(file, fileSize);
  file.write((const uint8_t*)"WAVE", 4);
  file.write((const uint8_t*)"fmt ", 4);

  writeUint32(file, 16);
  writeUint16(file, 1);
  writeUint16(file, 1);
  writeUint32(file, SAMPLE_RATE);
  writeUint32(file, SAMPLE_RATE * 2);
  writeUint16(file, 2);
  writeUint16(file, 16);

  file.write((const uint8_t*)"data", 4);
  writeUint32(file, dataSize);
}


#ifndef RF_HANDLER_H
#define RF_HANDLER_H

#include <Arduino.h>
#include "config.h"

void initRF();
void sendAudioDataViaRF();
bool sendData(uint8_t* data, size_t length);

#endif


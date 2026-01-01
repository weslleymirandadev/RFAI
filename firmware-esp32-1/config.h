#ifndef CONFIG_H
#define CONFIG_H

#define I2S_BCLK 18
#define I2S_WS   19
#define I2S_SD   23

#define BUTTON_PIN 21

// nRF24L01 pins
#define RF_CE_PIN 34
#define RF_CSN_PIN 35
#define RF_SCK_PIN 32
#define RF_MOSI_PIN 33
#define RF_MISO_PIN 25

#define SAMPLE_RATE 16000
#define CHANNELS 1
#define BUFFER_SAMPLES 512
#define SILENCE_TIMEOUT_MS 1500

#define VAD_THRESHOLD 500
#define DEBUG_VAD false
#define AUDIO_GAIN 75
#define USE_MSB_CONVERSION true

#define RMS_SMOOTHING_SAMPLES 4

#define DC_FILTER_ALPHA 99

#endif

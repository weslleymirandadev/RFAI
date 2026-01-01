#include "web_server.h"
#include "SPIFFS.h"
#include "recorder.h"
#include <pgmspace.h>

static const char base64_chars[] = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static String base64_encode_impl(const uint8_t* data, size_t length) {
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


const char html_page[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>ESP32 Audio Recorder</title>
  <style>
    body {
      font-family: Arial;
      text-align: center;
      padding: 20px;
    }
    audio {
      width: 100%;
      max-width: 600px;
      margin: 20px 0;
    }
    button {
      padding: 10px 20px;
      font-size: 16px;
      margin: 10px;
    }
  </style>
</head>
<body>
  <h2>ESP32 Audio Recorder</h2>
  <p>Audio gravado (VAD):</p>
  <audio controls src='/audio' preload='auto'></audio>
  <br>
  <button onclick='location.reload()'>Atualizar</button>
  <p><small>Gravação automática por Voice Activity Detection</small></p>
</body>
</html>
)";

void handleRoot(WebServer* server) {
  server->setContentLength(strlen_P(html_page));
  server->send(200, "text/html", "");
  
  char buffer[256];
  size_t len = strlen_P(html_page);
  size_t pos = 0;
  
  while (pos < len) {
    size_t chunk = (len - pos < 256) ? (len - pos) : 256;
    memcpy_P(buffer, html_page + pos, chunk);
    server->sendContent(buffer, chunk);
    pos += chunk;
  }
}

void handleAudio(WebServer* server) {
  File f = SPIFFS.open("/record.wav", FILE_READ);
  if (!f) {
    server->send(404, "text/plain", "Audio file not found");
    return;
  }
  
  if (f.size() <= 44) {
    server->send(204, "text/plain", "Audio file too small or empty");
    f.close();
    return;
  }
  
  server->sendHeader("Content-Type", "audio/wav");
  server->sendHeader("Cache-Control", "no-cache");
  server->streamFile(f, "audio/wav");
  f.close();
}

void handleMode(WebServer* server) {
  if (server->hasArg("mode")) {
    String mode = server->arg("mode");
    if (mode == "vad") {
      setVADMode(true);
      server->send(200, "text/plain", "Modo VAD ativado");
    } else if (mode == "button") {
      setVADMode(false);
      server->send(200, "text/plain", "Modo Botao ativado");
    } else {
      server->send(400, "text/plain", "Modo invalido. Use 'vad' ou 'button'");
    }
  } else {
    String response = "Modo atual: ";
    response += isVADMode() ? "VAD" : "Botao";
    server->send(200, "text/plain", response);
  }
}

void initWebServer(WebServer* server) {
  server->on("/", [server]() { handleRoot(server); });
  server->on("/audio", [server]() { handleAudio(server); });
  server->on("/mode", [server]() { handleMode(server); });
  server->begin();
}


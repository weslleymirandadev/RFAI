#include "gemini_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

String readBase64FromFile() {
  if (!SPIFFS.exists("/audio_base64.txt")) {
    Serial.println("ERRO: Arquivo base64 nao encontrado");
    return "";
  }
  
  File file = SPIFFS.open("/audio_base64.txt", FILE_READ);
  if (!file) {
    Serial.println("ERRO: Nao foi possivel abrir arquivo base64");
    return "";
  }
  
  String data = "";
  while (file.available()) {
    data += (char)file.read();
  }
  file.close();
  return data;
}

String extractTextFromJson(const String& jsonResponse) {
  DynamicJsonDocument doc(8192);
  if (deserializeJson(doc, jsonResponse)) {
    return "";
  }
  
  JsonArray candidates = doc["candidates"];
  if (candidates.size() == 0) {
    return "";
  }
  
  JsonArray parts = candidates[0]["content"]["parts"];
  if (parts.size() == 0 || !parts[0].containsKey("text")) {
    return "";
  }
  
  return parts[0]["text"].as<String>();
}

bool sendAudioToGemini() {
  String base64Data = readBase64FromFile();
  if (base64Data.length() == 0) {
    Serial.println("ERRO: Nenhum dado base64 encontrado");
    return false;
  }
  
  Serial.printf("Base64 lido: %d caracteres\n", base64Data.length());
  
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=";
  url += google_api_key;
  
  DynamicJsonDocument payload(base64Data.length() + 500);
  payload["contents"][0]["parts"][0]["inline_data"]["mime_type"] = "audio/wav";
  payload["contents"][0]["parts"][0]["inline_data"]["data"] = base64Data;
  
  String jsonPayload;
  serializeJson(payload, jsonPayload);
  
  Serial.println("Enviando requisicao para Gemini API...");
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int code = http.POST(jsonPayload);
  if (code <= 0) {
    Serial.printf("ERRO ao enviar: %s\n", http.errorToString(code).c_str());
    http.end();
    return false;
  }
  
  Serial.printf("Resposta HTTP: %d\n", code);
  String response = http.getString();
  http.end();
  
  if (code != 200) {
    Serial.printf("ERRO HTTP %d: %s\n", code, response.c_str());
    return false;
  }
  
  String text = extractTextFromJson(response);
  if (text.length() == 0) {
    Serial.println("ERRO: Formato de resposta inesperado");
    Serial.println(response);
    return false;
  }
  
  Serial.println("\n=== RESPOSTA DO GEMINI ===");
  Serial.println(text);
  Serial.println("==========================\n");
  return true;
}

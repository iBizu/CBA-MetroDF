// baixa update do github no horário programado

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include "time.h"

// Substitua pelo link direto do Github
const char* url_firmware_ota = "https://github.com/iBizu/CBA-MetroDF/releases/download/V1.0.0/CBA21_foca.ino.bin";

// Servidor NTP e Fuso Horário de Brasília (UTC-3)
const char* ntpServer = "a.st1.ntp.br"; // Servidor NTP oficial do Brasil
const long gmtOffset_sec = -3 * 3600;   // UTC-3 em segundos
const int daylightOffset_sec = 0;

// Flag para evitar que tente atualizar mais de uma vez no mesmo dia/minuto
bool atualizacaoExecutadaHoje = false;

// Configuração inicial do horário NTP
void inicializarNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP inicializado (Fuso Brasília UTC-3).");
}

// Função principal de execução da atualização
void executarAtualizacaoOTA() {
  Serial.println("\n[OTA Metrô-DF] A iniciar verificação e download de novo firmware...");
  
  WiFiClientSecure client;
  client.setInsecure(); // Ignora a verificação de certificado durante os testes

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 

  t_httpUpdate_return ret = httpUpdate.update(client, url_firmware_ota);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA Erro] Falha no upload (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
      
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA Info] Nenhuma atualização pendente ou ficheiro inacessível.");
      break;
      
    case HTTP_UPDATE_OK:
      Serial.println("[OTA Sucesso] Atualização concluída! A reiniciar o CBA...");
      break;
  }
}

// Função chamada no loop para verificar o relógio
void verificarHorarioOTA() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Se ainda não conseguiu obter a hora via NTP, pula a verificação
    return;
  }

  // Verifica se é 03:00 da manhã (Hora: 3, Minuto: 0)
  if (timeinfo.tm_hour == 16 && timeinfo.tm_min == 27) {
    if (!atualizacaoExecutadaHoje) {
      atualizacaoExecutadaHoje = true; // Marca como executado para não repetir no mesmo minuto
      executarAtualizacaoOTA();
    }
  } else {
    // Reseta a flag fora do horário de 03:00 para permitir a atualização no dia seguinte
    if (atualizacaoExecutadaHoje) {
      atualizacaoExecutadaHoje = false;
    }
  }
}
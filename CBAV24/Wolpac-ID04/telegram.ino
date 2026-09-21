// Envio de mensagens para o grupo do Telegram (API do bot, POST em JSON).
// Cada envio bloqueia por alguns segundos (handshake TLS): nada é contado enquanto isso.
// Por isso as mensagens geradas durante a contagem são só AGENDADAS (agendarTelegram) e
// enviadas pelo loop ocioso (enviarTelegramPendente), com limite de uma tentativa por minuto.
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

String telegramPendente = "";              // mensagem(ns) aguardando envio
unsigned long ultimaTentativaTelegram = 0; // millis() da última tentativa de enviar a pendente
const unsigned long TELEGRAM_INTERVALO_MS = 60000;   // intervalo mínimo entre tentativas da pendente
const size_t TELEGRAM_MAX_PENDENTE = 1500;           // limite da pendente (Telegram aceita até 4096)

// escapa o texto para ir dentro de uma string JSON
String escaparJson(String s)
{
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\r", "");
  s.replace("\n", "\\n");
  return s;
}

// prefixo comum de todas as mensagens: identifica a placa
String cabecalhoTelegram()
{
  return String(nomeota) + " (id " + String(numid) + ")";
}

// Envia imediatamente. Retorna true se o Telegram respondeu 200.
bool enviarTelegram(const String& texto)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[Telegram] sem Wi-Fi, nao enviado: " + texto);
    return false;
  }
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  WiFiClientSecure client;
  client.setInsecure();            // sem validação de certificado (ver observação no ota_update.ino)
  client.setHandshakeTimeout(10);  // segundos. O default do core é 120 s = tempo do watchdog
  client.setTimeout(8000);         // ms

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  String url = String("https://api.telegram.org/bot") + TG_TOKEN + "/sendMessage";
  if (!http.begin(client, url))
  {
    Serial.println("[Telegram] http.begin falhou");
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  String corpo = "{\"chat_id\":\"" + String(TG_CHAT) + "\",\"text\":\"" + escaparJson(texto) + "\"}";
  int codigo = http.POST(corpo);
  http.end();
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  if (codigo == 200)
  {
    Serial.println("[Telegram] enviado: " + texto);
    return true;
  }
  Serial.printf("[Telegram] falha, HTTP %d\n", codigo);
  return false;
}

// Agenda uma mensagem para ser enviada no loop ocioso (não bloqueia agora).
void agendarTelegram(const String& texto)
{
  if (telegramPendente.length() + texto.length() + 1 > TELEGRAM_MAX_PENDENTE)
  {
    Serial.println("[Telegram] pendente cheia, descartado: " + texto);
    return;
  }
  if (telegramPendente.length() > 0)
  {
    telegramPendente += "\n";
  }
  telegramPendente += texto;
}

// Chamada no loop ocioso: tenta enviar a pendente, no máximo uma vez por minuto.
void enviarTelegramPendente()
{
  if (telegramPendente.length() == 0) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (ultimaTentativaTelegram != 0 && millis() - ultimaTentativaTelegram < TELEGRAM_INTERVALO_MS) return;
  ultimaTentativaTelegram = millis();
  if (enviarTelegram(cabecalhoTelegram() + "\n" + telegramPendente))
  {
    telegramPendente = "";
  }
}

// motivo do último reset, para a mensagem de boot
String motivoReset()
{
  switch (esp_reset_reason())
  {
    case ESP_RST_POWERON:   return "energia ligada";
    case ESP_RST_SW:        return "reinicio por software (ESP.restart)";
    case ESP_RST_PANIC:     return "PANIC (excecao no firmware)";
    case ESP_RST_INT_WDT:   return "watchdog de interrupcao";
    case ESP_RST_TASK_WDT:  return "watchdog de tarefa (travou mais de 120 s)";
    case ESP_RST_WDT:       return "outro watchdog";
    case ESP_RST_BROWNOUT:  return "brown-out (queda de tensao)";
    case ESP_RST_DEEPSLEEP: return "saida de deep sleep";
    case ESP_RST_EXT:       return "reset externo";
    default:                return "desconhecido";
  }
}

// Mensagem enviada ao fim do setup(), quando já se sabe hora, SD e estado do OTA.
// Sem Wi-Fi no boot, fica agendada e sai pelo loop ocioso quando a rede voltar.
void enviarMensagemBoot()
{
  // contagens perdidas (servidor e SD falharam juntos) guardadas na flash pelo loop()
  prefsCBA.begin("my-app", true);
  unsigned int perdidasFlash = prefsCBA.getUInt("counterF", 0);
  prefsCBA.end();

  String m;
  if (primeiroBootPosOTA || (versaoAnterior.length() > 0 && versaoAnterior != FW_VERSION))
  {
    m += "ATUALIZADO: " + (versaoAnterior.length() > 0 ? versaoAnterior : String("?")) + " -> " + FW_VERSION;
  }
  else
  {
    m += "Reiniciou - motivo: " + motivoReset();
  }
  if (houveRollback)
  {
    m += "\nROLLBACK: a versao " + versaoRejeitada + " travou no boot e foi rejeitada. Rodando " + FW_VERSION + ". Publique uma versao nova.";
  }
  if (binarioVersaoErrada)
  {
    m += "\nATENCAO: baixei a versao " + versaoRejeitada + ", mas o .bin publicado foi compilado com FW_VERSION " FW_VERSION ". Nao vou baixar a " + versaoRejeitada + " de novo. Corrija FW_VERSION no codigo, exporte e publique uma versao MAIOR.";
  }
  m += "\nVersao: " FW_VERSION;
#if AMBIENTE_LAB
  m += " | ambiente: LABORATORIO";
#else
  m += " | ambiente: ESTACAO";
#endif
  if (otaativ == 1)
  {
    m += " | JUMPER OTA (rede de programacao)";
  }
  m += "\nModelo: " + String(modelo) + " | tipoEntr: " + String(tipoEntr);
  m += "\nWi-Fi: " + WiFi.SSID() + " " + WiFi.localIP().toString() + " (" + String(WiFi.RSSI()) + " dBm)";
  m += "\nHora da placa: " + rtc.getTime("%d/%m/%y %T") + (relogioValido() ? "" : " (RELOGIO INVALIDO)");
  m += "\nSD: " + String(flagF == 0 ? "ok" : "FALHOU") + " | pendentes no SD: " + String(flagSD == 1 ? "sim" : "nao") + " | perdidas na flash: " + String(perdidasFlash);

  if (WiFi.status() == WL_CONNECTED)
  {
    enviarTelegram(cabecalhoTelegram() + "\n" + m);
  }
  else
  {
    agendarTelegram(m);   // enviarTelegramPendente() prefixa o cabeçalho
  }
}

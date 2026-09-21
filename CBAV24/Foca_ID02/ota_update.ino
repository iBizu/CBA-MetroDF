// Atualização remota (OTA) pelo GitHub, fuso horário e proteção de rollback.
//
// Fluxo: lê versao.txt da release "latest"; se a versão for MAIOR que FW_VERSION (e não for uma
// versão já rejeitada por rollback) baixa OTA_ASSET e grava no slot inativo; a placa reinicia
// sozinha ao terminar. No boot seguinte, verificarEstadoOTA() confirma a imagem nova; se o
// firmware novo travar antes disso, o bootloader volta para o anterior (rollback) e a versão
// fica marcada como rejeitada para não ser baixada de novo.
//
// Segurança: client.setInsecure() não valida o certificado do GitHub. Para produção, ou pinar as
// raízes (USERTrust ECC para github.com e ISRG Root X1 para objects.githubusercontent.com) ou
// hospedar versao.txt e o .bin no Web Service interno via HTTP simples.
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <esp_ota_ops.h>

bool otaExecutadoNesteMinuto = false;   // estação: evita repetir dentro do mesmo minuto
unsigned long ultimaVerificacaoOTA = 0;  // bancada: controle do intervalo
String ultimaVersaoNuvemAvisada = "";    // evita repetir no Telegram o mesmo aviso sobre a mesma versão

// O hook verifyRollbackLater() (que adia a confirmação da imagem para verificarEstadoOTA())
// está em rollback_hook.cpp.

// Fuso de Brasília (UTC-3) sem NTP. O relógio é ajustado pelo servidor (recuperaTimestamp e cada
// resposta de enviarContagem). NTP e servidor gravavam no mesmo relógio e podiam divergir; e a
// rede da estação pode não ter saída para servidores NTP.
void configurarFuso()
{
  setenv("TZ", "<-03>3", 1);
  tzset();
}

// relógio posterior a jan/2023 = já foi sincronizado alguma vez (sobrevive a ESP.restart, não a queda de energia)
bool relogioValido()
{
  return rtc.getEpoch() > 1672531200;
}

// Chamada no início do setup(), antes do Wi-Fi.
void verificarEstadoOTA()
{
  const esp_partition_t* rodando = esp_ota_get_running_partition();
  esp_ota_img_states_t estado;
  if (esp_ota_get_state_partition(rodando, &estado) == ESP_OK && estado == ESP_OTA_IMG_PENDING_VERIFY)
  {
    primeiroBootPosOTA = true;
    esp_ota_mark_app_valid_cancel_rollback();   // confirma esta imagem; sem isto o bootloader volta para a anterior no próximo reset
    Serial.println("[OTA] Primeiro boot apos atualizacao: imagem confirmada.");
  }

  prefsCBA.begin("ota", false);
  String versaoTentada = prefsCBA.getString("tentada", "");
  versaoRejeitada = prefsCBA.getString("rejeitada", "");
  versaoAnterior = prefsCBA.getString("versao", "");

  // tentamos gravar "versaoTentada", mas estamos rodando FW_VERSION:
  if (versaoTentada.length() > 0 && versaoTentada != FW_VERSION)
  {
    if (esp_ota_get_last_invalid_partition() != NULL)
    {
      // há uma partição marcada como inválida/abortada: o bootloader fez rollback
      houveRollback = true;
      versaoRejeitada = versaoTentada;
      prefsCBA.putString("rejeitada", versaoRejeitada);
      Serial.println("[OTA] ROLLBACK detectado: versao " + versaoRejeitada + " rejeitada.");
    }
    else if (primeiroBootPosOTA)
    {
      // a imagem nova gravou e está rodando, mas se identifica com outra versão: o .bin publicado
      // na release foi compilado com FW_VERSION diferente do versao.txt. Sem esta trava a placa
      // veria "nuvem > placa" de novo e baixaria o mesmo arquivo a cada verificação, para sempre.
      binarioVersaoErrada = true;
      versaoRejeitada = versaoTentada;
      prefsCBA.putString("rejeitada", versaoRejeitada);
      Serial.println("[OTA] Binario publicado como " + versaoTentada + " foi compilado com FW_VERSION " FW_VERSION ". Versao " + versaoTentada + " bloqueada.");
    }
  }
  if (versaoTentada.length() > 0)
  {
    prefsCBA.remove("tentada");
  }
  if (versaoAnterior != FW_VERSION)
  {
    prefsCBA.putString("versao", FW_VERSION);   // guarda a versão que está rodando, para a mensagem "atualizado de X para Y"
  }
  prefsCBA.end();
}

// compara versões "a.b.c" numericamente: <0 se a<b, 0 se iguais, >0 se a>b
int compararVersao(String a, String b)
{
  while (a.length() > 0 || b.length() > 0)
  {
    int pa = a.indexOf('.');
    int pb = b.indexOf('.');
    int na = (pa < 0 ? a : a.substring(0, pa)).toInt();
    int nb = (pb < 0 ? b : b.substring(0, pb)).toInt();
    if (na != nb) return na - nb;
    a = (pa < 0) ? "" : a.substring(pa + 1);
    b = (pb < 0) ? "" : b.substring(pb + 1);
  }
  return 0;
}

// só dígitos e pontos, tamanho razoável (um HTML de "Not Found" não passa)
bool versaoValida(const String& v)
{
  if (v.length() == 0 || v.length() > 15) return false;
  for (unsigned int i = 0; i < v.length(); i++)
  {
    char c = v.charAt(i);
    if (!isDigit(c) && c != '.') return false;
  }
  return true;
}

// Lê o versao.txt da release "latest". Retorna true se leu uma versão válida.
bool lerVersaoNuvem(String& versaoNuvem)
{
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(10);   // segundos
  client.setTimeout(8000);          // ms

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // o asset da release redireciona (302) para o CDN do GitHub
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  if (!http.begin(client, url_versao_txt))
  {
    Serial.println("[OTA] http.begin falhou para versao.txt");
    return false;
  }
  int codigo = http.GET();
  bool ok = false;
  if (codigo == HTTP_CODE_OK)
  {
    versaoNuvem = http.getString();
    versaoNuvem.trim();
    ok = versaoValida(versaoNuvem);
    if (!ok)
    {
      Serial.println("[OTA] versao.txt com conteudo invalido: " + versaoNuvem.substring(0, 40));
    }
  }
  else
  {
    Serial.printf("[OTA] Falha ao ler versao.txt, HTTP %d\n", codigo);
  }
  http.end();
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  return ok;
}

// Verifica e, se houver versão mais nova, baixa e grava. Se der certo a placa reinicia aqui dentro.
// "origem" só identifica na Serial/Telegram quem chamou ("boot" ou "agendado").
void executarAtualizacaoOTA(const char* origem)
{
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.printf("\n[OTA] Verificacao (%s). Versao da placa: %s\n", origem, FW_VERSION);

  String nuvem;
  if (!lerVersaoNuvem(nuvem))
  {
    return;   // sem internet/GitHub fora: silencioso (o Telegram também não alcançaria)
  }
  Serial.println("[OTA] Versao na nuvem: " + nuvem);

  int cmp = compararVersao(nuvem, FW_VERSION);
  if (cmp == 0)
  {
    Serial.println("[OTA] Placa ja esta atualizada.");
    return;
  }
  if (cmp < 0)
  {
    Serial.println("[OTA] Versao na nuvem e ANTERIOR a da placa, ignorando (sem downgrade).");
    if (ultimaVersaoNuvemAvisada != nuvem)
    {
      ultimaVersaoNuvemAvisada = nuvem;
      enviarTelegram(cabecalhoTelegram() + "\nOTA: a release 'latest' esta na versao " + nuvem + ", anterior a da placa (" FW_VERSION "). Ignorada.");
    }
    return;
  }
  if (nuvem == versaoRejeitada)
  {
    Serial.println("[OTA] Versao " + nuvem + " foi rejeitada por rollback, aguardando versao nova.");
    if (ultimaVersaoNuvemAvisada != nuvem)
    {
      ultimaVersaoNuvemAvisada = nuvem;
      enviarTelegram(cabecalhoTelegram() + "\nOTA: a versao " + nuvem + " ja foi rejeitada por rollback nesta placa. Publique uma versao maior.");
    }
    return;
  }

  // ---- há versão nova: baixar ----
  // Avisos no Telegram ("baixando" e, se falhar, "FALHA") saem uma vez por versão da nuvem; as
  // tentativas seguintes da mesma versão são silenciosas (ex.: .bin faltando na release -> 404 a cada 5 min no laboratório).
  // Sucesso não precisa de aviso aqui: a placa reinicia e a mensagem de boot diz "ATUALIZADO".
  bool avisarTelegram = (ultimaVersaoNuvemAvisada != nuvem);
  if (avisarTelegram)
  {
    enviarTelegram(cabecalhoTelegram() + "\nOTA: baixando versao " + nuvem + " (atual " FW_VERSION ")...");
  }

  prefsCBA.begin("ota", false);
  prefsCBA.putString("tentada", nuvem);   // para verificarEstadoOTA() saber qual versão foi rejeitada, se houver rollback
  prefsCBA.end();

  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(15);   // segundos
  client.setTimeout(20000);         // ms

  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int atual, int total) {
    esp_task_wdt_reset();   // o download pode levar mais que os 120 s do watchdog
  });

  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  t_httpUpdate_return ret = httpUpdate.update(client, url_firmware_ota);
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  // só chega aqui se NÃO atualizou (HTTP_UPDATE_OK reinicia dentro de update())
  String erro;
  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      erro = "falha (" + String(httpUpdate.getLastError()) + "): " + httpUpdate.getLastErrorString();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      erro = "servidor respondeu 'sem atualizacao' (asset " OTA_ASSET " nao encontrado?)";
      break;
    default:
      erro = "retorno inesperado";
      break;
  }
  Serial.println("[OTA Erro] " + erro);
  prefsCBA.begin("ota", false);
  prefsCBA.remove("tentada");   // não foi gravado: pode tentar de novo na próxima verificação
  prefsCBA.end();
  if (avisarTelegram)
  {
    ultimaVersaoNuvemAvisada = nuvem;
    enviarTelegram(cabecalhoTelegram() + "\nOTA: FALHA ao baixar/gravar a versao " + nuvem + " - " + erro + "\n(continuo tentando a cada verificacao; so aviso de novo se a versao na nuvem mudar ou apos reiniciar)");
  }
}

// Chamada no loop ocioso.
void verificarHorarioOTA()
{
#if AMBIENTE_LAB
  if (millis() - ultimaVerificacaoOTA >= (unsigned long)OTA_INTERVALO_MIN * 60000UL)
  {
    ultimaVerificacaoOTA = millis();
    executarAtualizacaoOTA("agendado");
  }
#else
  if (!relogioValido()) return;   // sem hora certa não dá para saber se a estação está fechada
  if (rtc.getHour(true) == OTA_HORA && rtc.getMinute() == OTA_MINUTO)
  {
    if (!otaExecutadoNesteMinuto)
    {
      otaExecutadoNesteMinuto = true;
      executarAtualizacaoOTA("agendado");
    }
  }
  else
  {
    otaExecutadoNesteMinuto = false;
  }
#endif
}

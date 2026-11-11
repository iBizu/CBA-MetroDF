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

// Estado da janela de verificação agendada (só a estação usa; a bancada vai por intervalo).
int  ultimoMinutoOTA = -1;              // minuto em que rodou a última tentativa desta janela
bool otaConcluidoNestaJanela = false;   // a janela de hoje já conseguiu ler o versao.txt
bool janelaOTAAberta = false;           // já houve a primeira tentativa desta janela
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
    if (versaoTentada == FW_VERSION)
    {
      // o download desta versão deu certo: a contagem de tentativas não serve mais para nada
      prefsCBA.remove("tentativas");
      prefsCBA.remove("tentvers");
    }
  }
  // Versão guardada MAIOR do que a que está rodando: a placa andou para trás. Não é o rollback do
  // ESP-IDF — esse marca a partição como INVALID/ABORTED e cai no bloco acima, e aqui não há nada
  // marcado. O que se perde é o ponteiro de boot (otadata): sem ele o bootloader cai no primeiro
  // slot, que pode guardar uma imagem velha. Foi o que o Garen fez em 23/10/2026, indo da 11 para
  // a 10 entre a estação e a bancada — as duas cópias do otadata liam VALID, ota_seq zerado, e a
  // 11 continuava íntegra no outro slot (idêntica, byte a byte, à publicada).
  // Esta checagem não depende da contabilidade do ESP-IDF, que nesse caso não registra nada: é a
  // única que pega. Só avisa, de propósito — a versão nova não tem defeito, e bloqueá-la deixaria
  // a placa presa na imagem velha.
  if (versaoValida(versaoAnterior) && compararVersao(versaoAnterior, FW_VERSION) > 0)
  {
    houveRetrocesso = true;
    retrocessos = prefsCBA.getUInt("retrocessos", 0) + 1;
    prefsCBA.putUInt("retrocessos", retrocessos);
    Serial.println("[OTA] RETROCESSO: rodava a " + versaoAnterior + ", subiu na " FW_VERSION " (ocorrencia " + String(retrocessos) + ").");
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

// Lê o versao.txt do canal (ver OTA_URL_BASE). Retorna true se leu uma versão válida.
//
// Prazos generosos de propósito: o caminho da placa é bem mais pesado que o de um PC — são DOIS
// handshakes TLS em sequência (github.com e, depois do redirecionamento 302, o CDN que serve o
// arquivo), num ESP32 e muitas vezes com sinal fraco. Com 8 s isto dava HTTP -11 (READ_TIMEOUT)
// na bancada. Duas tentativas cobrem a falha ocasional sem prender o loop por muito tempo:
// a catraca não é lida enquanto esta função roda.
bool lerVersaoNuvem(String& versaoNuvem)
{
  const int TENTATIVAS = 2;
  int codigo = 0;

  for (int tentativa = 1; tentativa <= TENTATIVAS; tentativa++)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

    WiFiClientSecure client;
    client.setInsecure();
    client.setHandshakeTimeout(20);   // segundos
    client.setTimeout(15000);         // ms

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // o asset da release redireciona (302) para o CDN do GitHub
    http.setConnectTimeout(10000);
    http.setTimeout(15000);
    if (!http.begin(client, url_versao_txt))
    {
      Serial.println("[OTA] http.begin falhou para versao.txt");
      return false;
    }

    codigo = http.GET();
    if (codigo == HTTP_CODE_OK)
    {
      versaoNuvem = http.getString();
      versaoNuvem.trim();
      http.end();
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
      if (versaoValida(versaoNuvem))
      {
        return true;
      }
      Serial.println("[OTA] versao.txt com conteudo invalido: " + versaoNuvem.substring(0, 40));
      return false;   // o arquivo existe mas está errado: repetir não adianta
    }

    http.end();
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    Serial.println("[OTA] Tentativa " + String(tentativa) + "/" + String(TENTATIVAS)
                 + " falhou ao ler versao.txt. HTTP " + String(codigo) + " (" + descricaoErroHttp(codigo) + ")"
                 + " | heap livre: " + String(ESP.getFreeHeap() / 1024) + " KB | RSSI: " + String(WiFi.RSSI()) + " dBm");
    if (tentativa < TENTATIVAS)
    {
      delay(1000);
    }
  }

  Serial.println("[OTA] Desisti de ler o versao.txt nesta verificacao.");
  Serial.println("[OTA] URL consultada: " + String(url_versao_txt));
  return false;
}

// Verifica e, se houver versão mais nova, baixa e grava. Se der certo a placa reinicia aqui dentro.
// "origem" só identifica na Serial/Telegram quem chamou ("boot" ou "agendado").
// "novaJanela" zera a contagem de tentativas de download: verdadeiro na primeira tentativa da
// verificação agendada, falso na do boot. É o que impede o laço de reinícios (ver "há versão nova").
//
// Devolve TRUE quando a verificação completou, isto é, quando a placa chegou a saber qual é a
// versão da nuvem — inclusive se já estava atualizada ou se o download falhou. Devolve FALSE só
// quando nem isso foi possível (sem Wi-Fi, ou GitHub fora de alcance), que é o único caso em que
// vale repetir dentro da janela. O que governa repetir o DOWNLOAD é o OTA_MAX_TENTATIVAS, não isto.
bool executarAtualizacaoOTA(const char* origem, bool novaJanela)
{
  if (WiFi.status() != WL_CONNECTED) return false;
  Serial.printf("\n[OTA] Verificacao (%s). Versao da placa: %s\n", origem, FW_VERSION);

  String nuvem;
  if (!lerVersaoNuvem(nuvem))
  {
    return false;   // nem soube a versão da nuvem: silencioso agora, contabilizado por dia abaixo
  }
  Serial.println("[OTA] Versao na nuvem: " + nuvem);

  int cmp = compararVersao(nuvem, FW_VERSION);
  if (cmp == 0)
  {
    Serial.println("[OTA] Placa ja esta atualizada.");
    return true;
  }
  if (cmp < 0)
  {
    Serial.println("[OTA] Versao na nuvem e ANTERIOR a da placa, ignorando (sem downgrade).");
    if (ultimaVersaoNuvemAvisada != nuvem)
    {
      ultimaVersaoNuvemAvisada = nuvem;
      enviarTelegram(cabecalhoTelegram() + "\nOTA: a release do canal esta na versao " + nuvem + ", anterior a da placa (" FW_VERSION "). Ignorada.");
    }
    return true;
  }
  if (nuvem == versaoRejeitada)
  {
    Serial.println("[OTA] Versao " + nuvem + " foi rejeitada por rollback, aguardando versao nova.");
    if (ultimaVersaoNuvemAvisada != nuvem)
    {
      ultimaVersaoNuvemAvisada = nuvem;
      enviarTelegram(cabecalhoTelegram() + "\nOTA: a versao " + nuvem + " ja foi rejeitada por rollback nesta placa. Publique uma versao maior.");
    }
    return true;
  }

  // ---- há versão nova ----
  // Trava contra o laço de reinícios: se a placa travar durante o download, o watchdog reinicia,
  // o boot roda esta mesma verificação (OTA_CHECAR_NO_BOOT) e o download recomeça. Foi assim que o
  // Garen passou 23/10/2026 reiniciando a cada ~2,5 min na estação — e o prejuízo não foi o OTA
  // falhar, foi o dia inteiro sem contar ninguém.
  // A contagem vive na NVS e é gravada ANTES do download, senão a tentativa que mata a placa é
  // justamente a que não fica registrada: o verificarEstadoOTA() apaga a "tentada" em todo boot e
  // não sobra rastro de que a placa morreu lá dentro.
  // Chaves próprias ("tentvers"/"tentativas") em vez da "tentada" por esse mesmo motivo — uma
  // contagem que zera a cada reinício não trava laço nenhum.
  prefsCBA.begin("ota", false);
  if (novaJanela || prefsCBA.getString("tentvers", "") != nuvem)
  {
    prefsCBA.putUInt("tentativas", 0);
    prefsCBA.putString("tentvers", nuvem);
  }
  unsigned int tentativas = prefsCBA.getUInt("tentativas", 0);
  prefsCBA.end();

  if (tentativas >= OTA_MAX_TENTATIVAS)
  {
    Serial.println("[OTA] Ja tentei baixar a versao " + nuvem + " " + String(tentativas)
                 + " vezes sem conseguir. Desisto ate a proxima verificacao agendada, para nao ficar reiniciando.");
    if (ultimaVersaoNuvemAvisada != nuvem)
    {
      ultimaVersaoNuvemAvisada = nuvem;
      enviarTelegram(cabecalhoTelegram() + "\nOTA: desisti de baixar a versao " + nuvem + " depois de "
                   + String(tentativas) + " tentativas. Sigo contando na " FW_VERSION " e tento de novo na proxima verificacao agendada."
                   + " Sinal fraco e a causa mais provavel; para resolver agora, grave por USB.");
    }
    return true;
  }

  // Avisos no Telegram ("baixando" e, se falhar, "FALHA") saem uma vez por versão da nuvem; as
  // tentativas seguintes da mesma versão são silenciosas (ex.: .bin faltando na release -> 404 a cada 5 min no laboratório).
  // Sucesso não precisa de aviso aqui: a placa reinicia e a mensagem de boot diz "ATUALIZADO".
  bool avisarTelegram = (ultimaVersaoNuvemAvisada != nuvem);
  if (avisarTelegram)
  {
    enviarTelegram(cabecalhoTelegram() + "\nOTA: baixando versao " + nuvem + " (atual " FW_VERSION ") - tentativa " + String(tentativas + 1) + "/" + String(OTA_MAX_TENTATIVAS) + "...");
  }

  prefsCBA.begin("ota", false);
  prefsCBA.putUInt("tentativas", tentativas + 1);   // contada ANTES de baixar: se travar no download, o próximo boot já sabe
  prefsCBA.putString("tentada", nuvem);   // para verificarEstadoOTA() saber qual versão foi rejeitada, se houver rollback
  prefsCBA.end();

  WiFiClientSecure client;
  client.setInsecure();
  client.setHandshakeTimeout(25);   // segundos (dois handshakes TLS: github.com e o CDN do redirecionamento)
  client.setTimeout(30000);         // ms (o .bin tem ~1,2 MB; sinal fraco deixa o download lento)

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
  // O download falhou, mas a versao da nuvem foi lida: a verificacao COMPLETOU e a janela do dia
  // nao reabre por isso. Repetir o download e papel do OTA_MAX_TENTATIVAS, nao da janela.
  return true;
}

// Chamada no loop ocioso.
#if (OTA_MINUTO + OTA_JANELA_MIN) > 60
  #error "OTA_MINUTO + OTA_JANELA_MIN tem de caber dentro da hora (<= 60)."
#endif

// Dias SEGUIDOS em que a janela agendada terminou sem conseguir ler o versao.txt. Mora na NVS
// porque o que ele mede se conta em dias, e um reinício no meio não pode zerar a conta.
void contabilizarDiaSemVersao()
{
  prefsCBA.begin("ota", false);
  unsigned int dias = prefsCBA.getUInt("semversao", 0) + 1;
  prefsCBA.putUInt("semversao", dias);
  prefsCBA.end();

  Serial.println("[OTA] A janela de hoje terminou sem conseguir ler o versao.txt. Dias seguidos: "
               + String(dias));
  if (dias < OTA_DIAS_SEM_VERSAO) return;

  // Pela FILA, não pelo envio direto: se o GitHub não respondeu, o Telegram provavelmente também
  // não responde agora — mas a fila insiste de minuto em minuto e sai quando a rede deixar.
  agendarTelegram("Nao consigo verificar atualizacao ha " + String(dias) + " dia(s): a janela das "
                + String(OTA_HORA) + "h terminou sem conseguir ler o versao.txt do GitHub. Sigo"
                + " contando normalmente na versao " FW_VERSION ", mas nao recebo versao nova enquanto"
                + " isso durar. Sinal: " + String(WiFi.RSSI()) + " dBm."
                + " Se persistir, grave por USB ou verifique o enlace desta placa.");
}

void zerarDiasSemVersao()
{
  prefsCBA.begin("ota", true);
  unsigned int dias = prefsCBA.getUInt("semversao", 0);
  prefsCBA.end();
  if (dias == 0) return;   // caso normal: não gasta escrita de flash
  prefsCBA.begin("ota", false);
  prefsCBA.remove("semversao");
  prefsCBA.end();
  Serial.println("[OTA] Voltei a ler o versao.txt (estava ha " + String(dias) + " dia(s) sem conseguir).");
}

void verificarHorarioOTA()
{
  if (ambienteLab)
  {
    if (millis() - ultimaVerificacaoOTA >= (unsigned long)OTA_INTERVALO_MIN * 60000UL)
    {
      ultimaVerificacaoOTA = millis();
      executarAtualizacaoOTA("agendado", true);
    }
    return;
  }
  if (!relogioValido()) return;   // sem hora certa não dá para saber se a estação está fechada

  int minuto = rtc.getMinute();
  bool naJanela = (rtc.getHour(true) == OTA_HORA
                   && minuto >= OTA_MINUTO
                   && minuto < OTA_MINUTO + OTA_JANELA_MIN);

  if (!naJanela)
  {
    // Janela fechou. Se ela passou inteira sem a placa saber a versão da nuvem, conta o dia.
    if (janelaOTAAberta && !otaConcluidoNestaJanela)
    {
      contabilizarDiaSemVersao();
    }
    janelaOTAAberta = false;
    otaConcluidoNestaJanela = false;
    ultimoMinutoOTA = -1;
    return;
  }

  if (otaConcluidoNestaJanela) return;     // a janela de hoje já cumpriu o seu papel
  if (minuto == ultimoMinutoOTA) return;   // no máximo uma tentativa por minuto
  ultimoMinutoOTA = minuto;

  // "novaJanela" zera a contagem de downloads e vale só na PRIMEIRA tentativa do dia: se fosse a
  // cada minuto, o OTA_MAX_TENTATIVAS ganharia dez janelas por dia e deixaria de travar coisa
  // alguma, que é justamente o laço de reinícios que a versão 14 foi feita para conter.
  bool primeiraDaJanela = !janelaOTAAberta;
  janelaOTAAberta = true;
  if (executarAtualizacaoOTA("agendado", primeiraDaJanela))
  {
    otaConcluidoNestaJanela = true;
    zerarDiasSemVersao();
  }
}

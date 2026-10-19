// Sincronização do relógio com o servidor (GET /recuperaTimestamp).

// Timestamp da passagem em milissegundos (13 digitos), que e o formato esperado pelo Web Service.
// Le o relogio UMA vez: getEpoch() e getMillis() sao duas leituras separadas e, se o segundo virasse
// entre elas, o valor sairia 1 segundo errado.
// Os 3 digitos a mais sao tempo real decorrido, mas o instante em que o relogio foi acertado pelo
// servidor vem so com precisao de segundo, entao a fracao nao e sincronizada com a do servidor.
// Ainda assim resolve um problema real: o servidor deduplica por id + data, e com segundos duas
// passagens no mesmo segundo colapsavam num registro so.
String timestampMs()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t ms = (int64_t)tv.tv_sec * 1000LL + (tv.tv_usec / 1000);
  return String(ms);
}

// Converte o epoch recebido do servidor para SEGUNDOS, aceitando 10 digitos (segundos) ou
// 13 (milissegundos). Devolve 0 se o valor nao for plausivel.
// O relogio da placa trabalha em segundos; so o timestamp da contagem enviada e em milissegundos.
// Sem isto, um servidor que passasse a responder em milissegundos estouraria o long de 32 bits do
// toInt() e o relogio seria acertado com lixo — e dai toda passagem sairia com data errada.
long epochSegundos(const String& recebido)
{
  long long valor = strtoll(recebido.c_str(), NULL, 10);
  if (valor > 1000000000000LL) valor /= 1000LL;   // veio em milissegundos
  if (valor < 1672531200LL || valor > 4102444800LL) return 0;   // fora de jan/2023 .. jan/2100
  return (long)valor;
}

// descrição dos códigos negativos do HTTPClient
String descricaoErroHttp(int httpCode)
{
  switch (httpCode)
  {
    case -1:  return "CONNECTION_FAILED";
    case -2:  return "SEND_HEADER_FAILED";
    case -3:  return "SEND_PAYLOAD_FAILED";
    case -4:  return "NOT_CONNECTED";
    case -5:  return "CONNECTION_LOST";
    case -6:  return "NO_STREAM";
    case -7:  return "NO_HTTP_SERVER";
    case -8:  return "TOO_LESS_RAM";
    case -9:  return "ERROR_ENCODING";
    case -10: return "STREAM_WRITE";
    case -11: return "READ_TIMEOUT";
    default:  return "HTTP " + String(httpCode);
  }
}

// Uma tentativa de pegar a hora no servidor. Retorna true se recebeu e aplicou um epoch válido.
// Não reinicia a placa: quem decide isso é sincronizarRelogio().
bool tempo()
{
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  const char* path = "/recuperaTimestamp"; // caminho da API no Web Service

  if (WiFi.status() != WL_CONNECTED) // se não está conectado ao wifi, tenta por até 10 s
  {
    Serial.println("\nSem Wi-Fi, tentando conectar para atualizar a hora... ");
    if (otaativ == 1)
    {
      WiFi.begin(ssidOTA, passwordOTA);
    }
    else
    {
      WiFi.begin(ssid, password);
    }
    for (x = 0; x < 20 && WiFi.status() != WL_CONNECTED; x++)
    {
      delay(500);
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    }
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("\nFalhou ao conectar ao wifi ");
      return false;
    }
  }

  Serial.println("\nConectado ao wifi, solicitando data e hora ao servidor ");
  HTTPClient http;
  http.begin(host, port, path);
  http.addHeader("Content-Type", "application/json");
  http.setConnectTimeout(3000);
  http.setTimeout(3000);

  int httpCode = http.GET();
  bool ok = false;
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    //separa dados do payload: {"...":epoch}
    ind1 = payload.indexOf(':');
    ind2 = payload.indexOf('}', ind1 + 1 );
    recebetempo = payload.substring(ind1 + 1, ind2 );
    long epoch = epochSegundos(recebetempo);
    if (epoch > 0)
    {
      atualizatempo = epoch;
      rtc.setTime(epoch);
      ok = true;
      Serial.println("\nRequisição ok, data atualizada: " + rtc.getTime("%d/%m/%y %T"));
    }
    else
    {
      Serial.println("\nServidor respondeu, mas o epoch e invalido: " + payload);
    }
  }
  else
  {
    errormes = descricaoErroHttp(httpCode);
    Serial.println("\nErro na requisição de data e hora. Código: " + String(httpCode) + " (" + errormes + ")");
  }
  http.end();
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  return ok;
}

// Chamada no setup(). O relógio do ESP32 sobrevive a ESP.restart() e ao watchdog (não a queda de energia).
// - relógio já válido: uma tentativa basta. Sem servidor, segue contando em modo offline: as
//   contagens vão para o SD e o servidor reajusta a hora em cada contagem que conseguir enviar.
// - relógio inválido (energia ligada sem servidor): insiste por ~2 min e reinicia, porque
//   contagem sem hora ("E2") não serve ao BI.
void sincronizarRelogio()
{
  const int MAX_TENTATIVAS_SEM_RELOGIO = 8;
  int tentativa = 0;
  while (true)
  {
    tentativa++;
    Serial.println("\nAtualizando data e hora, tentativa " + String(tentativa));
    if (tempo())
    {
      return;
    }
    if (relogioValido())
    {
      Serial.println("\nServidor indisponivel, mas o relogio ja e valido: seguindo em modo offline.");
      return;
    }
    if (tentativa >= MAX_TENTATIVAS_SEM_RELOGIO)
    {
      Serial.println("\nSem hora e sem servidor, reiniciando... ");
      delay(5000);
      ESP.restart();
    }
    delay(5000);
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  }
}

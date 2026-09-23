// Resumo periódico no Telegram (no fechamento da estação; na bancada, a cada RESUMO_INTERVALO_MIN)
// e tarefas executadas apenas com a catraca parada.
//
// O resumo mostra o que entrou desde o resumo anterior (referência guardada em Preferences,
// namespace "resumo") e os totais acumulados do SD, para conferência com o BI no dia seguinte.

bool resumoEnviadoNesteMinuto = false;   // estação: evita repetir dentro do mesmo minuto
unsigned long ultimoResumo = 0;           // bancada: controle do intervalo

// Conta as linhas de LOGREG.csv ainda não reenviadas (a partir de nextposition).
// Monta o cartão; retorna -1 se o SD não responder. Não altera os flags de contagem.
long contarPendentesSD()
{
  long n = -1;
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  digitalWrite(SD_CS, Select);
  if (SD.begin( SD_CS, SD_MOSI, SD_MISO, SD_CLK ))
  {
    n = 0;
    root = SD.open("LOGREG.csv");
    if (root)
    {
      root.seek(nextposition);
      long lidos = 0;
      while (root.available())
      {
        if (root.read() == '\n') n++;
        if (++lidos % 1024 == 0) esp_task_wdt_reset();
      }
      root.close();
    }
  }
  digitalWrite(SD_CS, DeSelect);
  SD.end();
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  return n;
}

void enviarResumo()
{
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  // referência do resumo anterior
  prefsCBA.begin("resumo", false);
  bool primeiro = !prefsCBA.isKey("E");
  int e0 = (int)prefsCBA.getUInt("E", 0);
  int s0 = (int)prefsCBA.getUInt("S", 0);
  int w0 = (int)prefsCBA.getUInt("W", 0);
  prefsCBA.putUInt("E", (unsigned int)counterE);
  prefsCBA.putUInt("S", (unsigned int)counterS);
  prefsCBA.putUInt("W", (unsigned int)counterW);
  prefsCBA.end();

  long pendentes = contarPendentesSD();

  String m = "Resumo " + rtc.getTime("%d/%m %H:%M") + " - v" FW_VERSION;
  if (primeiro)
  {
    m += "\nEntradas: " + String(counterE) + " | Saidas: " + String(counterS) + " | Erros: " + String(counterW) + " (totais acumulados; a partir do proximo resumo mostro a diferenca)";
  }
  else
  {
    m += "\nEntradas: +" + String(counterE - e0) + " (total " + String(counterE) + ")";
    m += "\nSaidas: +" + String(counterS - s0) + " (total " + String(counterS) + ")";
    m += "\nErros de giro: +" + String(counterW - w0) + " (total " + String(counterW) + ")";
  }
  m += "\nPendentes no SD: " + (pendentes < 0 ? String("SD SEM RESPOSTA") : String(pendentes));
  m += " | perdidas na flash: " + String(counterR);
  m += "\nWi-Fi " + String(WiFi.RSSI()) + " dBm | ligado ha " + String(millis() / 3600000UL) + " h | heap " + String(ESP.getFreeHeap() / 1024) + " KB";
  if (!enviarTelegram(cabecalhoTelegram() + "\n" + m))
  {
    // sem rede na hora do resumo: fica agendado e sai quando o Wi-Fi voltar (a referência já foi
    // atualizada acima, então o texto agendado continua sendo o resumo daquele período).
    // enviarTelegramPendente() prefixa o cabeçalho.
    agendarTelegram(m);
  }
}

// Vigia de estado preso: ver TEMPO_MAX_ESTADO_MS no sketch principal.
// Devolve true quando desistiu de esperar e devolveu a catraca ao repouso; quem chama deve
// sair do loop() (um "return"), porque loop() recomeça zerando cont e volta ao estado ocioso.
unsigned long marcoEstado = 0;    // millis() em que cont assumiu o valor atual
int contMarcado = 0;              // valor de cont observado na última verificação
bool avisoPresoEnviado = false;   // um aviso no Telegram por ocorrência

bool estadoPreso()
{
  if (cont != contMarcado)      // mudou de estado: reinicia a contagem de tempo
  {
    contMarcado = cont;
    marcoEstado = millis();
    if (cont == 0) avisoPresoEnviado = false;   // voltou ao repouso: pode avisar de novo numa próxima vez
    return false;
  }
  if (cont == 0) return false;                              // repouso não é estado preso
  if (millis() - marcoEstado < TEMPO_MAX_ESTADO_MS) return false;

  Serial.println(String("[Vigia] Catraca parada fora do repouso (cont = ") + String(cont) + ") ha mais de "
               + String(TEMPO_MAX_ESTADO_MS / 1000) + " s. Voltando ao repouso.");
  if (!avisoPresoEnviado)
  {
    avisoPresoEnviado = true;
    agendarTelegram("Catraca parada fora do repouso (cont = " + String(cont) + ") por mais de "
                  + String(TEMPO_MAX_ESTADO_MS / 1000) + " s. Verifique os sensores. "
                  + "A placa voltou ao repouso para seguir contando e se comunicando.");
  }
  cont = 0;
  direcao = 0;
  contMarcado = 0;
  marcoEstado = millis();
  return true;
}

// Chamada no loop ocioso.
void verificarHorarioResumo()
{
#if AMBIENTE_LAB
  if (millis() - ultimoResumo >= (unsigned long)RESUMO_INTERVALO_MIN * 60000UL)
  {
    ultimoResumo = millis();
    enviarResumo();
  }
#else
  if (!relogioValido()) return;
  if (rtc.getHour(true) == RESUMO_HORA && rtc.getMinute() == RESUMO_MINUTO)
  {
    if (!resumoEnviadoNesteMinuto)
    {
      resumoEnviadoNesteMinuto = true;
      enviarResumo();
    }
  }
  else
  {
    resumoEnviadoNesteMinuto = false;
  }
#endif
}

// Tarefas que só rodam com a catraca parada (cont == 0). Qualquer uma pode bloquear alguns
// segundos (rede); nada é contado enquanto isso, por isso ficam fora dos estados de giro.
void tarefasOciosas()
{
  ArduinoOTA.handle();        // programação pela IDE (porta de rede)
  verificarHorarioOTA();      // atualização pelo GitHub
  verificarHorarioResumo();   // resumo no Telegram
  enviarTelegramPendente();   // avisos agendados durante a contagem (ex.: falha do SD)
}

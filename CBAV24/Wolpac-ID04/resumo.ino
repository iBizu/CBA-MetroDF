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
  // So aparece quando houve algo a mostrar, para nao poluir o resumo das placas que nao veem
  // ruido nenhum (no modelo 1 a confirmacao nem chega a rodar).
  // Quanto tempo a placa passa cega depois de cada passagem. E aqui que se ve se o envio e o
  // cartao ainda estao comendo a pessoa seguinte.
  if (janelaCegaN > 0)
  {
    m += "\nJanela cega: media " + String(janelaCegaSoma / janelaCegaN) + " ms, maxima " + String(janelaCegaMax)
       + " ms (" + String(janelaCegaN) + " passagens)";
    janelaCegaSoma = 0;
    janelaCegaMax = 0;
    janelaCegaN = 0;
  }
  if (ruidoDescartado > 0)
  {
    m += "\nRuido descartado: " + String(ruidoDescartado) + " (transicao que nao se confirmou; seriam contagens falsas)";
    ruidoDescartado = 0;
  }
  // Saltos de meia volta. O segundo numero e o que interessa: sao os saltos que fechariam uma
  // passagem, isto e, os candidatos a contagem falsa por empurrao. Ver DESCARTA_SALTO_MEIA_VOLTA.
  if (saltoMeiaVolta > 0)
  {
    m += "\nSaltos de meia volta: " + String(saltoMeiaVolta) + " | fechariam passagem: " + String(passagemPorSalto);
#if DESCARTA_SALTO_MEIA_VOLTA
    m += " (descartadas)";
#else
    m += " (contadas)";
#endif
    if (passagemPorSalto > 0)
    {
      // So um dos dois casos e empurrao. "Voltou ao repouso" e a barreira de volta no lugar (meia
      // volta abandonada); "pulou um passo" e um giro de verdade em que a placa perdeu um estado,
      // e descartar esse custa uma passagem real.
      m += "\n  voltou ao repouso (empurrao): " + String(saltoVoltouRepouso)
         + " | pulou um passo do giro: " + String(passagemPorSalto - saltoVoltouRepouso);
    }
    saltoMeiaVolta = 0;
    passagemPorSalto = 0;
    saltoVoltouRepouso = 0;
  }
  // Giro completo no sentido que esta catraca nao faz (ver SENTIDO_UNICO). Nao foi enviado.
  if (giroImpossivel > 0)
  {
    m += "\nGiros no sentido impossivel: " + String(giroImpossivel) + " (descartados)";
    if (giroImpossivel >= GIRO_IMPOSSIVEL_ALERTA)
    {
      m += "\nATENCAO: muitos giros no sentido errado. Confira se os cabos dos sensores foram trocados.";
    }
    giroImpossivel = 0;
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

// Grava no cartao os contadores que estao so na RAM. Chamada do laco ocioso, e so depois de a
// catraca ficar parada por FLUSH_CONTADORES_MS: gravar na primeira iteracao ociosa cairia no mesmo
// instante em que a gravacao acontecia antes, que e justamente quando vem a pessoa seguinte.
void gravarContadores()
{
  if (!contadoresSujos) return;
  if (millis() - ultimaPassagem < FLUSH_CONTADORES_MS) return;   // ainda pode vir gente atras

  flagE = 1; flagS = 1; flagW = 1;
  escreve = 1; cardID();   // apaga os txt antigos
  escreve = 0; cardID();   // regrava a partir da RAM
  // Se o cartao falhou no meio, o fail() ja pos os flags em 2 para recriar tudo na proxima
  // montagem que der certo -- nao desfazer isso.
  if (flagE == 1) flagE = 0;
  if (flagS == 1) flagS = 0;
  if (flagW == 1) flagW = 0;
  if (flagF == 0) contadoresSujos = false;   // so da por gravado se o cartao respondeu
}

// Registra quanto tempo a placa ficou sem ler os sensores por causa de uma passagem.
void registrarJanelaCega(unsigned long ms)
{
  janelaCegaN++;
  janelaCegaSoma += ms;
  if (ms > janelaCegaMax) janelaCegaMax = ms;
  ultimaPassagem = millis();
}

// Chamada no loop ocioso.
void verificarHorarioResumo()
{
  if (ambienteLab)
  {
    if (millis() - ultimoResumo >= (unsigned long)RESUMO_INTERVALO_MIN * 60000UL)
    {
      ultimoResumo = millis();
      enviarResumo();
    }
    return;
  }
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
}

// Tarefas que só rodam com a catraca parada (cont == 0). Qualquer uma pode bloquear alguns
// segundos (rede); nada é contado enquanto isso, por isso ficam fora dos estados de giro.
void tarefasOciosas()
{
  ArduinoOTA.handle();        // programação pela IDE (porta de rede)
  gravarContadores();         // persiste os contadores com a catraca parada (ver acima)
  verificarHorarioOTA();      // atualização pelo GitHub
  verificarHorarioResumo();   // resumo no Telegram
  enviarTelegramPendente();   // avisos agendados durante a contagem (ex.: falha do SD)
}

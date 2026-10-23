// Descoberta do ambiente no boot: a placa tenta conectar e deixa a rede que atende decidir o
// servidor, o canal de OTA e os horários. Ver o bloco "Ambiente" no sketch principal.

// Aplica tudo o que depende do ambiente.
void aplicarAmbiente(int lab)
{
  ambienteLab = lab;
  ssid     = lab ? SSID_LAB  : SSID_ESTACAO;
  password = lab ? SENHA_LAB : SENHA_ESTACAO;
  host     = lab ? HOST_LAB  : HOST_ESTACAO;
  const char* base = lab ? OTA_URL_LAB : OTA_URL_ESTACAO;
  url_versao_txt   = String(base) + "versao.txt";
  url_firmware_ota = String(base) + OTA_ASSET;
}

const char* nomeAmbiente(int lab)
{
  return lab ? "LABORATORIO" : "ESTACAO";
}

// Uma tentativa de associar, com limite próprio. Devolve true se conectou.
// Conectar é a prova que interessa: a POC_MANUTENCAO é rede oculta e não aparece em varredura, e
// associar também confirma senha certa e AP aceitando a placa, o que um nome no ar não confirma.
bool tentarRede(const char* rede, const char* senha, int segundos)
{
  Serial.print("   rede \"" + String(rede) + "\" (ate " + String(segundos) + "s) ");
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(rede, senha);
  for (int i = 0; i < segundos * 2 && WiFi.status() != WL_CONNECTED; i++)
  {
    delay(500);
    Serial.print(".");
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  }
  bool ok = (WiFi.status() == WL_CONNECTED);
  Serial.println(ok ? " conectado." : " sem resposta.");
  return ok;
}

// Chamada por setupOTA(). Ordem das tentativas: primeiro o último ambiente conhecido (na NVS), que
// é o caso normal e deixa o boot rápido; depois o outro, que cobre a placa que mudou de lugar.
//
// Se NENHUMA das duas redes responder, mantém o último conhecido e segue em modo offline, contando
// para o SD. Não se adivinha aqui: uma placa da estação com o AP fora do ar que "caísse para a
// bancada" passaria a apontar para o servidor de teste e para o canal lab de OTA, e só voltaria com
// uma visita presencial. Sem rede não há informação nova, então nada muda.
//
// Não existe nova tentativa durante a operação, de propósito: a detecção custa segundos e a contagem
// é bloqueante, então re-sondar no loop cegaria a catraca. E não é preciso — para trocar de lugar a
// placa é desligada da tomada, ou seja, passa por um boot.
void conectarAmbiente()
{
  prefsCBA.begin("ambiente", false);
  int lembrado = prefsCBA.getInt("lab", AMBIENTE_LAB);
  prefsCBA.end();
  if (lembrado != 0 && lembrado != 1)
  {
    lembrado = AMBIENTE_LAB;   // NVS com lixo: volta para a semente de compilação
  }
  aplicarAmbiente(lembrado);
  ambienteDetectado = 0;
  ambienteMudou = 0;

  // Jumper nos pinos 0 e 4: "modo físico OTA", o caminho de emergência para gravar a placa pelo
  // Wi-Fi quando o conector USB está com defeito. Vale a rede de programação, e o ambiente NÃO é
  // redefinido — a placa está ali só para ser gravada, não para passar a viver naquele lugar.
  if (otaativ == 1)
  {
    Serial.println("Modo físico OTA (jumper nos pinos 0 e 4). Rede de programação:");
    Serial.println("Ambiente mantido da NVS: " + String(nomeAmbiente(ambienteLab)));
    tentarRede(ssidOTA, passwordOTA, TEMPO_REDE_S);
    return;
  }

  Serial.println("Descobrindo o ambiente (ultimo conhecido: " + String(nomeAmbiente(lembrado)) + ")");
  int ordem[2] = { lembrado, lembrado ? 0 : 1 };
  for (int i = 0; i < 2; i++)
  {
    int cand = ordem[i];
    Serial.println(" -> " + String(nomeAmbiente(cand)));
    if (tentarRede(cand ? SSID_LAB : SSID_ESTACAO, cand ? SENHA_LAB : SENHA_ESTACAO, TEMPO_REDE_S))
    {
      aplicarAmbiente(cand);
      ambienteDetectado = 1;
      if (cand != lembrado)
      {
        ambienteMudou = 1;
        prefsCBA.begin("ambiente", false);
        prefsCBA.putInt("lab", cand);
        prefsCBA.end();
        Serial.println("Ambiente mudou para " + String(nomeAmbiente(cand)) + " e foi gravado na NVS.");
      }
      Serial.println("Ambiente: " + String(nomeAmbiente(ambienteLab)) + " | servidor " + String(host) + ":" + String(port));
      Serial.println("Canal de OTA: " + url_versao_txt);
      return;
    }
  }

  Serial.println("[Alerta] Nenhuma rede conhecida respondeu. Mantendo " +
                 String(nomeAmbiente(ambienteLab)) + " e iniciando em modo OFFLINE.");
}

// --- Backlog que atravessou ambientes --------------------------------------------------------
// Uma linha do LOGREG.csv guarda so "sentido;timestamp;": ela nao sabe para qual servidor era.
// O atrasado() entrega o backlog ao host do ambiente em que a placa esta AGORA, entao uma placa
// que acumulou passagens reais na estacao e foi trazida para o laboratorio despejaria essas
// contagens no banco de teste, e o BI de producao nunca as receberia. No sentido inverso, giros
// de bancada entrariam na producao como se fossem passageiros. Nos dois casos em silencio: as
// linhas sao validas, o servidor responde 000 e elas somem do arquivo.
//
// Por isso o ambiente em que o backlog foi gravado fica na NVS (namespace "ambiente", chave
// "backlog"). Se nao bate com o ambiente atual, o backlog NAO e enviado: a placa avisa na
// mensagem de boot do Telegram e espera. Se voltar ao ambiente de origem, o registro passa a
// bater e o backlog drena normalmente, sem perder contagem nenhuma — e por isso bloquear e
// melhor que descartar.
#define BACKLOG_SEM_ORIGEM (-1)
#define BACKLOG_MISTO        2

const char* nomeOrigemBacklog(int origem)
{
  if (origem == BACKLOG_MISTO) return "MISTO";
  return nomeAmbiente(origem);
}

// Origem do backlog atual, ou BACKLOG_SEM_ORIGEM se nao houver marca.
int ambienteDoBacklog()
{
  prefsCBA.begin("ambiente", true);
  int origem = prefsCBA.getInt("backlog", BACKLOG_SEM_ORIGEM);
  prefsCBA.end();
  return origem;
}

// Chamada pelo card() ao acrescentar uma linha ao LOGREG.csv. So escreve na NVS quando o valor
// muda, para nao gastar o flash a cada passagem perdida.
// Se ja havia backlog de OUTRO ambiente, o arquivo passa a ter linhas das duas origens e nao da
// mais para rotear por arquivo: marca MISTO, que fica bloqueado sempre, e a decisao vira humana.
void marcarAmbienteBacklog()
{
  int origem = ambienteDoBacklog();
  if (origem == ambienteLab || origem == BACKLOG_MISTO) return;
  int novo = (origem == BACKLOG_SEM_ORIGEM) ? ambienteLab : BACKLOG_MISTO;
  prefsCBA.begin("ambiente", false);
  prefsCBA.putInt("backlog", novo);
  prefsCBA.end();
  if (novo == BACKLOG_MISTO)
  {
    Serial.println("[SD] Backlog passou a ter linhas de dois ambientes: marcado MISTO.");
  }
}

// Chamada quando o LOGREG.csv acabou de ser drenado ou apagado: nao ha mais backlog a proteger.
void limparAmbienteBacklog()
{
  if (ambienteDoBacklog() == BACKLOG_SEM_ORIGEM) return;
  prefsCBA.begin("ambiente", false);
  prefsCBA.remove("backlog");
  prefsCBA.end();
}

// Backlog que ja estava no cartao antes desta protecao nao tem origem marcada. No primeiro boot
// assume o ambiente atual, que e onde ele foi acumulado; a partir dai a protecao vale. Sem isto
// essas linhas ficariam sem origem para sempre e nunca seriam protegidas.
// Chamada no setup(), depois do leId(), que e quem define flagSD.
// Contagens guardadas SO na flash (NVS "my-app"/counterF), de quando servidor e cartao falharam
// juntos. Lido direto da NVS porque o counterR global so e preenchido no loop(), e isto roda no
// setup(). Usa o prefsCBA, livre neste ponto: o preferences so abre dentro do loop().
unsigned int pendentesNaFlash()
{
  prefsCBA.begin("my-app", true);
  unsigned int n = prefsCBA.getUInt("counterF", 0);
  prefsCBA.end();
  return n;
}

void adotarAmbienteBacklog()
{
  // flagSD so quer dizer algo se o cartao respondeu: o fail() nao mexe nela, entao depois de uma
  // falha de monta ela ainda guarda o valor da leitura anterior. O contador da flash sempre vale.
  bool cartaoMudo = (flagF != 0);
  bool temBacklog = (!cartaoMudo && flagSD == 1) || pendentesNaFlash() > 0;

  if (!temBacklog)
  {
    // Marca orfa: nao ha nada guardado em lugar nenhum, mas sobrou marca. Acontece quando o
    // LOGREG.csv e apagado na mao, que e como o cartao se limpa aqui. Deixar a marca faria a
    // proxima contagem perdida ser lida como uma SEGUNDA origem e o conjunto viraria MISTO --
    // bloqueado para sempre, tendo origem unica. So limpa com o cartao respondendo: com ele
    // mudo nao da para afirmar que o LOGREG.csv esta vazio.
    if (!cartaoMudo && ambienteDoBacklog() != BACKLOG_SEM_ORIGEM)
    {
      limparAmbienteBacklog();
      Serial.println("[SD] Nao ha backlog, mas havia marca de origem: marca removida.");
    }
    return;
  }
  if (ambienteDoBacklog() != BACKLOG_SEM_ORIGEM) return;     // ja tem origem
  prefsCBA.begin("ambiente", false);
  prefsCBA.putInt("backlog", ambienteLab);
  prefsCBA.end();
  // Isto e um PALPITE: a informacao de origem nao existe nesses dados. Costuma acertar, porque o
  // normal e a placa nao ter saido do lugar; erra justamente quando ela saiu, que e o caso de um
  // cartao com defeito trazido para a bancada. Por isso vai para a mensagem de boot: o operador
  // ve o que foi assumido e pode discordar antes da primeira passagem drenar o backlog.
  origemBacklogAdotada = true;
  Serial.println("[SD] Backlog sem origem marcada: assumindo " + String(nomeAmbiente(ambienteLab)) + ".");
}

// true = ha backlog gravado em outro ambiente; o atrasado() nao deve enviar.
bool backlogDeOutroAmbiente()
{
  int origem = ambienteDoBacklog();
  if (origem == BACKLOG_SEM_ORIGEM || origem == ambienteLab) return false;
  Serial.println("[SD] Backlog gravado em " + String(nomeOrigemBacklog(origem)) +
                 " e a placa esta em " + String(nomeAmbiente(ambienteLab)) +
                 ": nao vou enviar, iria para o servidor errado.");
  return true;
}

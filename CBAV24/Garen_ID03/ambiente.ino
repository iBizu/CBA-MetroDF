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

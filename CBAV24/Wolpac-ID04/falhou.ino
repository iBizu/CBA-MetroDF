// texto se o cartão SD falhar
void falhou()
{
  static bool avisoAgendado = false;   // um aviso no Telegram por boot, para não inundar o grupo

  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  Serial.println("\nSD falhou! "); // imprime mensagem na porta serial

  if (!avisoAgendado)
  {
    avisoAgendado = true;
    // agendado (não enviado agora): o envio bloqueia alguns segundos e este ponto pode estar no meio de uma contagem
    agendarTelegram("FALHA no cartao SD. Contagens que nao chegarem ao servidor vao para a memoria flash (so o total, sem hora).");
  }
  return;
}

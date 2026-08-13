// texto se o cartão SD falhar
void falhou()
{

  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  Serial.println("\nSD falhou! "); // imprime mensagem na porta serial    
  
  return;
}


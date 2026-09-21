//função para caso falhe a comunicação com o SD
void fail()
{
  
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
 // volta os flags para a opção de criação de arquivo
 flagM = 2;        
 flagE = 2;        
 flagS = 2;        
 flagW = 2;        
 flagP = 2; 
  return;
} // fim void fail()


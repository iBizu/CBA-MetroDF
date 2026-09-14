//Função que envia o pacote  
void sendMessage() 

 
{   

  
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  if (flagSD != 2 )   //  se estiver enviando instantaneamente, chama leId 
  {  

    leId();    // chama a função leId, para pegar do cartão SD o numero ID e a quantidade de entradas, saidas, etc


    if (flagF == 0)    // se consegiu  comunicar com SD
   {
      outgoing =   String(numid) + ";" + timestamp  + ";" + String(atraso) + ";"  + "0" +  ";" + String(sentido) + ";"  ;     //cria o pacote somando senha, sentido, timestamp e id da mensagem
   }
   if (flagF == 1)    // se não consegiu  comunicar com SD
   {
      outgoing =   String(numid) + ";" + timestamp  + ";" + String(atraso) + ";"  + "1" +  ";" + String(sentido) + ";"  ;     //cria o pacote somando senha, sentido, timestamp e id da mensagem
   }


  }     
  
  
   // fim if (flagSD ! =2 && resposta != "222")
  if (flagSD == 2 )   //  se estiver enviando com atraso carrega outro pacote para enviar ao coordenador
  {                                                                                           
    //outgoing =  String(numid) + ";" + timestamp2 + ";" + "1"  + ";" + "0" + ";" + sentido2 + ";"  ;  //cria o pacote somando senha, sentido, timestamp e id armazenados no SD
    outgoing =  String(numid) + ";" + timestamp2  + "1"  + ";" + "0" + ";" + sentido2 + ";"  ;  //cria o pacote somando senha, sentido, timestamp e id armazenados no SD
  }      // fim if (flagSD == 2 & )


 
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   servidor(); 
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    
}    // fim  void sendMessage()

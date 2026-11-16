//Função que envia o pacote  
void sendMessage()
{
  // Mede a janela cega: daqui ate o fim do servidor() a placa nao le os sensores. So conta no
  // envio ao vivo (flagSD != 2); o reenvio de backlog roda com a catraca parada e nao atrapalha.
  unsigned long inicioJanela = millis();
  bool passagemAoVivo = (flagSD != 2);

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
    // pacote atrasado: numid;timestamp;atraso=1;sdErr=0;sentido;  (mesmo formato do envio instantaneo)
    outgoing =  String(numid) + ";" + timestamp2 + ";" + "1"  + ";" + "0" + ";" + sentido2 + ";"  ;
  }      // fim if (flagSD == 2 & )


 
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   servidor();
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  // A medida nao inclui o bloco de reconciliacao no topo do loop(), que ainda roda antes de a
  // leitura dos sensores recomecar: o numero real e um pouco maior que este.
  if (passagemAoVivo) registrarJanelaCega(millis() - inicioJanela);
}    // fim  void sendMessage()

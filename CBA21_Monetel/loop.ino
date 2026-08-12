// função em loop
void loop()
{   
  // Verifica constantemente se é 03:00 da manhã para atualizar via GitHub
  verificarHorarioOTA();

     Serial.println("\nLoop inciado! "); // imprime mensagem na porta serial

   cont = 0 ;
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
 // Open Preferences with my-app namespace. Each application module, library, etc   // has to use a namespace name to prevent key name collisions. We will open storage in
 // RW-mode (second parameter has to be false).
 // Note: Namespace name is limited to 15 chars.
 preferences.begin("my-app", false); //  The begin() method opens a “storage space” with a defined namespace. The false argument means that we’ll use it in read/write mode. Use true to open or create the namespace in read-only mode.
 // Remove all preferences under the opened namespace:  preferences.clear();
 // Or remove the counter key only:   preferences.remove("counter");
 // Get the counter value, if the key does not exist, return a default value of 0
 // Note: Key name is limited to 15 chars.
 unsigned int counterF = preferences.getUInt("counterF", 0);    // contador para giro da catraca e armazenar na memoria flash
 preferences.putUInt("counterF", counterF);    // variavel counterF recebe o que está no namespace counterF (memoria flash)
 counterR = counterF;   // grava o conteudo em counterR para exibir no displa
 if (counterR != 0 )
 {
   flagR = 1;  //  flagR indica que há algo na memoria flash
 }
       

 
  if(flagSERV == 0)  // se enviou ao servidor
  {

   

   if ( flagF == 0 )   // se  consegiu comunicar com o SD
   {
 
      if (flagSD == 0)   // se flagSD indica que não há nada salvo no SD a ser enviado
      {

        escreve = 1;   //seleciona escreve como apagar arquivo no SD
        card(stamp);   // chama a função void card()
        resposta = "999";    // reseta resposta
      }
      if (flagSD == 1 || counterR != 0 )  // se flagSD ou counterR indicam que existe informação no SD e/ou na memoria flash que deve ser enviada ao coordenador
      {           Serial.println( " Inicio do envio de atrasados 1: " + String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual


        atrasado();     // chama função que envia dados armazenados no SD
                  Serial.println( "Fim do envio de atrasados 2: " + String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual

        if (flagR == 0)  // se flagR indica que não há mais informação na meoria flash
        {
          counterF = 0 ;       // zera counterF
          preferences.putUInt("counterF", counterF);   // grava o conteudo de counter F na memoria flash
          counterR = counterF;    // zera counterR para exibir no display
          preferences.end();   // finaliza preferences (memoria flash)
        }  // fim if (flagR == 0)
        resposta = "999";                    // reseta resposta
      }  // fim if (flagSD == 1 || counterR != 0 )
    } // fim  if ( flagF == 0 )
  }  //  if(flagSERV == 0)


                   
    //grava no SD
    if(flagSERV == 1) // se  falhou ao enviar para o  servidor 
    {

      
    escreve = 0;    // seleciona escreve como escrever no SD
    outgoing = String(sentido) + ";" + timestamp + ";"  ;  //cria o pacote somando  sentido e timestamp   
    card(outgoing);    // chama função void card() para escrever no SD
    if ( flagF == 1 )  // se falhou ao gravar os dados no SD, manda gravar na memoria Flash
    {
      flagR = 1 ;    // flag indica que há algo na memoria flash
      //gravando na memoria flash:
      counterF ++;     // conta uma giro a mais a ser gravado na memoria flash
      preferences.putUInt("counterF", counterF);   // grava o conteudo de counter F na memoria flash
      counterR = counterF;    // grava o counteudo em counterR para exibir no display
      preferences.end();   // finaliza preferences (memoria flash)
    } // fim if ( flagF == 1 ) 
    resposta = "999";   // reseta resposta para aguardar reposta 000 do coordenador no proximo ciclo
    flagSD = 1;    // flag para informar que existe informação no SD que deve ser enviada ao coordenador
   }       // fim if(flagSERV == 1)

   if(resposta == "111" || resposta == "222" || resposta == "333" )               // se não recebeu resposta do coordenardor, ou se coordenador indica que não conseguiu comunicar com o SD armazena dados no cartão SD
                 {         

                                    Serial.println( "Gravando no SD: " + String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual


                  //grava no SD
                  escreve = 0;                      // seleciona escreve como escrever no SD
        
                  outgoing = String(sentido) + ";" + timestamp + ";" + String(msgId) ;   //cria o pacote somando senha, sentido, timestamp e id da mensagem   
                        
                  card(outgoing);                   // chama função void card() para escrever no SD

                   if ( flagF == 1 )  // se falhou ao gravar os dados no SD, manda gravar na memoria Flash
                   {
                    flagR = 1 ;       // flag indica que há algo na memoria flash

                      //gravando na memoria flash:
                      counterF ++;       // conta uma giro a mais a ser gravado na memoria flash
                      preferences.putUInt("counterF", counterF);      // grava o conteudo de counter F na memoria flash
                     counterR = counterF;    // grava o counteudo em counterR para exibir no display
                     preferences.end();   // finaliza preferences (memoria flash)
 
                    
                   } // fim if ( flagF == 1 ) 
                  
                  resposta = "999";                 // reseta resposta para aguardar reposta 000 do coordenador no proximo ciclo
                  flagSD = 1;                       // flag para informar que existe informação no SD que deve ser enviada ao coordenador
                 }                                  // fim if(resposta == "111") 


      Serial.println("\nStand by... "); // imprime mensagem na porta serial    


//reseta estado dos botões/sensores
estadoBtn1 = HIGH;   // estado inicial do botão 1 = 1
estadoBtn2 = HIGH;   // estado inicial do botao 2 = 1
estadoBtn3 = HIGH;   // estado inicial do botao 3 = 0
estadoBtn4 = HIGH;   // estado inicial do botao 4 = 0
// reseta o valor das entradas analogicas
valor1 = 4095;
valor2 = 4095;


delay(25);

if (modelo == 1)  // se o bloqueio é modelo foca. garen ou wolpac
{

  while (cont == 0)   //loop enquanto não há movimento da catraca posição 0 (posição inicial)
  {
   esp_task_wdt_reset(); //Reseta o temporizador do watchdog
 
 // Esta linha deve ser adicionada EM ALGUM LUGAR do loop:
 // Pode ser no início, final, ou em algum ponto que execute frequentemente
  ArduinoOTA.handle();
    


  if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 

       if( valor1 > 450 && valor2  > 450) // verifica entradas analogicas
    {
      cont = 0;   // mantém ponto inicial
    }

      if( valor1 < 300 && valor2  > 450) // verifica entradas analogicas
    {
      cont++;   // um quarto de giro para frente
      
    }
 if( valor1> 450 && valor2  < 300) // verifica entradas analogicas
    {
      cont--;  // um quarto de giro para trás
    }

    
    if( valor1 < 300 && valor2 < 300)
    {
      delay(10);  //aguarda para verificar se é erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if( valor1 < 300 && valor2 < 300)
    {
      if(direcao == 0 || direcao == 1)
      {
      cont = 2 ;  // pula para meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -2 ;  // pula para meia volta (possivel erro)
      }
    }
    }
   
  } // fim  if (tipoEntr == 1)




  if (tipoEntr == 2)
    {
      //recupera o estado do botao
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 

       if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = 0;   // mantém o valor incial
    }   

    if(estadoBtn3 == LOW && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont++;   // um quarto de giro para frente
    }     
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    {
      cont--;  // um quarto de giro para trás
    }    
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {      
       if(direcao == 0 || direcao == 1)
      {
      cont = 2 ;  // pula para meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -2 ;  // pula para meia volta (possivel erro)
      }
    }  
    }
  }   // fim if (tipoEntr == 2)



  

   if(cont != 0)
   {
    delay(25); // delay se mudou de estado
   }
}    // fim     while (cont == 0) 



if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}

  while (cont == 1)  // loop enquanto não há movimento da catraca posição 1 (1 quarto de giro para frente)
  { 
 
    
     esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     
   
    if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

      if( valor1 < 300 && valor2  > 450) // verifica entradas analogicas
    {
      cont = 1;   // mantém ponto inicial
    }
    if(valor1 < 300  &&   valor2 < 300 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 > 450 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 > 450 &&   valor2 < 300 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // delay para verificar possivel erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 > 450 &&   valor2 < 300 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -1 ;  // pula  meia volta (possivel erro)
      }
    }
    }     


     } // fim  if (tipoEntr == 1)




    if (tipoEntr == 2)
  {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2  


   if(estadoBtn3 == LOW && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = 1;   // mantém o valor incial
    }  
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }                    
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW) 
      {
      if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -1 ;  // pula  meia volta (possivel erro)
      }
      }
    }     
  } // fim  if (tipoEntr == 2)



    

   if(cont != 1)
   {
    delay(25); // delay se mudou de estado
   }

  }     // fim   while (cont == 1)   



  while (cont == 2)    // loop enquanto não há movimento da catraca posição 2 (dois quartos de giro para frente)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog


 if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    
      if(valor1 < 300  &&   valor2 < 300) // verifica entradas analogicas
    {
      cont = 2;   // mantém ponto inicial
    }
    if(valor1 > 450  &&   valor2 < 300 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 < 300  &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 > 450 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda para verificar se é erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 > 450 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      }
    }
    }     
     
    } // fim  if (tipoEntr == 1)

  

  


  if (tipoEntr == 2)
    {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2     

    if(estadoBtn3 == LOW && estadoBtn4 == LOW)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = 2;   // mantém o valor incial
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont++;   // um quarto de giro para frente
    }                   
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO
    { 
      cont--;     // um quarto de giro para trás
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH) 
      {
      if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      }
      } 
    }   
  } // fim if (tipoEntr == 2)
   



    if(cont != 2)
   {
    delay(25); // delay se mudou de estado
   }
  }   // fim   while (cont == 2) 



  while (cont == 3)   // loop enquanto não há movimento da catraca posição 3 (três quartos de giro para frente)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog


 if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

     if( valor1 > 450 && valor2 < 300 ) // verifica entradas analogicas
    {
      cont = 3;   // mantém ponto inicial
    }
    if(valor1 > 450  &&   valor2 > 450 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 < 300  &&   valor2 < 300  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 < 300 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda par ver se é algum erro
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
     valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 < 300 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
       if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      }
    }
    }     
     
    } // fim  if (tipoEntr == 1)



     if (tipoEntr == 2)
    {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2  
     
     if(estadoBtn3 == HIGH && estadoBtn4 == LOW)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = 3;   // mantém o valor incial
    }  
   if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)  // verifica se o estado do sensor 2 mudou e se mudou para DESLIGADO
   { 
      cont++;  // um quarto de giro para frente
   }  
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)
    { 
      cont--;     // um quarto de giro para trás
    } 
   if(estadoBtn3 == LOW && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH) 
    {
 if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      }       
     }
  }
  }  // fim  if (tipoEntr == 2)
  



 if(cont != 3)
   {
    delay(25); // delay se mudou de estado
   }
    
  }    // fim    while (cont == 3)  

   
  if (cont == 4)  // posição 4 (giro completo para frente)
  { 
    direcao = 1;


    // Serial.end();
  //pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
  //digitalWrite(pinoLedInterno, LOW);
  //delay(400); 
  //digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
    //delay(100);

if(ledaux < 3)
{
  ledaux = ledaux + 1;
 digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   // determina timesamp
   ep =  rtc.getEpoch();   // carrega o epoch time em ep
   if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não conseguiu atualizar o epoch time)
   {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
   }
   if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
   {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
   }
   cont = 0 ;    // reseta contagem de giro                          
   flagE = 1;    // habilita flag para CounterE
   Serial.println("\nEnviando entrada... "); // imprime mensagem na porta serial 
   sentido = 1;    // define sentido como entrada  
   resposta = "111";   // altera resposta para aguardar resposta 000 do coordenador indicando recebimento
   atraso = "0";   // sem atraso


   sendMessage();    // chama função que envia o pacote
   esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   
    flagM = 0;    // desabilita flag para msgID
    flagE = 0;    // desabilita flag para CounterE
    delay(25);
  }   // fim if (cont==4)



  while (cont == -1)     //loop enquanto não há movimento da catraca posição -1 (1 quarto de giro para trás)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog



if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    if( valor1 > 450 && valor2  < 300) // verifica entradas analogicas
    {
      cont = -1;   // mantém ponto inicial
    }
    if(valor1 > 450  &&   valor2 > 450 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 < 300  &&   valor2 < 300  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 < 300 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguardar apra verificar possível erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
       valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
        if(valor1 < 300 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 1 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -3 ;  // pula  meia volta (possivel erro)
      } 
    }
    }      

     
    } // fim  if (tipoEntr == 1)


 
    if (tipoEntr == 2)
    {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2

    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = -1 ;   // mantém o valor incial
    } 
   if(estadoBtn3 == LOW && estadoBtn4 == LOW)
   { 
      cont--; // um quarto de giro para trás
   }  
   if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)
    { 
      cont++;    // um quarto de giro para frente
    }  
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(20); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH) 
      {
       if(direcao == 0 || direcao == 1)
      {
        cont = 1 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -3 ;  // pula  meia volta (possivel erro)
      } 
      } 
    }
  } // fim (tipoEntr == 2)

    

     if(cont != -1)
   {
    delay(25); // delay se mudou de estado
   } 
  }     // fim    while (cont == -1)  


  while (cont == -2)   //loop enquanto não há movimento da catraca posição -2 ( dois quartos de giro para trás)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

   

if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

     if( valor1 < 300 && valor2  < 300) // verifica entradas analogicas
    {
      cont = -2;   // mantém ponto inicial
    }
    if(valor1 > 450  &&   valor2 < 300 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 < 300  &&   valor2 > 450  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 > 450 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10);  // aguarda para verificar possivel erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
    if(valor1 > 450 &&   valor2 > 450 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    {
        if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      } 
    }
    }     
     

    } // fim  if (tipoEntr == 1)




    if (tipoEntr == 2)
    {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2

    if(estadoBtn3 == LOW && estadoBtn4 == LOW)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = -2 ;   // mantém o valor incial
    } 
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH)
    { 
      cont--;    // um quarto de giro para trás
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)
    { 
      cont++;   // um quarto de giro para frente
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH) 
      {
  if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      }       } 
    }
  } // fim  if (tipoEntr == 2)

               
   
     if(cont != -2)
   {
    delay(25); // delay se mudou de estado
   }   
  }     // fim     while (cont == -2)

  while (cont == -3)    //loop enquanto não há movimento da catraca posição -3 (três quartos de giro para trás)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  
  
if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    if( valor1 < 300 && valor2  > 450) // verifica entradas analogicas
    {
      cont = -3;   // mantém ponto inicial
    }
    if(valor1 < 300   &&   valor2 < 300 )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
    }  
                   
    if(valor1 > 450 &&   valor2 > 450  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
    }      

    if(valor1 > 450 &&   valor2 < 300 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda para verificar possível erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  
      if(valor1 > 450 &&   valor2 < 300 )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
        if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      } 
    }
    }     

     
    } // fim  if (tipoEntr == 1)





   if (tipoEntr == 2)
    {
       //recupera o estado do botao
    estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
    estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2

     if(estadoBtn3 == LOW && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO 
    {
      cont = -3 ;   // mantém o valor incial
    } 
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)
    { 
      cont--;   // um quarto de giro para trás
    }  
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)
    { 
      cont++;    // um quarto de giro para frente
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
      delay(10); // aguarda para verificar se existe erro
      estadoBtn3 = digitalRead(PIN_BTN3);  // Lê o sensor 1
      estadoBtn4 = digitalRead(PIN_BTN4);  // Lê o sensor 2 
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW) 
      {
         if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      } 
      } 
    }
  } // fim  if (tipoEntr == 2)

  

    
    if(cont != -3)
   {
    delay(25); // delay se mudou de estado
   }
  }    // fim     while (cont == -3)


  if (cont == -4)    // posição -4 (giro completo para tras)
  { 

      direcao = 2;
   //Serial.end();
  //pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
  //digitalWrite(pinoLedInterno, LOW);
  //delay(400); 
  //digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
    //delay(100);

if(ledaux < 3)
{
  ledaux = ledaux + 1;
   digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   // determina timesamp
   ep =  rtc.getEpoch();   // carrega o epoch time em ep
   if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não consegiu atualizar o epoch time)
    {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
    }
    if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
    {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
    }
    cont = 0 ;    // reseta contagem de giro
    flagM = 1;   // habilita flag para msgID
    flagS = 1;   // habilita flag para CounterS
     Serial.println("\nEnviando saída... "); // imprime mensagem na porta serial 

    sentido = 2;      // define o sentido como saída           
    resposta = "111";      // altera resposta para aguardar resposta 000 do coordenador indicando recebimento        
    atraso = "0";   // sem atraso
    
    sendMessage();   // chama função que envia o pacote
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     flagM = 0;   // desabilita flag para msgID
    flagS = 0;  // desabilita flag para CounterS
    delay(25);
  }     // fim     while (cont == -4)

  if (cont < -4 || cont > 4)   // em caso de erro e por algum motivo não contou a volta corretamente
  {  

  
  
 //Serial.end();
 //pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
 // digitalWrite(pinoLedInterno, LOW);
  //delay(400); 
 // digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
    //delay(100);

if(ledaux < 3)
{
  ledaux = ledaux + 1;
 digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    // determina timesamp
    ep =  rtc.getEpoch();   // carrega o epoch time em ep
    if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não consegiu atualizar o epoch time)
    {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
    }
    if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
    {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
    }
    cont = 0 ;    // reseta contagem de giro   
    flagM = 1;    // habilita flag para msgID
    flagW = 1;    // habilita flag para CounterW
     Serial.println("\nEnviando erro na contagem... "); // imprime mensagem na porta serial 
    sentido = 3;    // define sentdio como erro
    resposta = "111";    // altera resposta para aguardar resposta 000 do coordenador indicando recebimento   
    atraso = "0";   // sem atraso
    
    sendMessage();    // chama função que envia o pacote
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   
    flagM = 0;    // desabilita flag
    flagW = 0;     // desabilita flag 
    delay(25);
  }     // fim   if (cont < -4 || cont> 4)

 } // fim if (modelo == 1)

 
 else if (modelo == 2) // se o bloqueio é modelo  ascom/monetel
{


  while (cont == 0)   //loop enquanto não há movimento da catraca posição 0 (posição inicial)
  {
    
    // Esta linha deve ser adicionada EM ALGUM LUGAR do loop:
 // Pode ser no início, final, ou em algum ponto que execute frequentemente
  ArduinoOTA.handle();
    

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

    //recupera o estado do botao
    estadoBtn1 = digitalRead(PIN_BTN1);  // Lê o sensor 1
    estadoBtn2 = digitalRead(PIN_BTN2);  // Lê o sensor 2    
 

  if(estadoBtn1 == HIGH && estadoBtn2 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO
    {
      cont = 0;   // mantém estado inicial
    }    
    if(estadoBtn1 == LOW && estadoBtn2 == HIGH && (direcao == 1 ||direcao == 0) )   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO
    {
      cont++;   // meio giro para frente
    }     
    if(estadoBtn1 == HIGH && estadoBtn2 == LOW && (direcao == 2 ||direcao == 0) )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    {      
      cont--;  // meio giro para trás      
    }    
    if(estadoBtn1 == LOW && estadoBtn2 == LOW   )  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
    delay(10); // aguarda para verfica possivel erro
    //recupera o estado do botao
    estadoBtn1 = digitalRead(PIN_BTN1);  // Lê o sensor 1
    estadoBtn2 = digitalRead(PIN_BTN2);  // Lê o sensor 2  
    if(estadoBtn1 == LOW && estadoBtn2 == LOW   )  // verifica se o estado do sensor 1 e 2 mudaram ao mesmo tempo (mecanicamente deveria ser impossível)
    {
    cont = 5 ;  // algum erro na leitura dos sensores
    }
    }     

   if(cont != 0)
   {
    delay(25); // delay se mudou de estado
   }
  }    // fim     while (cont == 0) 


  if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}

  while (cont == 1)  // loop enquanto não há movimento da catraca posição 1 (1 quarto de giro para frente)
  {
    
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog    
  
  //recupera o estado do botao
    estadoBtn1 = digitalRead(PIN_BTN1);   // Lê o sensor 1
    estadoBtn2 = digitalRead(PIN_BTN2);   // Lê o sensor 2   

    if(estadoBtn1 == LOW && estadoBtn2 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO
    {
      cont = 1;   // mantém estado inicial
    }  
                    
    if(estadoBtn1 == HIGH && estadoBtn2 == HIGH  )  // verifica se o estado do sensor 2 mudou e se mudou para DESLIGADO
    { 
      cont ++ ;     // meio giro para frente
    }     
      
          
    if(cont != 1)
   {
    delay(25); // delay se mudou de estado
   }

  }     // fim   while (cont == 1)   

  
  if (cont == 2)  // posição 2 (giro completo para frente)
  { 

     direcao = 1;
    // Serial.end();
  //pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
 // digitalWrite(pinoLedInterno, LOW);
 // delay(400); 
  //digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
   // delay(100);

if(ledaux < 3)
{
  ledaux =  ledaux + 1;
 digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   // determina timesamp
   ep =  rtc.getEpoch();   // carrega o epoch time em ep
   if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não conseguiu atualizar o epoch time)
   {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
   }
   if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
   {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
   }
   cont = 0 ;    // reseta contagem de giro                          
   flagE = 1;    // habilita flag para CounterE
   Serial.println("\nEnviando entrada... "); // imprime mensagem na porta serial 
   sentido = 1;    // define sentido como entrada  
   resposta = "111";   // altera resposta para aguardar resposta 000 do coordenador indicando recebimento
   atraso = "0";   // sem atraso

   sendMessage();    // chama função que envia o pacote
   esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   
    flagM = 0;    // desabilita flag para msgID
    flagE = 0;    // desabilita flag para CounterE

    delay(25);
   
  }   // fim if (cont == 2)



  while (cont == -1)     //loop enquanto não há movimento da catraca posição -1 (1 quarto de giro para trás)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
          
   //recupera o estado do botao
   estadoBtn1 = digitalRead(PIN_BTN1);  // Lê o sensor 1
   estadoBtn2 = digitalRead(PIN_BTN2);    // Lê o sensor 2

   if(estadoBtn1 == HIGH &&   estadoBtn2 == LOW)
   { 
    cont == -1 ;   // mantém estado incial
   }  

     if(estadoBtn1 == HIGH && estadoBtn2 == HIGH )  // verifica se o estado do sensor 2 mudou e se mudou para DESLIGADO
    { 
      cont -- ;     // meio giro para trás
    }  
   
   if(cont != -1)
   {
    delay(25); // delay se mudou de estado
   }
    
  }     // fim    while (cont == -1)  


  if (cont == -2)    // posição -4 (giro completo para tras)
  {   
  direcao = 2;

 //Serial.end();
 // pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
 // digitalWrite(pinoLedInterno, LOW);
 // delay(400); 
 // digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
   // delay(100);

if(ledaux < 3)
{
  ledaux =  ledaux + 1;
 digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   // determina timesamp
   ep =  rtc.getEpoch();   // carrega o epoch time em ep
   if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não consegiu atualizar o epoch time)
    {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
    }
    if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
    {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
    }
    cont = 0 ;    // reseta contagem de giro
    flagM = 1;   // habilita flag para msgID
    flagS = 1;   // habilita flag para CounterS
     Serial.println("\nEnviando saída... "); // imprime mensagem na porta serial 

    sentido = 2;      // define o sentido como saída           
    resposta = "111";      // altera resposta para aguardar resposta 000 do coordenador indicando recebimento        
    atraso = "0";   // sem atraso
    
    sendMessage();   // chama função que envia o pacote
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     flagM = 0;   // desabilita flag para msgID
    flagS = 0;  // desabilita flag para CounterS
    delay(25);
  }     // fim     while (cont == -2)

  if (cont < -2 || cont > 2)   // em caso de erro e por algum motivo não contou a volta corretamente
  {  


   //Serial.end();
  //pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
        // Liga o LED
  //digitalWrite(pinoLedInterno, LOW);
  //delay(400); 
  //digitalWrite(pinoLedInterno, HIGH);
//delay(400);
//Serial.begin(115200);// inicia comunicação serial
    //delay(100);

if(ledaux < 3)
{
   ledaux =  ledaux + 1;
     digitalWrite(pinoLedInterno, HIGH);
Serial.begin(115200);// inicia comunicação serial
}

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    // determina timesamp
    ep =  rtc.getEpoch();   // carrega o epoch time em ep
    if ( ep  < 1672531200 ) // se o tempo é anterior a janeiro de 2023 (não consegiu atualizar o epoch time)
    {
      timestamp  = "E2" ;   // timestamp recebe mensagem de erro
    }
    if ( ep  > 1672531200 ) // se o tempo é posterior a janeiro de 2023 ( consegiu atualizar o epoch time)
    {
      timestamp  = String(ep) ;   // timestamp recebe tempo armazenado em ep
    }
    cont = 0 ;    // reseta contagem de giro   
    flagM = 1;    // habilita flag para msgID
    flagW = 1;    // habilita flag para CounterW

    Serial.println("\nEnviando erro na contagem... "); // imprime mensagem na porta serial 
    delay(10);
   
    sentido = 3;    // define sentdio como erro
    resposta = "111";    // altera resposta para aguardar resposta 000 do coordenador indicando recebimento   
    atraso = "0";   // sem atraso


    sendMessage();    // chama função que envia o pacote
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
   
    flagM = 0;    // desabilita flag
    flagW = 0;     // desabilita flag 
    delay(25);
  }     // fim   if (cont < -2 || cont > 2)

}  // fim if  (modelo == 2)

delay(25);
preferences.end();
esp_task_wdt_reset(); //Reseta o temporizador do watchdog

  verificarHorarioOTA(); //Verifica que horas são para Atualização do Sistema

}    // fim void loop()

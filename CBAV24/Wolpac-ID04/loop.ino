
// função em loop
void loop()
{   

#if DEBUG_ADC
  // ---- Modo calibracao (DEBUG_ADC = 1 no sketch principal) ----
  // A placa NAO conta e NAO envia nada: so mostra o que os sensores estao lendo.
  // Gire a catraca devagar, anote o valor de cada sensor em repouso e com ele acionado,
  // ponha esses numeros em ADC_REPOUSO / ADC_ACIONADO, volte DEBUG_ADC para 0 e regrave.
  Serial.println("
== MODO CALIBRACAO (DEBUG_ADC = 1): a placa nao esta contando ==");
  Serial.println("   limiares atuais: acionado < " + String(ADC_ACIONADO) + " | repouso > " + String(ADC_REPOUSO));
  while (true)
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    Serial.println("analogico  GPIO" + String(PIN_BTN5) + "=" + String(analogRead(PIN_BTN5))
                 + "  GPIO" + String(PIN_BTN6) + "=" + String(analogRead(PIN_BTN6))
                 + "   |  digital  GPIO" + String(PIN_BTN1) + "=" + String(digitalRead(PIN_BTN1))
                 + " GPIO" + String(PIN_BTN2) + "=" + String(digitalRead(PIN_BTN2))
                 + " GPIO" + String(PIN_BTN3) + "=" + String(digitalRead(PIN_BTN3))
                 + " GPIO" + String(PIN_BTN4) + "=" + String(digitalRead(PIN_BTN4)));
    delay(500);
  }
#endif


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
        // nada a fazer no SD: leId() ja verificou que LOGREG.csv esta vazio ou nao existe.
        // (antes montava o cartao so para apagar um arquivo vazio, ~1 montagem a mais por passagem)
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
        
                  outgoing = String(sentido) + ";" + timestamp + ";" ;   // mesmo formato do bloco flagSERV == 1: "sentido;timestamp;" (atrasado() depende disso)
                        
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

 cont0a:
  while (cont == 0)   //loop enquanto não há movimento da catraca posição 0 (posição inicial)
  {

  // catraca parada: OTA (IDE e GitHub), resumo e avisos do Telegram (ver resumo.ino)
  tarefasOciosas();

   esp_task_wdt_reset(); //Reseta o temporizador do watchdog


  if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 

       if( valor1 > ADC_REPOUSO && valor2  > ADC_REPOUSO) // verifica entradas analogicas
    {
      cont = 0;   // mantém ponto inicial
    }

      if( valor1 < ADC_ACIONADO && valor2  > ADC_REPOUSO) // verifica entradas analogicas
    {
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont1a ;
      
    }
 if( valor1> ADC_REPOUSO && valor2  < ADC_ACIONADO) // verifica entradas analogicas
    {
      cont--;  // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont1b ;
    }

    
    if( valor1 < ADC_ACIONADO && valor2 < ADC_ACIONADO)
    {
      delay(10);  //aguarda para verificar se é erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if( valor1 < ADC_ACIONADO && valor2 < ADC_ACIONADO)
    {
      if(direcao == 0 || direcao == 1)
      {
      cont = 2 ;  // pula para meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont2a ;
      }
      if(direcao == 2)
      {
      cont = -2 ;  // pula para meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont2b ;
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
      delay(25); // delay se mudou de estado
      goto cont1a ;
    }     
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    {
      cont--;  // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont1b ;
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
      delay(25); // delay se mudou de estado
      goto cont2a ;
      }
      if(direcao == 2)
      {
      cont = -2 ;  // pula para meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont2b ;
      }
    }  
    }
  }   // fim if (tipoEntr == 2)



  

   //if(cont != 0)
   //{
    //delay(25); // delay se mudou de estado
  // }
}    // fim     while (cont == 0) 





 cont1a: 
 while (cont == 1)  // loop enquanto não há movimento da catraca posição 1 (1 quarto de giro para frente)
  { 
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
 
   if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}
    
     esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     
   
    if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

      if( valor1 < ADC_ACIONADO && valor2  > ADC_REPOUSO) // verifica entradas analogicas
    {
      cont = 1;   // mantém ponto inicial
    }
    if(valor1 < ADC_ACIONADO  &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
      goto cont2a ;
    }  
                   
    if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont0a ;
    }      

    if(valor1 > ADC_REPOUSO &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // delay para verificar possivel erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 > ADC_REPOUSO &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
        goto cont3a ;
      }
      if(direcao == 2)
      {
      cont = -1 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont1b ;
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
        delay(25); // delay se mudou de estado
      goto cont2a ;
    }                    
    if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont0a ;
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
        delay(25); // delay se mudou de estado
        goto cont3a ;
      }
      if(direcao == 2)
      {
      cont = -1 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont1b ;
      }
      }
    }     
  } // fim  if (tipoEntr == 2)



    

   //if(cont != 1)
  // {
   // delay(25); // delay se mudou de estado
  // }

  }     // fim   while (cont == 1)   


cont2a: 
  while (cont == 2)    // loop enquanto não há movimento da catraca posição 2 (dois quartos de giro para frente)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog


 if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    
      if(valor1 < ADC_ACIONADO  &&   valor2 < ADC_ACIONADO) // verifica entradas analogicas
    {
      cont = 2;   // mantém ponto inicial
    }
    if(valor1 > ADC_REPOUSO  &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
      goto cont3a ;
    }  
                   
    if(valor1 < ADC_ACIONADO  &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
       goto cont1a ;
    }      

    if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda para verificar se é erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
        goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont4b ;
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
      delay(25); // delay se mudou de estado
      goto cont3a ;
    }                   
    if(estadoBtn3 == LOW && estadoBtn4 == HIGH)   // verifica se o estado do sensor 1 mudou e se mudou para LIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont1a ;
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
        delay(25); // delay se mudou de estado
        goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont4b ;
      }
      } 
    }   
  } // fim if (tipoEntr == 2)
   



    //if(cont != 2)
  // {
   // delay(25); // delay se mudou de estado
   //}
  }   // fim   while (cont == 2) 


 cont3a: 
  while (cont == 3)   // loop enquanto não há movimento da catraca posição 3 (três quartos de giro para frente)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog


 if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

     if( valor1 > ADC_REPOUSO && valor2 < ADC_ACIONADO ) // verifica entradas analogicas
    {
      cont = 3;   // mantém ponto inicial
    }
    if(valor1 > ADC_REPOUSO  &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
      goto cont4a ;
    }  
                   
    if(valor1 < ADC_ACIONADO  &&   valor2 < ADC_ACIONADO  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont2a ;
    }      

    if(valor1 < ADC_ACIONADO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda par ver se é algum erro
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
     valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
      if(valor1 < ADC_ACIONADO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
       if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
        goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont4b ;
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
      delay(25); // delay se mudou de estado
      goto cont4a ;
   }  
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont2a ;
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
        delay(25); // delay se mudou de estado
        goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
      goto cont4b ;
      }       
     }
  }
  }  // fim  if (tipoEntr == 2)
  



 //if(cont != 3)
   //{
   // delay(25); // delay se mudou de estado
   //}
    
  }    // fim    while (cont == 3)  


  cont4a: 
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


 cont1b: 
  while (cont == -1)     //loop enquanto não há movimento da catraca posição -1 (1 quarto de giro para trás)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}

if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    if( valor1 > ADC_REPOUSO && valor2  < ADC_ACIONADO) // verifica entradas analogicas
    {
      cont = -1;   // mantém ponto inicial
    }
    if(valor1 > ADC_REPOUSO  &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
      goto cont0a ;    
    }  
                   
    if(valor1 < ADC_ACIONADO  &&   valor2 < ADC_ACIONADO  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
      goto cont2b ;
    }      

    if(valor1 < ADC_ACIONADO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguardar apra verificar possível erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
       valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
        if(valor1 < ADC_ACIONADO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      if(direcao == 0 || direcao == 1)
      {
        cont = 1 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
         goto cont1a ;
      }
      if(direcao == 2)
      {
      cont = -3 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont3b ;
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
      delay(25); // delay se mudou de estado
       goto cont2b ;
   }  
   if(estadoBtn3 == HIGH && estadoBtn4 == HIGH)
    { 
      cont++;    // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont0a ;
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
        cont = 1 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
         goto cont1a ;
      }
      if(direcao == 2)
      {
      cont = -3 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont3b ;
      } 
      } 
    }
  } // fim (tipoEntr == 2)

    

    // if(cont != -1)
   //{
   // delay(25); // delay se mudou de estado
   //} 
  }     // fim    while (cont == -1)  

 cont2b: 
  while (cont == -2)   //loop enquanto não há movimento da catraca posição -2 ( dois quartos de giro para trás)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

   

if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

     if( valor1 < ADC_ACIONADO && valor2  < ADC_ACIONADO) // verifica entradas analogicas
    {
      cont = -2;   // mantém ponto inicial
    }
    if(valor1 > ADC_REPOUSO  &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont1b ;
    }  
                   
    if(valor1 < ADC_ACIONADO  &&   valor2 > ADC_REPOUSO  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
       goto cont3b ;
    }      

    if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10);  // aguarda para verificar possivel erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica) 
    if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    {
        if(direcao == 0 || direcao == 1)
      {
        cont = 4 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
         goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont4b ;
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
      delay(25); // delay se mudou de estado
       goto cont3b ;
    }  
    if(estadoBtn3 == HIGH && estadoBtn4 == LOW)
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont1b ;
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
        delay(25); // delay se mudou de estado
         goto cont4a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont4b ;
      }       } 
    }
  } // fim  if (tipoEntr == 2)

               
   
     //if(cont != -2)
  // {
   // delay(25); // delay se mudou de estado
   //}   
  }     // fim     while (cont == -2)

 cont3b: 
  while (cont == -3)    //loop enquanto não há movimento da catraca posição -3 (três quartos de giro para trás)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  
  
if (tipoEntr == 1)
    {
    //recupera o estado do botao
    valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
    valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  

    if( valor1 < ADC_ACIONADO && valor2  > ADC_REPOUSO) // verifica entradas analogicas
    {
      cont = -3;   // mantém ponto inicial
    }
    if(valor1 < ADC_ACIONADO   &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    { 
      cont++;   // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont2b ;
    }  
                   
    if(valor1 > ADC_REPOUSO &&   valor2 > ADC_REPOUSO  )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      cont--;     // um quarto de giro para trás
      delay(25); // delay se mudou de estado
       goto cont4b ;
    }      

    if(valor1 > ADC_REPOUSO &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
      delay(10); // aguarda para verificar possível erro
      valor1 = analogRead(PIN_BTN5); // Lê o sensor 5 (entrada analogica)  
      valor2 = analogRead(PIN_BTN6); // Lê o sensor 6 (entrada analogica)  
      if(valor1 > ADC_REPOUSO &&   valor2 < ADC_ACIONADO )  // verifica se o estado do sensor 1 mudou e se mudou para DESLIGADO
    { 
        if(direcao == 0 || direcao == 1)
      {
        cont = 3 ;  // pula  meia volta (possivel erro)
        delay(25); // delay se mudou de estado
         goto cont3a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont4b ;
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
      delay(25); // delay se mudou de estado
       goto cont4b ;
    }  
    if(estadoBtn3 == LOW && estadoBtn4 == LOW)
    { 
      cont++;    // um quarto de giro para frente
      delay(25); // delay se mudou de estado
       goto cont2b ;
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
        delay(25); // delay se mudou de estado
         goto cont3a ;
      }
      if(direcao == 2)
      {
      cont = -4 ;  // pula  meia volta (possivel erro)
      delay(25); // delay se mudou de estado
       goto cont4b ;
      } 
      } 
    }
  } // fim  if (tipoEntr == 2)

  

    
    //if(cont != -3)
  // {
   // delay(25); // delay se mudou de estado
   //}
  }    // fim     while (cont == -3)


 cont4b: 
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

 cont5a: 
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

 cont0c: 
  while (cont == 0)   //loop enquanto não há movimento da catraca posição 0 (posição inicial)
  {
    
    // catraca parada: OTA (IDE e GitHub), resumo e avisos do Telegram (ver resumo.ino)
  tarefasOciosas();

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
        delay(25); // delay se mudou de estado
      goto cont1c; 
    }     
    if(estadoBtn1 == HIGH && estadoBtn2 == LOW && (direcao == 2 ||direcao == 0) )  // verifica se o estado do sensor 2 mudou e se mudou para LIGADO
    {      
      cont--;  // meio giro para trás 
        delay(25); // delay se mudou de estado    
      goto cont1d;  
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
    delay(25); // delay se mudou de estado
    goto cont3c; 
    }
    }     

   //if(cont != 0)
   //{
   // delay(25); // delay se mudou de estado
  // }
  }    // fim     while (cont == 0) 




 cont1c: 
  while (cont == 1)  // loop enquanto não há movimento da catraca posição 1 (1 quarto de giro para frente)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}

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
      delay(25); // delay se mudou de estado
     goto cont2c; 
    }     
      
          
    //if(cont != 1)
  // {
   // delay(25); // delay se mudou de estado
   //}

  }     // fim   while (cont == 1)   

  
   cont2c: 
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


 cont1d: 
  while (cont == -1)     //loop enquanto não há movimento da catraca posição -1 (1 quarto de giro para trás)
  {
    if (estadoPreso()) return;   // catraca presa fora do repouso: volta ao inicio do loop()
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

      if(ledaux < 3)
{  
Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída
     // Liga o LED
  digitalWrite(pinoLedInterno, LOW);
}
    
   //recupera o estado do botao
   estadoBtn1 = digitalRead(PIN_BTN1);  // Lê o sensor 1
   estadoBtn2 = digitalRead(PIN_BTN2);    // Lê o sensor 2

   if(estadoBtn1 == HIGH &&   estadoBtn2 == LOW)
   { 
    cont = -1 ;   // mantém estado incial
   }  

     if(estadoBtn1 == HIGH && estadoBtn2 == HIGH )  // verifica se o estado do sensor 2 mudou e se mudou para DESLIGADO
    { 
      cont -- ;     // meio giro para trás
      delay(25); // delay se mudou de estado
      goto cont2c;
    }  
   
  // if(cont != -1)
   //{
   // delay(25); // delay se mudou de estado
  // }
    
  }     // fim    while (cont == -1)  


 cont2d: 
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


 cont3c: 
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


}    // fim void loop()

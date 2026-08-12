//Função que envia dados armazenados no Sd para o coordenador  
void atrasado() 
{   
   Serial.println( " Inicio do envio de atrasados 2: " + String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual

  auxatra = 0 ;
 if (flagSD == 1 ) // se há algo no SD a ser enviado ao coordenador 
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    // inicia com leitura do SD: 
    digitalWrite(SD_CS, Select);    //  SELECT (Low) SD Card SPI , sinal LOW habilita cartão SD para comunicação/edição   
    /* SD.begin() Initializes the SD library and card. This begins use of the SPI bus (digital pins 11, 12, and 13 on most Arduino boards; 50, 51, and 52 on the Mega) 
    and the chip select pin, which defaults to the hardware SS pin (pin 10 on most Arduino boards, 53 on the Mega). Note that even if you use a different 
    chip select pin, the hardware SS pin must be kept as an output or the SD library functions will not work.  */
    if (!SD.begin( SD_CS, SD_MOSI, SD_MISO, SD_CLK ))    // verifica/inicia comunicação com cartão sd
    { 
     flagF = 1; // indica ao flag que falhou a comunicação
     falhou();    // chama função de aviso de falha no SD;
     fail();    // chama a função fail, reseta flags
     digitalWrite(SD_CS, DeSelect);   //  DESELECT (high) SD Card SPI
     SD.end();     // finaliza a comunicação com cartão SD
     esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     return;            
    }    // fim if !SD.begin

    else     // se comunicação com cartão SD funcionou
    {     

       //attachInterrupt(PIN_BTN1, envio1, RISING ); 
  //attachInterrupt(PIN_BTN2, envio2, RISING ); 
      
     // interrupts();   //Habilita o interrupção no Arduino
  
     flagF = 0; // indica ao flag que iniciou a comunicação
     root = SD.open("LOGREG.csv");     // abre arquivo LOGREG.csv para leitura
     if (root)   // se comunicou corretamente com o SD              
     {                 
      
       endwait = time (NULL) + 4 ;   // tempo de espera de 4 segundos
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog    
        while (root.available())   //enquanto houver algo a ser lido no arquivo txt
        {

          esp_task_wdt_reset(); //Reseta o temporizador do watchdog
         root.seek(nextposition);  // move o ponteiro de leitura para a proxima posição
         linha = root.readStringUntil( '\n' );   // coloca na variável "linha" tudo que houver no arquivo até que haja um pulo para a proxima linha de texto 
         // decompõe linha em sentido , timestamp e msgid:
         ind1 = linha.indexOf( ';' );   // localiza o primeiro separador "; " na linha
         sentido2 = linha.substring(0 , ind1 );   // lê o digito referente ao sentido na linha de texto
         timestamp2 = linha.substring(ind1 + 1 );   // le os digitos referentes ao timestamp na linha de texto
         timestamp2.trim();    // remove espaços em branco do id lido, impedindo assim que "/n" seja enviado fazendo o texto pular para a proxima linha no coordenador
         flagSD = 2;     // flag para informar a função sendMessage que não é necessario chamar LeId
         resposta = "111";    // modifica resposta para esperar a resposta do coordenador
          atraso = "1" ; // atrasado
         sendMessage();     // envia a mensagem
         //delay(10);    // delay para dar tempo do coordenador processar a informação
         esp_task_wdt_reset(); //Reseta o temporizador do watchdog
         // if (resposta == "000" && auxatra < 51 && (time (NULL) <= endwait))  // se  recebeu resposta correta do coordenador
         if (resposta == "000" && (time (NULL) <= endwait) )  // se  recebeu resposta correta do coordenador
          {
            auxatra = auxatra + 1 ;
            nextposition = root.position();  // determina até que posição do arquivo txt foi lida 
          }
          //if (resposta != "000" || auxatra > 50 ||(time (NULL) > endwait))  // se não recebeu resposta correta do coordenador
          if (resposta != "000"  || (time (NULL) > endwait))  // se não recebeu resposta correta do coordenador
          {     
            auxatra = 0 ;
            esp_task_wdt_reset(); //Reseta o temporizador do watchdog               
            root.close();   // fecha arquivo
            digitalWrite(SD_CS, DeSelect);  //  DESELECT (high) SD Card SPI
            SD.end();   // finaliza a comunicação com cartão SD
            flagSD = 1;    // flag para informar que existe informação no SD que deve ser enviada ao coordenador
            flagP = 1;   // flag par ainformar que existe informção a ser gravada em POSITION.txt
            escreve = 1;   // seleciona apagar no cartão SD
            cardID();   // chama função cardID para apagar o arquivo POSITION.txt no cartão SD   
            escreve = 0;    // seleciona escrever no cartão SD
            cardID();   // chama função cardID para gravar o novo nextposition no arquivo POSITION.txt no cartão SD                   
            resposta = "999";  // reseta resposta     
            flagP = 0;  // reseta flagP 
              // noInterrupts(); //Desabilita o interrupção no Arduino
              cont = 0 ;
            // detachInterrupt(PIN_BTN1); 
    //detachInterrupt(PIN_BTN2); 
            return;   // sai de  void atrasado()
          }   // fim  if (resposta != "000")     
        }     // fim while (root.available())  
         cont = 0 ;                                           
      }     // fim  if (root)                                            
      SD.remove("LOGREG.csv");  // deleta do cartão SD o arquivo LOGREG.csv
      root.close();  // fecha arquivo
    }    // fim else if (!SD.begin
    digitalWrite(SD_CS, DeSelect);   //  DESELECT (high) SD Card SPI
    SD.end();   
    nextposition = 0;  // reseta nextposition
    flagSD = 0;   // flag para informar que não existe informação no SD que deve ser enviada ao coordenador// finaliza a comunicação com cartão SD
    flagP = 1;   // flag par ainformar que existe informção a ser gravada em POSITION.txt
    escreve = 1;   // seleciona apagar no cartão SD
    cardID();   // chama função cardID para apagar o arquivo POSITION.txt no cartão SD   
    escreve = 0;    // seleciona escrever no cartão SD
    cardID();  // chama função cardID para gravar o novo nextposition no arquivo POSITION.txt no cartão SD
    flagP = 0;  // reseta flagP 
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog 
  } // fim if (flagSD == 1 )
  auxatra = 0 ;
  if (counterR != 0 )  // se existe algo na memoria flash a ser enviado
  {
   resposta = "111";  // modifica resposta para esperar a resposta do coordenador
   debug = 0x06;   // debug indica informação da memoria flash sendo enviada
   sendMessage();   // envia a mensagem
   delay(50);    // delay 
   if (resposta == "000")    // se recebeu resposta do coordenador
   {
      flagR = 0 ;   // reseta flag que indica conteudo na memoria flash
    }
  }
    // noInterrupts(); //Desabilita o interrupção no Arduino
                   Serial.println( "Fim do envio de atrasados 1: " + String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual

 //  detachInterrupt(PIN_BTN1); 
   // detachInterrupt(PIN_BTN2); 
} // fim de void atrasado()        



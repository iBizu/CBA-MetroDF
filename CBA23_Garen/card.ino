//função escreve e apaga arquivo LOGREG.csv no cartão SD
void card(String stamp)
{

  
 digitalWrite(SD_CS, Select);  //  SELECT (Low) SD Card SPI , sinal LOW habilita cartão SD para comunicação/edição 
 /* SD.begin() Initializes the SD library and card. This begins use of the SPI bus (digital pins 11, 12, and 13 on most Arduino boards; 50, 51, and 52 on the Mega) 
 and the chip select pin, which defaults to the hardware SS pin (pin 10 on most Arduino boards, 53 on the Mega). Note that even if you use a different 
 chip select pin, the hardware SS pin must be kept as an output or the SD library functions will not work.  */
 if (!SD.begin( SD_CS, SD_MOSI, SD_MISO, SD_CLK ))  // verifica/inicia comunicação com cartão sd
 {
   esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    flagF = 1; // indica ao flag que falhou a comunicação
    falhou();    // chama função de aviso de falha no SD;  
    fail();    // quando implementar conexão a rede, acrescentar algum tipo de aviso ao banco de dados    
    digitalWrite(SD_CS, DeSelect);   // DESELECT (high) SD Card SPI
    SD.end();  // finaliza a comunicação com cartão SD
    return;  
  } 
  else 
  {

    if ( flagF == 1 )   // se não consegiu comunicação com o SD na ultima tentativa, atualiza os contadores
    {
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     escreve = 0 ;   // seleciona escrever no SD
     cardID();   // chama função que que cria os arquivos txt no SD (se os mesmos já não estiverem criados)
     leId();    // chama função que lê arquivos no SD, para atualizar os contadores e ID de mensagem
    }  // fim if ( flagF == 1 )   // se não consegiu comunicação com o SD
    flagF = 0; // indica ao flag que iniciou a comunicação
    root = SD.open("LOGREG.csv", FILE_WRITE);  // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "LOGREG.csv". FILE_WRITE permite ler e escrever no arquivo
    if (root)    // se o arquivo foi criado e aberto corretamente
    { 
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
      if (escreve == 0)  // se a váriavel escreve está selecionada como escrever arquivo
      { 
        root.println(stamp);   // escreve o conteúdo da váriavel packet(stamp) no arquivo selecionado em SD.open
      }
      if (escreve == 1)  // se a váriavel escreve está selecionada como apagar arquivo
      {   
        SD.remove("LOGREG.csv");    // deleta do cartão SD o arquivo selecionado dentro do parênteses
      }
      root.close();    // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
      
      if (escreve == 0)  // se a váriavel escreve está selecionada como escrever arquivo
      { 
          Serial.println("\n LogReg Gravado! "); // imprime mensagem na porta serial      
      }   
      if (escreve == 1)   // se a váriavel escreve está selecionada como apagar arquivo
      { 
        Serial.println("\n Logreg Apagado! "); // imprime mensagem na porta serial      }   
      }    
      root.close();  // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
    
    }    // fim if (root)
          
  }  // fim else if (!SD.begin
 digitalWrite(SD_CS, DeSelect);  //  DESELECT (high) SD Card SPI
 SD.end();     // finaliza a comunicação com cartão SD
 esp_task_wdt_reset(); //Reseta o temporizador do watchdog
}   // fim void card

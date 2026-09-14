                       //função escreve e apaga arquivos MSGID.txt, ENTRADAS.txt, SAIDAS.txt e ERROS.txt no cartão SD
void cardID()
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
    fail();    // chama a função fail, reseta flags    
    digitalWrite(SD_CS, DeSelect);  // DESELECT (high) SD Card SPI
    SD.end();   // finaliza a comunicação com cartão SD
    return;   
  } 
  else 
 { 
  if (flagrepete2 != 0) // se não exibiu a mensagem ainda
    {
     Serial.println("\nSD Ok! "); // imprime mensagem na porta serial 
      flagrepete2 = 0;    
    }
   flagF = 0; // indica ao flag que iniciou a comunicação
   
   
    if(flagE == 2)   // se está habilitado para criar ENTRADAS.txt
    {
     root = SD.open("ENTRADAS.txt", FILE_WRITE);   // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "ENTRADAS.txt". FILE_WRITE permite ler e escrever no arquivo
     if (root)                                  // se o arquivo foi criado e aberto corretamente
     {
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     }
     root.close();  // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
    }
    if(flagE == 1)   // se está habilitado para alterar o número de entradas
    {
      root = SD.open("ENTRADAS.txt", FILE_WRITE);   // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "ENTRADAS.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)   // se o arquivo foi criado e aberto corretamente
      { 
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
        if (escreve == 0)    // se a variável escreve está selecionada como escrever arquivo
        {                   
          aux2 = String(counterE);   // converte counterE em string para ser armazenado no arquivo txt
          root.println(aux2);   // escreve o conteúdo da variável msgId no arquivo selecionado em SD.open
        }   // fim if (escreve == 0) 
        if(escreve == 1)      // se a variável escreve está selecionada como apagar arquivo
        {   
          SD.remove("ENTRADAS.txt");    // deleta do cartão SD o arquivo selecionado dentro do parênteses
        }
        root.close();    // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
      }     // fim if (root)
    }
    if(flagS == 2)      // se está habilitado para criar SAIDAS.txt
    {
      root = SD.open("SAIDAS.txt", FILE_WRITE);   // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "SAIDAS.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)  // se o arquivo foi criado e aberto corretamente
      {
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
      }
      root.close();   // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
    }
    if(flagS == 1)    // se está habilitado para alterar o número de saídas
    {
      root = SD.open("SAIDAS.txt", FILE_WRITE);  // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "SAIDAS.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)   // se o arquivo foi criado e aberto corretamente
      { 
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
        if (escreve == 0)    // se a variável escreve está selecionada como escrever arquivo
        {                   
          aux2 = String(counterS);    // converte counterS em string para ser armazenado no arquivo txt
          root.println(aux2);  // escreve o conteúdo da variável msgId no arquivo selecionado em SD.open
        }      // fim  if (escreve == 0)
        if (escreve == 1)   // se a variável escreve está selecionada como apagar arquivo
        {   
          SD.remove("SAIDAS.txt");    // deleta do cartão SD o arquivo selecionado dentro do parênteses
        }
        root.close();      // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
      }    // fim if (root)
    }    // fim if(flagS == 1)
    if(flagW == 2)     // se está habilitado para criar ERROS.txt
    {
      root = SD.open("ERROS.txt", FILE_WRITE);   // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "ERROS.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)   // se o arquivo foi criado e aberto corretamente
      {
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
      }
      root.close();    // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
    }
    if(flagW == 1)   // se está habilitado para alterar o número de erros
    {
      root = SD.open("ERROS.txt", FILE_WRITE);  // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "ERROS.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)       // se o arquivo foi criado e aberto corretamente
      { 
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
       if (escreve == 0)   // se a variável escreve está selecionada como escrever arquivo
       { 
         aux2 = String(counterW);     // converte counterW em string para ser armazenado no arquivo txt
         root.println(aux2);   // escreve o conteúdo da variável msgId no arquivo selecionado em SD.open
        }    // fim if (escreve == 0)
        if (escreve == 1)   // se a variável escreve está selecionada como apagar arquivo
        {   
          SD.remove("ERROS.txt");    // deleta do cartão SD o arquivo selecionado dentro do parênteses
        }
        root.close();   // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
      }    // fim if (root)
    }  // fim if(flagW == 1)
    if(flagP == 2)   // se está habilitado para criar POSITION.txt
    {
      root = SD.open("POSITION.txt", FILE_WRITE);  // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "POSITION.txt". FILE_WRITE permite ler e escrever no arquivo
      if (root)   // se o arquivo foi criado e aberto corretamente
      {
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
      }
      root.close();  // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
    }
    if(flagP == 1)   // se está habilitado para alterar o número de erros
    {
     root = SD.open("POSITION.txt", FILE_WRITE);   // cria (ou se já estiver criado, seleciona) na raiz do sd o arquivo "POSITION.txt". FILE_WRITE permite ler e escrever no arquivo
     if (root)    // se o arquivo foi criado e aberto corretamente
     { 
       if (escreve == 0)   // se a variável escreve está selecionada como escrever arquivo
       { 
         aux2 = String(nextposition);   // converte nextposition  em string para ser armazenado no arquivo txt
         root.println(aux2);   // escreve o conteúdo da variável msgId no arquivo selecionado em SD.open
       }      // fim if (escreve == 0)
       if (escreve == 1)  // se a variável escreve está selecionada como apagar arquivo
       {   
          SD.remove("POSITION.txt");    // deleta do cartão SD o arquivo selecionado dentro do parênteses
        }
        root.close();   // close() fecha o arquivo e garante que informação escrita seja salva fisicamente no cartão SD
      }      // fim if (root)
    }   // fim if(flagP == 1)
  }  // fim else  if (!SD.begin         
  digitalWrite(SD_CS, DeSelect);     // DESELECT (high) SD Card SPI
  SD.end();    // finaliza a comunicação com cartão SD
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
}  // fim void  cardID()

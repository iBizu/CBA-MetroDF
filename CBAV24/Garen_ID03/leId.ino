// função que lê o conteudo de MSGID.txt (armazena o ID da ultima mensagem enviada), ENTRADAS.txt (armazena o numero de enrtadas), etc,  que estão armazenados no cartão SD
void leId()             
{
 


  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  digitalWrite(SD_CS, Select);    //  SELECT (Low) SD Card SPI , sinal LOW habilita cartão SD para comunicação/edição   
  /* SD.begin() Initializes the SD library and card. This begins use of the SPI bus (digital pins 11, 12, and 13 on most Arduino boards; 50, 51, and 52 on the Mega) 
  and the chip select pin, which defaults to the hardware SS pin (pin 10 on most Arduino boards, 53 on the Mega). Note that even if you use a different 
  chip select pin, the hardware SS pin must be kept as an output or the SD library functions will not work.  */
  if (!SD.begin( SD_CS, SD_MOSI, SD_MISO, SD_CLK ))    // verifica/inicia comunicação com cartão sd
  { 
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    flagF = 1; // indica ao flag que falhou a comunicação
    falhou();     // chama função de aviso de falha no SD; 
    debug = 0x05;    // mensagem enviada mas houve erro no SD
    digitalWrite(SD_CS, DeSelect);   //  DESELECT (high) SD Card SPI
    SD.end();      // finaliza a comunicação com cartão SD
    fail();     // chama a função fail, reseta flags
    return;    
  }    // fim if !SD.begin
  else     // se comunicação com cartão SD funcionou
  {  
   
   
    flagF = 0; // indica ao flag que iniciou a comunicação
   
    if(flagE == 1 || flagE == 2)  // se FlagE está marcando alteração em ENTRADAS.txt
    {
      root = SD.open("ENTRADAS.txt");   // abre arquivo ENTRADAS.txt para leitura
      if (root)   // se comunicou corretamente com o SD 
      {   

        esp_task_wdt_reset(); //Reseta o temporizador do watchdog  
        // puxa do arquivo txt o número corrente de counterE:
        linha = root.readStringUntil('\n');   // coloca na variável "linha" tudo que houver no arquivo até que haja um pulo para a proxima linha de texto 
        aux3 = linha.toInt();  // converte o que leu no arquivo txt para inteiro e transfere para a variável  aux3
        if(flagE == 1)    // se está habilitado para alterar CounterE
        {     
          counterE  = aux3 +1;    // Incrementa numero de entradas
        }       // fim if(flagE == 1) 
        if(flagE == 2)     // se está habilitado para criar arquivo "ENTRADAS.txt"
        {     
          counterE  = aux3;    // numero atual de entradas
          flagE = 0;    // reseta flagE
        }       // fim if(flagE == 2) 
      }     // fim if root
      root.close();    // fecha arquivo
    }  // fim if(flagE == 1 || flagE == 2)
    if(flagS == 1 || flagS == 2)     // se FlagS está marcando alteração em SAIDAS.txt
    {
      root = SD.open("SAIDAS.txt");    // abre arquivo SAIDAS.txt para leitura
      if (root)
      {   
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog  
        // puxa do arquivo txt o número corrente de counterS:
        linha = root.readStringUntil('\n');    // coloca na variável "linha" tudo que houver no arquivo até que haja um pulo para a proxima linha de texto 
        aux4 = linha.toInt();   // converte o que leu no arquivo txt para inteiro e transfere para a variável  aux4 
        if(flagS == 1)    // se está habilitado para alterar CounterS
        {    
          counterS  = aux4 +1;    // Incrementa numero de saídas
        }        // fim if(flagS == 1)
        if(flagS == 2)     // se está habilitado para criar arquivo "SAIDAS.txt"
        {    
          counterS  = aux4;      // Incrementa numero de saídas
          flagS = 0;   // reseta flagS
        }      // fim if(flagS == 2)
      }    // fim if root
      root.close();     // fecha arquivo
    }     // fim if(flagS == 1 || flagS == 2)
    root = SD.open("ERROS.txt");   // abre arquivo ERROS.txt para leitura
    if (root)
    {    
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog 
      // puxa do arquivo txt o número corrente de counterW:
      linha = root.readStringUntil('\n');   // coloca na variável "linha" tudo que houver no arquivo até que haja um pulo para a proxima linha de texto 
      aux5 = linha.toInt();      // converte o que leu no arquivo txt para inteiro e transfere para a variavel  aux5   
      if(flagW == 1)    // se está habilitado para alterar CounterW
      {  
        counterW  = aux5 +1;   // Incrementa numero de erros
      }   // Fim if(flagW == 1)
      if(flagW == 2)  // se está habilitado para criar arquivo "ERROS.txt"
      {  
        counterW  = aux5;  // Incrementa numero de erros
        flagW = 0;    // reseta flagW
      }     // Fim if(flagW == 2)
    }     // fim if root
    root.close();   // fecha arquivo
    if(flagP == 1 || flagP == 2)   // se FlagP está marcando alteração em POSITION.txt
    {
      root = SD.open("POSITION.txt");   // abre arquivo POSITION.txt para leitura
      if (root)
      {     
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
        // puxa do arquivo txt o número corrente de nextposition:
        linha = root.readStringUntil('\n');   // coloca na variável "linha" tudo que houver no arquivo até que haja um pulo para a proxima linha de texto 
        aux6 = linha.toInt();   // converte o que leu no arquivo txt para inteiro e transfere para a variavel aux6   
        nextposition  = aux6;   // nextposition recebe posição gravada no SD
        flagP = 0;    // reseta flagP
      }   // fim if root
      root.close();    // fecha arquivo
    }       // fim if(flagP == 1 || flagP == 2)   
    // verifica se existe backlog em LOGREG.csv. Abre em modo LEITURA: com FILE_WRITE a mySD
    // posiciona o cursor no fim do arquivo e available() retornava sempre 0, o que marcava
    // flagSD = 1 em toda passagem (e chamava atrasado() com o arquivo vazio)
    root = SD.open("LOGREG.csv");   // abre LOGREG.csv para leitura (retorna arquivo invalido se nao existir)
    if (root && root.size() > 0)   // arquivo existe e tem conteudo
    {
      flagSD = 1;    // informa que ha algo no SD a ser enviado ao servidor
    }
    else
    {
      flagSD = 0;    // informa que nao ha nada no SD a ser enviado ao servidor
    }
    if (root)
    {
      root.close();
    }
 } // fim else if (!SD.begin
 digitalWrite(SD_CS, DeSelect); //  DESELECT (high) SD Card SPI
 SD.end();  // finaliza a comunicação com cartão SD
 if(flagE == 1 || flagS == 1 || flagW == 1 || flagM == 1)   // se algum flag está marcadao para escrever no SD
 { 

 

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    // apaga o(s) arquivo(s) txt antigo e escreve o(s) novo(s):
    escreve = 1;   // seleciona apagar arquvivo antigo do SD
    cardID();    // chama função que escreve/apaga o arquivo txt no SD 
    escreve = 0;  // seleciona escrever  novo arquivo no SD
    cardID();  // chama função que escreve/apaga o arquivo txt no SD
  } // fim if(flagE == 1 || flagS == 1 || flagW == 1 || flagM == 1
}  // Fim Void leId() 

// função para atualizar data e hora
void tempo()
{

  
esp_task_wdt_reset(); //Reseta o temporizador do watchdog
const char* path = "/recuperaTimestamp"; // Substitua pelo caminho da API no Web Service


if (WiFi.status() != WL_CONNECTED) // se não está conectado ao wifi
{
x = 0;
  // Conecte-se à rede Wi-Fi
  if(otaativ == 1)
  {
   WiFi.begin(ssidOTA, passwordOTA);
  }
else{
    WiFi.begin(ssid, password);
  }
   do {
    

esp_task_wdt_reset(); //Reseta o temporizador do watchdog

if (flagrepete != 0) // se não exibiu a mensagem ainda
    {

    Serial.println("\nConectando ao wifi... "); // imprime mensagem na porta serial
//delay(1000);
 
  flagrepete = 0 ;
    }

    x = x + 1;
    delay(50);
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    
    if (x == 20)
    {
      delay(1000);
      flagrepete = 1;
      esp_task_wdt_reset(); //Reseta o temporizador do watchdog
 
      Serial.println("\nFalhou ao conectar ao wifi... "); // imprime mensagem na porta serial
 delay(1000);
    

    //WiFi.disconnect(true);  // desconecta wifi, se não desconectar corre o risco de dar conflito com o lora (estava reiniciando toda vez que recebia mensagem0)
    //delay(1000);
       
    


      tentativas = tentativas + 1;
   if (tentativas == 20)
   {
    Serial.println("\nFalhou ao conectar ao wifi e atualizar data, reiniciando... "); // imprime mensagem na porta serial
    delay(5000);


    ESP.restart(); // reinica esp
   }

    if(otaativ == 1)
  {
   WiFi.begin(ssidOTA, passwordOTA);
  }
else{
    WiFi.begin(ssid, password);
  }
    }

  } while (WiFi.status() != WL_CONNECTED);
  flagrepete = 1;
} // fim if (WiFi.status() != WL_CONNECTED) /



  if (WiFi.status() == WL_CONNECTED)
  {
 

 esp_task_wdt_reset(); //Reseta o temporizador do watchdog
 
  Serial.println("\nConectado ao wifi, enviando requisição "); // imprime mensagem na porta serial
  //delay(1000);
   tentativas = 0 ; // reseta tentativas
  // se conectou no wifi, atualiza o timestamp:
  // Crie um cliente HTTP
  HTTPClient http;
  



// Defina o método HTTP (GET, POST, etc.)
  http.begin(host, port, path);
  http.addHeader("Content-Type", "application/json"); // Defina o tipo de conteúdo


  x = 0;

  while ( atualizatempo == 0 )
  {
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  // Envie a requisição HTTP
  int httpCode = http.GET();

  // Verifique se a requisição foi bem-sucedida
  if (httpCode == HTTP_CODE_OK) {
    // Obtenha a resposta do servidor
    String payload = http.getString();
   

esp_task_wdt_reset(); //Reseta o temporizador do watchdog


    

  
   delay(100);
  

    //separa dados do payload
    ind1 = payload.indexOf(':');    // localiza o primeiro separador "; " no pacote
    ind2 = payload.indexOf('}', ind1 + 1 );    // localiza o segundo separador "; " no pacote
    recebetempo = payload.substring(ind1 + 1, ind2 );    // localiza o segundo separador "; " no pacote
    atualizatempo = recebetempo.toInt();
    rtc.setTime(atualizatempo); 



Serial.println("\nRequisição ok, data aualizada! "); // imprime mensagem na porta serial



   //delay(1000);
  

  } 
  
  else {

flagrepete = 1;
    esp_task_wdt_reset(); //Reseta o temporizador do watchdog

    if (flagrepete != 0) // se não exibiu a mensagem ainda
    {

if (httpCode == -1) 
{
  errormes =  "CONNECTION_FAILED" ;
}
if (httpCode == -2) 
{
  errormes =  "SEND_HEADER_FAILED" ;
}

if (httpCode == -3) 
{
  errormes =  "SEND_PAYLOAD_FAILED" ;
}

if (httpCode == -4) 
{
  errormes =  "NOT_CONNECTED" ;
}

if (httpCode == -5) 
{
  errormes =  "CONNECTION_LOST " ;
}

if (httpCode == -6) 
{
  errormes =  "NO_STREAM" ;
}

if (httpCode == -7) 
{
  errormes =  "NO_HTTP_SERVER" ;
}

if (httpCode == -8) 
{
  errormes =  "TOO_LESS_RAM" ;
}

if (httpCode == -9) 
{
  errormes =  "ERROR_ENCODING " ;
}

if (httpCode == -10) 
{
  errormes =  "STREAM_WRITE" ;
}

if (httpCode == -11) 
{
  errormes =  "READ_TIMEOUT " ;
}


    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
     Serial.println("\nErro na requisição, falha no envio de dados. Código de erro: " + String(httpCode)); // imprime mensagem na porta serial

 Serial.println("\nDescrição de erro: " + String(errormes)); // imprime mensagem na porta serial


 tentativas = tentativas + 1;
   if (tentativas == 20)
   {

    Serial.println("\nFalhou ao enviar requisição e atualizar data, reiniciando... "); // imprime mensagem na porta serial
    delay(5000);

    ESP.restart(); // reinica esp
   }


 

  flagrepete = 0; 
    }
    
  }

  }
 flagrepete = 1;
  // Libere o cliente HTTP
 // http.stop();  // dava erro ao compilar
  http.end();  // achei no exemplo da biblioteca no github
  }

esp_task_wdt_reset(); //Reseta o temporizador do watchdog
}
// função para enviar informação para o banco de dados
void servidor()
{  

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    const char* path = "/enviarContagem/"; // Substitua pelo caminho da API no Web Service
    serv = path + outgoing;

    // ⭐⭐ MODIFICAÇÃO: Variáveis para controle de timeout ⭐⭐
    unsigned long startTime = millis();
    bool requestCompleted = false;
    const unsigned long TIMEOUT_TOTAL = 800; // Timeout total de 800ms
    const unsigned long TIMEOUT_REQUEST = 400; // Timeout por tentativa de 500ms

    //serv = "/enviarMedicao/255;1;1716579000;1716579090;0;0;100";

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
                flagrepete = 0 ; // varaivel auxiliar apra evitar repetições de mensagens na tela (0 mensagem já foi exibida , 1 mensagem ainda não foi exibida)
            }

            //delay(100);
            x = x + 1;
            esp_task_wdt_reset(); //Reseta o temporizador do watchdog
            
            if (x == 2)
            { 
                flagrepete = 1;
                esp_task_wdt_reset(); //Reseta o temporizador do watchdog     
                Serial.println("\nFalhou ao conectar ao wifi, falha no envio dos dados "); // imprime mensagem na porta serial

                //delay(500);
                //WiFi.disconnect(true);  // desconecta wifi, se não desconectar corre o risco de dar conflito com o lora (estava reiniciando toda vez que recebia mensagem0)
                resposta = "222";
                flagSERV = 1;  // flag informa que falhou ao enviar dados
                pacote =  String(id) +  ';' + timestamp + ';' +  '1' + ';' + SDERRO + ';' + sentido;   // cria linha que vai ser gravada no SD  
                flagrepete = 1;
                return;
            }
        } while (WiFi.status() != WL_CONNECTED);
        flagrepete = 1;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
        Serial.println("\nConectado ao wifi! Enviando dados... "); // imprime mensagem na porta serial

        // se conectou no wifi, envia dados:
        // Crie um cliente HTTP
        HTTPClient http;
        esp_task_wdt_reset();

        // ⭐⭐ MODIFICAÇÃO: Configurar timeouts na requisição HTTP ⭐⭐
        http.begin(host, port, serv);
        http.addHeader("Content-Type", "application/json"); // Defina o tipo de conteúdo
        http.setTimeout(350); // ⭐⭐ NOVO: Timeout de 500ms para a requisição ⭐⭐
        http.setConnectTimeout(700); // ⭐⭐ NOVO: Timeout de 1s para conexão ⭐⭐

        // ⭐⭐ MODIFICAÇÃO: Substituição do loop while original por sistema com timeout ⭐⭐
        // CÓDIGO ORIGINAL COMENTADO:
        /*
        x = 0;
        atualizatempo = 0;
        while (x < 1 && atualizatempo == 0 )
        {
            esp_task_wdt_reset(); //Reseta o temporizador do watchdog
            // Envie a requisição HTTP
            int httpCode = http.GET();

            // Verifique se a requisição foi bem-sucedida
            if (httpCode == HTTP_CODE_OK) {
                // Obtenha a resposta do servidor
                String payload = http.getString();

                Serial.println("\nRequisição ok, dados enviados "); // imprime mensagem na porta serial
                debug = 0x03;
                resposta = "000";
                flagSERV = 0;  // flag confirma que enviou dados

                //separa dados do payload
                ind1 = payload.indexOf(':');    // localiza o primeiro separador "; " no pacote
                ind2 = payload.indexOf('}', ind1 + 1 );    // localiza o segundo separador "; " no pacote
                recebetempo = payload.substring(ind1 + 1, ind2 );    // localiza o segundo separador "; " no pacote
                atualizatempo = recebetempo.toInt();
                rtc.setTime(atualizatempo);
            } else {
                if (flagrepete != 0) {
                    // ... código de tratamento de erro ...
                }
                flagSERV = 1;
            }
            x++;
            esp_task_wdt_reset();
        }
        */

        // ⭐⭐ NOVO CÓDIGO COM TIMEOUT ⭐⭐
        atualizatempo = 0;
        while (!requestCompleted && (millis() - startTime < TIMEOUT_TOTAL)) {
            esp_task_wdt_reset(); //Reseta o temporizador do watchdog
            
            // Envie a requisição HTTP
            int httpCode = http.GET();

            // Verifique se a requisição foi bem-sucedida
            if (httpCode > 0) {
                // Requisição completada (com sucesso ou erro HTTP)
                requestCompleted = true;
                
                if (httpCode == HTTP_CODE_OK) {



                    // Obtenha a resposta do servidor
                    String payload = http.getString();

                    Serial.println("\nRequisição ok, dados enviados: " + String(serv)); // imprime mensagem na porta serial
              
                    debug = 0x03;
                    resposta = "000";
                    flagSERV = 0;  // flag confirma que enviou dados

                    //separa dados do payload
                    ind1 = payload.indexOf(':');    // localiza o primeiro separador "; " no pacote
                    ind2 = payload.indexOf('}', ind1 + 1 );    // localiza o segundo separador "; " no pacote
                    recebetempo = payload.substring(ind1 + 1, ind2 );    // localiza o segundo separador "; " no pacote
                    atualizatempo = recebetempo.toInt();
                    rtc.setTime(atualizatempo);
                } else {
                    // ⭐⭐ MODIFICAÇÃO: Tratamento de erro com timeout ⭐⭐
                    if (flagrepete != 0) {
                        if (httpCode == -1) errormes = "CONNECTION_FAILED";
                        if (httpCode == -2) errormes = "SEND_HEADER_FAILED";
                        if (httpCode == -3) errormes = "SEND_PAYLOAD_FAILED";
                        if (httpCode == -4) errormes = "NOT_CONNECTED";
                        if (httpCode == -5) errormes = "CONNECTION_LOST";
                        if (httpCode == -6) errormes = "NO_STREAM";
                        if (httpCode == -7) errormes = "NO_HTTP_SERVER";
                        if (httpCode == -8) errormes = "TOO_LESS_RAM";
                        if (httpCode == -9) errormes = "ERROR_ENCODING";
                        if (httpCode == -10) errormes = "STREAM_WRITE";
                        if (httpCode == -11) errormes = "READ_TIMEOUT";

                        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
                        Serial.println("\nErro na requisição, falha no envio de dados. Código de erro: " + String(httpCode));
                        Serial.println("\nDescrição de erro: " + String(errormes));
                        flagrepete = 0;
                    }
                    flagSERV = 1;
                }
            } else {
                // ⭐⭐ NOVO: Se a requisição está demorando muito, cancela ⭐⭐
                if (millis() - startTime > TIMEOUT_REQUEST) {
                    Serial.println("\n⏰ Timeout - Requisição cancelada para não perder contagem");
                    requestCompleted = true;
                    resposta = "222";
                    flagSERV = 1;
                } else {
                    // ⭐⭐ NOVO: Pequeno delay antes de tentar novamente ⭐⭐
                    delay(30);
                }
            }
            esp_task_wdt_reset();
        }

        // ⭐⭐ NOVO: Verificação final de timeout crítico ⭐⭐
        if (!requestCompleted) {
            Serial.println("\n⏰⏰ Timeout crítico - Cancelando requisição");
            //flagSERV = 1;
            //resposta = "222";
            
        }

        if (flagSERV == 1) {
            pacote =  String(id) +  ';' + timestamp + ';' +  '1' + ';' + SDERRO + ';' + sentido;   // cria linha que vai ser gravada no SD  
        }

        flagrepete = 1;
        // Libere o cliente HTTP
        // http.stop();  // dava erro ao compilar
        http.end();  // achei no exemplo da biblioteca no github
        esp_task_wdt_reset();
    }

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
}
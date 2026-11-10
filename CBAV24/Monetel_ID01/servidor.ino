// função para enviar informação para o banco de dados
void servidor()
{  

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
    const char* path = "/enviarContagem/"; // Substitua pelo caminho da API no Web Service
    serv = path + outgoing;

    //Variáveis para controle de timeout
    unsigned long startTime = millis();
    bool requestCompleted = false;
    // Orcamento de tempo do envio. A contagem e BLOQUEANTE: nada e contado enquanto o servidor()
    // fala com o servidor, por isso estes valores sao curtos de proposito. Foram de 800/400 para
    // 600/300 em 10/11/2026, para encurtar a janela cega.
    //
    // ATENCAO ao que eles NAO fazem: os dois sao conferidos apenas ENTRE tentativas, nunca durante
    // um http.GET() em andamento. Quem limita uma tentativa isolada e o setConnectTimeout(700) com
    // o setTimeout(350), mais abaixo. Logo:
    //   - servidor que conecta e nao responde: o REQUEST corta a segunda tentativa, e isso encurta
    //     a janela cega de ~770 ms para ~370 ms. E aqui que esta todo o ganho destes dois numeros.
    //   - servidor inalcancavel: quem manda e o setConnectTimeout, nao estes dois, e ele continua
    //     em 700 ms de proposito -- ver o comentario la.
    const unsigned long TIMEOUT_TOTAL = 600;
    const unsigned long TIMEOUT_REQUEST = 300;

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

        //Configurar timeouts na requisição HTTP
        http.begin(host, port, serv);
        http.addHeader("Content-Type", "application/json"); // Defina o tipo de conteúdo
        http.setTimeout(350); //Timeout de leitura da resposta
        // Limite do aperto de mao do TCP, e o que REALMENTE limita uma tentativa isolada: os
        // TIMEOUT_TOTAL/REQUEST do inicio desta funcao nao interrompem um http.GET() em andamento,
        // este valor sim. Logo, e o unico que vale quando o servidor esta inalcancavel.
        //
        // FOI BAIXADO PARA 400 ms EM 10/11/2026 E DESFEITO ANTES DE PUBLICAR. Nao chegou a nenhuma
        // placa. O motivo de desfazer: a estacao mede -82 dBm, e num enlace ruim o aperto de mao
        // passa de 400 ms com facilidade. Quando passa, TODO envio falha e toda passagem vai para o
        // LOGREG.csv. Contagem nao se perderia -- o atrasado() reentrega e o servidor deduplica por
        // numid + timestamp, com o horario do momento da passagem -- mas o backlog pararia de
        // drenar enquanto durasse, porque o atrasado() tambem precisa conectar: o BI ficaria sem
        // dado em tempo real e o dia inteiro passaria a depender do cartao, que e a corrente mais
        // fraca numa placa com leitor marginal (ver o incidente do Garen de 23/10/2026).
        // Em resumo: 700 ms de janela cega de vez em quando e melhor que falhar sempre.
        // Se um dia valer tentar de novo, o teste tem de ser numa placa so, com o resumo do
        // Telegram a vista -- e nao nas tres da estacao de uma vez, que e o que o canal latest faz.
        http.setConnectTimeout(700);

        //NOVO CÓDIGO COM TIMEOUT
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
                    // So aceita um epoch plausivel, como tempo() ja fazia. Sem esta checagem uma
                    // resposta truncada ou inesperada zerava o relogio da placa (rtc.setTime(0) = 1970),
                    // e dai em diante toda passagem sairia com timestamp "E2". Como o servidor
                    // deduplica por id + data de medicao, todas elas colapsariam num unico registro:
                    // o movimento do dia sumiria do BI sem nenhum erro aparente.
                    long epochRecebido = epochSegundos(recebetempo);
                    if (epochRecebido > 0)
                    {
                        atualizatempo = epochRecebido;
                        rtc.setTime(epochRecebido);
                    }
                    else
                    {
                        Serial.println("[Servidor] Resposta sem epoch valido, relogio mantido: " + payload.substring(0, 60));
                    }
                } else {
                    // Guarda o codigo para quem chamou decidir o que fazer: atrasado() precisa
                    // distinguir "o servidor recusou esta linha" (4xx, nao adianta repetir) de
                    // "nao consegui falar com o servidor" (codigo negativo, vale tentar depois).
                    ultimoHttpCode = httpCode;
                    if (flagrepete != 0) {
                        esp_task_wdt_reset(); //Reseta o temporizador do watchdog
                        Serial.println("\nErro na requisicao, falha no envio de dados. Codigo: " + String(httpCode)
                                     + " (" + descricaoErroHttp(httpCode) + ")");
                        Serial.println("Pacote que falhou: " + String(serv));   // sem isto nao da para saber o que o servidor recusou
                        flagrepete = 0;
                    }
                    flagSERV = 1;
                }
            } else {
                //Se a requisição está demorando muito, cancela
                if (millis() - startTime > TIMEOUT_REQUEST) {
                    Serial.println("\n⏰ Timeout - Requisição cancelada para não perder contagem");
                    requestCompleted = true;
                    resposta = "222";
                    flagSERV = 1;
                } else {
                    //Pequeno delay antes de tentar novamente
                    delay(30);
                }
            }
            esp_task_wdt_reset();
        }

        //Verificação final de timeout crítico: sem isto flagSERV ficava com o valor da
        //passagem anterior e, se fosse 0, esta contagem era descartada sem ir para o SD
        if (!requestCompleted) {
            Serial.println("\n⏰⏰ Timeout crítico - Cancelando requisição, contagem vai para o SD");
            resposta = "222";
            flagSERV = 1;
        }

        if (flagSERV == 1) {
            pacote =  String(id) +  ';' + timestamp + ';' +  '1' + ';' + SDERRO + ';' + sentido;   // cria linha que vai ser gravada no SD  
        }

        flagrepete = 1;
        // Libere o cliente HTTP
        http.end();  // achei no exemplo da biblioteca no github
        esp_task_wdt_reset();
    }

    esp_task_wdt_reset(); //Reseta o temporizador do watchdog
}

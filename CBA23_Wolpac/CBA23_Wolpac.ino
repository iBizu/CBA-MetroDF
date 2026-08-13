#include <mySD.h>     // biblioteca para utilizar cartão sd  // SD Card (VMA304)
#include <time.h>     // biblioteca para trabalhar com medição de tempo tempo em C
#include <Preferences.h>   // biblioteca de preferencias permite armazenar dados na memoria flash (não apagam nem com reset nem com falta de energia)(regrava  100.000 vezes apenas antes de queimar a memoria flash)
#include <ESP32Time.h>      // biblioteca para trabalhar com medição de tempo em placa esp32
#include <esp_task_wdt.h> //Biblioteca do watchdog timer
#include <esp_now.h>  // biblioteca para o protocolo de comunicação ESP-NOW
#include <esp_wifi.h> // esp32 Wifi support
#include "SPI.h"
#include <WiFi.h>  // arduino Wifi support
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <nvs_flash.h> // Necessário para nvs_flash_erase()
#include <ArduinoOTA.h>


//VARIÁVEIS EDITÁVEIS: seus valores devem ser editados manualmente para cada contador conforme sua id, estação, etc
int  numid = 4 ;  // id do dispositvo
int tipoEntr = 1 ; // 1 - divisor de tensão (garen, 2 fios), 2 - Zener (Foca, switch, dois fios), Ascom/Monetel é indiferente a esta variavel
int teste = 1;  // (0 para funcionamento definitivo, vai funcionar conforme modelo habilitado fisicamente pelos pinos , 1 - para testes, vai ignorar a definição dos pinos e usar o modelo definido na váriavel abaixo
int modelo = 1; // modelo de bloqueio: 1 - Foca , garem ou wolpac,  2 - Ascom/Monetel, 0 - indefinido
const char *nomeota = "Wolpac-ID04"; // define nome que vai aparecer na IDE do arduino ao tentar programar via OTA  // "Monetel-ID01" "Foca-ID02" "Garen-ID03"  "Wolpac-ID04"
//dados para conectar ao wi-fi (estação, final):
const char* ssid = "WIFI-ARHD"; // Substitua por seu SSID
const char* password = "mfr-09720"; // Substitua por sua senha Wi-Fi
const char* host = "wsserver02-prod.metro.gdfnet.df"; // Substitua pelo endereço do Web Service
const int port = 9668; // Substitua pela porta do Web Service (geralmente 80 ou 443)



/*
//dados para conectar ao wi-fi (escola metroviaria, final):
const char* ssid = "WIFI-ARHD"; // Substitua por seu SSID
const char* password = "mfr-09720"; // Substitua por sua senha Wi-Fi
const char* host = "wsserver02-prod.metro.gdfnet.df"; // Substitua pelo endereço do Web Service
const int port = 9668; // Substitua pela porta do Web Service (geralmente 80 ou 443)


//dados para conectar ao wi-fi (escola metroviaria, teste):
const char* ssid = "WIFI-ARHD"; // Substitua por seu SSID
const char* password = "mfr-09720"; // Substitua por sua senha Wi-Fi
const char* host = "10.66.24.196"; // Substitua pelo endereço do Web Service
const int port = 9668; // Substitua pela porta do Web Service (geralmente 80 ou 443)

//dados para conectar ao wi-fi (estação, teste):
const char* ssid = "POC_MANUTENCAO"; // Substitua por seu SSID
const char* password = "joVzu79RgOx8@"; // Substitua por sua senha Wi-Fi
const char* host = "10.66.24.196"; // Substitua pelo endereço do Web Service
const int port = 9668; // Substitua pela porta do Web Service (geralmente 80 ou 443)
 
//dados para conectar ao wi-fi (estação, final):
const char* ssid = "POC_MANUTENCAO"; // Substitua por seu SSID
const char* password = "joVzu79RgOx8@"; // Substitua por sua senha Wi-Fi
const char* host = "wsserver02-prod.metro.gdfnet.df"; // Substitua pelo endereço do Web Service
const int port = 9668; // Substitua pela porta do Web Service (geralmente 80 ou 443)
 */


// programação OTA (via wifi)
int otaativ = 0;  //variavel auxiliar para desativar ou desativar OTA manutalmente (0 desativado, 1 ativado) (1 é quando curta circuita os pinos 12 e 14)
#define PIN_BTNOTA0 0   // pino 0 saída sensor ota
#define PIN_BTNOTA1 4  // pino  4 entrada sensor ota
int estadoBtnOTA = LOW;   // estado inicial do pino 3 = 0
//dados para conectar ao wi-fi (escola metroviaria, programação OTA):
const char* ssidOTA = "WIFI-ARHD"; // Substitua por seu SSID
const char* passwordOTA = "mfr-09720"; // Substitua por sua senha Wi-Fi



//Watchdog timer
#define WDT_TIMEOUT 50000 // 1000mS = 1 second... -> Change to your requirement.
#define CONFIG_FREERTOS_NUMBER_OF_CORES 1 
// Fixes complier error “invalid conversion from ‘int’ to ‘const esp_task_wdt_config_t*'”:
esp_task_wdt_config_t twdt_config = 
 {
    .timeout_ms = WDT_TIMEOUT,
    .idle_core_mask = 0,    // Bitmask of cores
    .trigger_panic = true,
  };


ESP32Time rtc;
//ESP32Time rtc(-10800);  // offset in seconds: (0) = GMT / (3600)= GMT+1 / (-10800) = GMT-3 (fuso horario de brasilia)


//Váriaveis fixas (não devem ser editadas manualmente)
String packSize = "--";    // tamanho do pacote
String packet;       // pacote a ser recebido
String outgoing;    // pacote a ser enviado
String stamp;      // pacote a ser gravado no SD caso não haja resposta do coordenador
int escreve ;      // se escreve igual a 0: função card() escreve arquivo txt; se esceve igual a 1: função card() apaga arquivo txt
int aux1, aux3, aux4, aux5, aux6;    // variáveis auxiliares para ler e gravar arquivos txt no SD
int ind1, ind2;     // localizadores  do caractere  ";" na string
String linha,aux2;     // variáveis auxiliares para ler e gravar arquivos txt armazenados no SD
int i, j, x , y, k;       // variavel auxiliar para comando for
int ledaux = 0; // variavel axiliar para indicação de passagem através do led
String payload = "000000000"; // recebe resposta do servidor
String  recebetempo; // ajuda a separar payload
int atualizatempo = 0 ; // variavel para converter recebtempo em inteiro
int ep;  // variavel auxiliar para verificar o epoch time
String timestamp = "0000000000";   // grava o timestamp do giro da catraca
long timestampint = 0 ;     // variavel auxiliar para converter timestamp para inteiro
int sentido = 0;      // variável para armazenar o sentido de movimento da catraca  (1 para entrada, 2 para saída e 3 para erro)
String atraso = "0";  // variavel auxiliar para detectar atraso do pacote (0 para ok, 1 para atrasado)
String SDERRO = "0";  // variavel auxiliar para detectar erro no SD (0 para ok, 1 para erro)
int timein = 0; // variavel auxiliar para comparar tempo medido com tempo recebido
String serv; // variavel auxiliar para montar o  caminho API no Web Service
byte id;   // Id do aparelho (endereço hexadecimal convertido para decimal xx-y estação-apararelho)
int tentativas = 0; // tentaivas de comunicação com wifi
String errormes = "vazio" ;  // variavel exuliar para exibir mensagens de erro
int direcao = 0 ;  // variavel auxiliar par acontagem de giro da catraca (0 indefinido,  1 detectou movimento de entrada, 2 detectou movimento de saída)
  



//variaveis para enviar mensagem com atraso
String timestamp2;    // grava o timestamp do giro da catraca
String sentido2;    // variável para armazenar o sentido de movimento da catraca  (1 para entrada, 2 para saída e 3 para erro)
String msgId2;     // identifica cada mensagem com um ID diferente


String resposta = "999";               
/* resposta armazena a resposta recebida do coordenador
"999" é o default e significa que não há nada a ser enviado ou gravado no SD
"000" indica que recebeu resposta do coordenador, não é necessário gravar no SD
"111" indica que houve um giro na catraca mas não houve resposta do coordenador, por isso deve ser gravado no SD
"222" indica que o tamanho da mensagem recebida está errado (não bate com o tamanho indicado no cabeçalho)
"333" indica que o coordenador não conseguiu gravar no SD
"444"  indica que o endereço de destino da mensagem recebida não é o endereço deste contador
*/


// variaveis auxiliares para ler e gravar informação no cartão SD
int msgId = 0;     // identifica cada mensagem com um ID diferente
int counterE = 0;   // variavel auxiliar para contagem de entradas
int counterS = 0;   // variavel auxiliar para contagem de saidas
int counterW = 0;   // variavel auxiliar para contagem de erros
int counterR = 0;   // variavel auxiliar para contagem de erros gravados na memoria flash
int flagM = 2;     // variavel auxiliar para gravar/apagar MsgID no SD (0 para desabilitado, 1 para habilitado, 2 para criação inicial do arquivo)
int flagE = 2;     // variavel auxiliar para gravar/apagar counterE no SD (0 para desabilitado, 1 para habilitado, 2 para criação inicial do arquivo)
int flagS = 2;     // variavel auxiliar para gravar/apagar counterS no SD (0 para desabilitado, 1 para habilitado, 2 para criação inicial do arquivo)
int flagW = 2;      // variavel auxiliar para gravar/apagar counterW no SD (0 para desabilitado, 1 para habilitado, 2 para criação inicial do arquivo)
int flagP = 2;     // variavel auxiliar para gravar/apagar nextposition no SD (0 para desabilitado, 1 para habilitado, 2 para criação inicial do arquivo)
int flagSD = 0;    // variavel auxiliar para informar que há informação no SD a ser enviada para o coordenador (0 desabilitado, 1 habilitado, 2 auxiliar para a função sendMessage )
int flagF = 0;     // variavel auxiliar para informar que há falha na comunicação com o SD (0 comunicou , 1 falhou )
int flagR = 0;     // variavel auxiliar para informar que há algo na  memoria flash a ser enviado  (0 para desabilitado, 1 para habilitado)
int flagrepete = 1 ; // varaivel auxiliar para  evitar repetições de mensagens na tela (0 mensagem já foi exibida, 1 mensagem ainda não foi exibida)
int flagrepete2 = 1 ; // varaivel auxiliar para  evitar repetições de mensagens na tela (0 mensagem já foi exibida, 1 mensagem ainda não foi exibida)
int auxatra = 0; // variavel auxiliar para limitar o envio de atrasados

int flagSERV = 2;     // variavel auxiliar para verificar envio de dados ao servidor (0 para enviou, 1 para não enviou, 2 para criação inicial  ou outros usos)
String  pacote , pacote2, pacote3;  // variáveis auxiliares para ler e gravar arquivos txt armazenados no SD



unsigned long  nextposition = 0; // variavel auxiliar para gravar até que posição (qual linha) do arquivo txt já foi lida (inicia na posição 0)


//variáveis para trabalhar com tempo
time_t endwait;  // tempo final de espera do timer   
int seconds = 4;  // tempo em segundos


// código para debug das informações enviadas 
byte debug = 0x03;   
//0x01  para mensagem enviada no mesmo momento do giro da catraca
//0x02  para mensagem enviada com atraso, após o giro da catraca
//0x03  para contador acabou de ser ligado e solicita ajuste de hora e data
//0x04  para coordenador acabou de ligar e está enviando atualização de hora e data
//0x05  erro ao contador tentar comunicar com o cartão SD , e  mensagem foi enviada com msgId default 999999999 (erro E2)
//0x06  para mensagem enviada para informar contagens perdidas devido a falha simultanea no SD e na comunicação com o coordenador

//armazenam estado dos botões/sensores
int estadoBtn1 = LOW;   // estado inicial do botão 1 = 0
int estadoBtn2 = LOW;   // estado inicial do botao 2 = 0
int estadoBtn3 = LOW;   // estado inicial do botao 3 = 0
int estadoBtn4 = LOW;   // estado inicial do botao 4 = 0
int estadoMod1 = LOW;   // estado inicial do pino de habilitação 1 = 0
int estadoMod2 = LOW;   // estado inicial do pino de habilitação 2 = 0
int estadoMod3 = LOW;   // estado inicial do pino de habilitação 3 = 0
int estadoMod4 = LOW;   // estado inicial do pino de habilitação 4 = 0



//conta o  movimento de giro da catraca: 4 é um giro completo para frente, -4 é um giro completo para trás
int cont = 0 ;    // posição inicial da catraca =0


//habilita os pinos para conectar botões/sensores
#define PIN_BTN1 15     //sensor 1 do foca no pino 23
#define PIN_BTN2 2    
#define PIN_BTN3 16     //sensor 1 do foca no pino 23
#define PIN_BTN4 17    // sensor 2 do foca no pino 17
#define PIN_MOD1 32    // Pino para habilitar o modelo de operação 1
#define PIN_MOD2 33    // Pino para habilitar o modelo de operação 2
#define PIN_MOD0 25    // Pino de saída que envia o sinal para ativar os modelos de operação
const int PIN_BTN5 = 36;  // pino de entrada analogica
const int PIN_BTN6 = 39;   // pino  entrada analogica
const int pinoLedInterno = 1 ;  // Pino do led interno


// variavéis para armazenar o valor das entradas analogicas
int valor1 = 0;
int valor2 = 0;


//cartão SD
#define  SD_CLK      18    // pino 5 funciona o primeiro ciclo e depois trava, (pino 2 funciona)
#define  SD_MISO    19    // consegui mudar para o pino 22 e funcionou (19 e 22 funcionam)
#define  SD_MOSI    23    // pinos 36 a 39 não funcionam, 27 funciona o primeiro ciclo e depois trava, (pinos 13 e 21 funcionam)
#define  SD_CS      5    //funcionou também com pino 21 e 18,  (pinos 18, 21 e 39 funcionam)


/*arquivos de texto armazenados no SD:
LOGREG: armazena pacotes que não foi possivel mandar para o coordenador (sentido; timestamp; msgId)
MSGID: armazena o ID da ultima mensagem enviada
ENTRADAS: armazena total de entradas contadas
SAIDAS: armazena total de saidas contadas
ERROS: armazena total de erros de contagem de giro
POSITION: em caso de envio de mensagem com atraso, armazena a proxima posição a ser lida em LOGREG.csv 
*/


#define  Select    LOW    //  Low CS means that SPI device Selected
#define  DeSelect  HIGH   //  High CS means that SPI device Deselected


ext::File root;   // necessario para arquivo na raiz do cartão sd // inicializa arquivo "root" na memória do Esp32. Do arquivo root no esp, é transferido para o arquivo txt no SD conforme os comandos 
Preferences preferences;   // inicia instancia da biblioteca de preferencias para armazenar dados na memoria flash (necessario se for utilizar memória flash)



// Principal
void setup()
{ 

delay(2000);
   // Init Serial Monitor
   Serial.begin(115200);// inicia comunicação serial
     //Serial2.begin(115200, SERIAL_8N1, 17, 16);
    // Serial2.println("Serial2 funcionando pelo GPIO16/17!");

    

    Serial.println("\nContador inciado! "); // imprime mensagem na porta serial
    
 // Configura BOD para 3.3V (nível 8) - ATUALIZADO PARA ESP-IDF v5.x+
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 
    (8 << RTC_CNTL_DBROWN_OUT_THRES_S) |  
    (1 << RTC_CNTL_BROWN_OUT_ENA_S) | 
    (1 << RTC_CNTL_BROWN_OUT_RST_ENA_S));

    checkNVS(); // Verifica a integridade da NVS



// verifica OTA
pinMode(PIN_BTNOTA0, OUTPUT); // pino 1 como saida
pinMode(PIN_BTNOTA1, INPUT);  // pino 3 como entrada
delay(500);
digitalWrite(PIN_BTNOTA0 , HIGH); // define saída em PIN_MOD0 como alta
estadoBtnOTA = LOW;  // estado inicial do pino  3 = 0
delay(500);
estadoBtnOTA = digitalRead(PIN_BTNOTA1);  // Lê o pino
delay(500);
if(estadoBtnOTA == LOW)
{
otaativ = 0 ; // desativa ota
}
if(estadoBtnOTA == HIGH)
{
otaativ = 1 ; // ativa ota
}
delay(500);
digitalWrite(PIN_BTNOTA0 , LOW); // define saída em PIN_MOD0 como alta
delay(500);




  esp_task_wdt_deinit(); //wdt is enabled by default, so we need to 'deinit' it first
//Habilita o watchdog configurando o timeout para 120 segundos
 esp_task_wdt_init(&twdt_config); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);        
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog


  // set output pins
  pinMode(SD_CS,OUTPUT);     // pino 39 como saida para enviar o sinal que habilita/desabilita o cartão SD    
 pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_BTN2, INPUT);
  pinMode(PIN_BTN3, INPUT);
  pinMode(PIN_BTN4, INPUT);
  pinMode(PIN_MOD1, INPUT);
  pinMode(PIN_MOD2, INPUT);
  pinMode(PIN_MOD0, OUTPUT); // Pino de saída para enviar o sinal que habilita os modelos de operação
 
 digitalWrite(PIN_MOD0 , HIGH); // define saída em PIN_MOD0 como alta

estadoMod1 = digitalRead(PIN_MOD1);
estadoMod2 = digitalRead(PIN_MOD2);
//delay(1000);
digitalWrite(PIN_MOD0, LOW); // define saída em PIN_MOD0 como baixa



if (teste == 0)  // se não é teste
{
if (estadoMod1 == HIGH  && estadoMod2 == LOW  ) // verifica qual modelo está sendo habilitado
{
  modelo = 1;  // define modelo tipo foca
  Serial.println("\nModelo 1 - Foca "); // imprime mensagem na porta serial
}

else if (estadoMod1 == LOW  && estadoMod2 == HIGH ) // verifica qual modelo está sendo habilitado
{
  modelo = 2;  // define modelo tipo monetel
  Serial.println("\nModelo 2 - Ascom/monetel "); // imprime mensagem na porta serial
}

else
{
  modelo = 0;  // define erro de habilitação
  Serial.println("\nErro ao habilitar modelo, reiniciando... "); // imprime mensagem na porta serial
  delay(5000);
  ESP.restart(); // reinica esp
}
}


  i=0;     // zera variavel auxiliar i

  do     // chama função tempo varias vezes
{
       Serial.println("\nAtualizando data e hora, tentativa " + String(i + 1)); // imprime mensagem na porta serial

   tempo();     // chama função para atualizar o timestamp 

    i++;
}while (atualizatempo == 0 && i < 4) ; // se não receber resposta,  chama função tempo() mais 3 vezes antes de desistir

    //rtc.setTime(1762957650); // usado para testes
  

  Serial.println( String(rtc.getTime("%d/%m/%y   %T") ) );  // mostra timestamp atual
        // formating options  http://www.cplusplus.com/reference/ctime/strftime/      // exibe no  display

//delay (500);
 


  //busca dados no SD para atualizar o display
  escreve = 0;        // seleciona escrever no SD
  cardID();    // chama função que que cria os arquivos txt no SD (se os mesmos já não estiverem criados)
  leId();    // chama função que lê arquivos no SD, para atualizar os contadores e ID de mensagem       
 
    
  esp_task_wdt_reset(); //Reseta o temporizador do watchdog
  Serial.end();
  pinMode(pinoLedInterno, OUTPUT); // define pino do led interno como saída


 for (k = 0; k < 3; k++)
{
        // Liga o LED
  digitalWrite(pinoLedInterno, HIGH);
  delay(500); 
  digitalWrite(pinoLedInterno, LOW);
delay(500);
}
  digitalWrite(pinoLedInterno, HIGH);
 Serial.begin(115200);// inicia comunicação serial
    delay(100);



  // ---> INÍCIO DA ATUALIZAÇÃO DA ORDEM DO WI-FI <---
  Serial.println("\nConectando à rede Wi-Fi...");
  setupOTA(); // Liga o Wi-Fi e prepara o OTA Local
  
  // Liga o servidor de hora para o OTA do GitHub
  inicializarNTP(); 
// ---> FIM DA ATUALIZAÇÃO <---
 delay(100);
} // fim void setup()

 
   
                                                                                  

void setupOTA() {



  // Conexão WiFi
  WiFi.mode(WIFI_STA);
  if(otaativ == 1)
  {
   WiFi.begin(ssidOTA, passwordOTA);
  }
else{
    WiFi.begin(ssid, password);
  }

  delay(1000);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

 // Configuração básica do OTA
  ArduinoOTA.setHostname(nomeota); // Opcional: definir um nome
  //   ArduinoOTA.setHostname("esp32-ota"); // Opcional: definir um nome
  
  ArduinoOTA.onError([](ota_error_t error) {
    // Reinicia em caso de erro na atualização
    ESP.restart();
  });

  ArduinoOTA.begin();
}  // fim void setupOTA() 


    

                   
                                                                                    

        











 

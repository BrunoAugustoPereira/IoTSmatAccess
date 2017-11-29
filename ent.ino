#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <MFRC522.h>
#include <SPI.h>
 

#define TOPICO_SUBSCRIBE "SmartAccess/ReSiD/Porta1/Lock"     
#define TOPICO_PUBLISH   "SmartAccess/ReSiD/Porta1/Entrada"  
#define TOPICO_PUBLISH_CONFIR   "SmartAccess/ReSiD/Porta1/CONF" 

#define ID_MQTT  "Porta1_Entrada"     
 
//defines - mapeamento de pinos do NodeMCU
#define rele   D1      //Pino Ligado ao Rele   
#define trigPin     D0  //usado pelo Ultrassonico
#define echoPin     D8  //usado pelo Ultrassonico 
#define SS_PIN      D4         //usado pelo RFID 
#define D5    14               //parada do rfid clk
#define D6    12              //parada do rfid  miso
#define D7    13              //parada do rfid  mosi
#define RST_PIN     D2     //usado pelo RFID 


//valores e variaveis para Ultrassonico
#define ValDistPad 10
long duration;
int distance;


int cont=0;
String UltimaTag;

//Criando RFID nas posições dos pinos.
MFRC522 mfrc522(SS_PIN, RST_PIN);


// WIFI (Necessario alterar para os valores da Rede)
const char* SSID = "DarkVador"; 
const char* PASSWORD = "q1w2e3r4"; 
 
// MQTT (Necessario alterar para os valores do Broker configurado no Rasp)
const char* BROKER_MQTT = "iot.eclipse.org"; 
int BROKER_PORT = 1883; 
 
 
//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

 
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void AtuaLock(String msg);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
 
/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
    SPI.begin();
    mfrc522.PCD_Init();
    
}
 
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    
    reconectWiFi();
}
 
//Função: inicializa parâmetros de conexão MQTT(endereço do broker, porta e seta função de callback)
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
 
//Função: função de callback esta função é chamada toda vez que uma informação de um dos tópicos subescritos chega)
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    AtuaLock(msg);
}

//Função: verifica o valor recebido e atua na fechaduda.
//Recebendo 1 para Abrir a porta e 0 para não abrir
// ERROR: discutir necessidade de função para retornar ao BD que funcionario não entrou
void AtuaLock(String msg)
{ 
    //verifica se deve colocar nivel baixo de tensão na saída rele:
    if (msg.equals("1"))
    {
        digitalWrite(rele, LOW);
        while(ReadUltraSon()== ValDistPad)
        {
          cont++;
          delay(100);
          if (cont == 100)
            {
              avisarserv();
              break;
            }
        } 
        cont=0;
        digitalWrite(rele, HIGH);
    }
  
}
void avisarserv(){
  char ConteudoConvert[25];
  UltimaTag.toCharArray(ConteudoConvert,25);
  MQTT.publish(TOPICO_PUBLISH_CONFIR, ConteudoConvert);
  }
 
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT,"owntracks","mosquittoDigitalOcean")) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
 
//Função: reconecta-se ao WiFi
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
  
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. Em caso de desconexão(qualquer uma das duas), a conexão é refeita.
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
    
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}


long ReadUltraSon()
{
  //Limpa trigPin
 digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // seta trigPin em HIGH por 10 micro segundos
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // le o echoPin, retorna o tempo de viagem da onda sonora em micro segundos
  duration = pulseIn(echoPin, HIGH);
  // calcula a distancia
  distance= duration*0.034/2;
  return distance;
}

//Função: envia ao Broker o estado atual do output 
void ReadAndPublishCard(void)
{

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  //Mostra ID na serial
  Serial.print("ID da tag :");
  String conteudo= "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println("");
  char ConteudoConvert[25];
  conteudo.toUpperCase();
  conteudo.toCharArray(ConteudoConvert,25);
  MQTT.publish(TOPICO_PUBLISH, ConteudoConvert);
  Serial.println("Id enviado ao broker!");
  delay(1000);
}
 
//Função: inicializa o output em nível lógico baixo
void InitOutput(void)
{
    pinMode(rele, OUTPUT);
    pinMode(trigPin, OUTPUT); 
    pinMode(echoPin, INPUT);
    digitalWrite(rele, HIGH);          
}
 
 
//programa principal
void loop() 
{   
    //garante funcionamento das conexões WiFi e ao broker MQTT
    VerificaConexoesWiFIEMQTT();
 
    //envia o status de todos os outputs para o Broker no protocolo esperado
    ReadAndPublishCard();
 
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}

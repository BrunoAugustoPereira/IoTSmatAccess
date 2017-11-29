#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <MFRC522.h>
#include <SPI.h>
 
  
#define TOPICO_PUBLISH   "SmartAccess/ReSiD/Porta1/Saida"   

#define ID_MQTT  "Porta1_Saida"     
 
//defines - mapeamento de pinos do NodeMCU
#define D1    5       
#define SS_PIN D4        //usado pelo RFID 
#define D5    14
#define D6    12
#define D7    13
#define RST_PIN D2      //usado pelo RFID 
#define D9    3
#define D10   1




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
void VerificaConexoesWiFIEMQTT(void);

 
/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:

    initSerial();
    initWiFi();
    initMQTT();
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("passou do Setup");
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
  UltimaTag=conteudo;
  MQTT.publish(TOPICO_PUBLISH, ConteudoConvert);
  Serial.println("Id enviado ao broker!");
  delay(1000);
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

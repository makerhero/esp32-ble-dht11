/* 
 *  Programa baseado no programa original desenvolvido por Timothy Woo 
 *  Tutorial do projeto original; https://www.hackster.io/botletics/esp32-ble-android-arduino-ide-awesome-81c67d
 *  Modificado para ler dados do sensor DHT11
 */  
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <DHT.h>

#include <iostream>
#include <string>

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
const int LED = 2; // Could be different depending on the dev board. I used the DOIT ESP32 dev board.

/*
 * Definição do DHT11
 */
#define DHTPIN 23 // pino de dados do DHT11
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE);

int humidity;
int temperature;

// Veja o link seguinte se quiser gerar seus próprios UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define DHTDATA_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" 


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      Serial.println(rxValue[0]);

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
        Serial.println("*********");
      }

      // Processa o caracter recebido do aplicativo. Se for A acende o LED. B apaga o LED
      if (rxValue.find("A") != -1) { 
        Serial.println("Turning ON!");
        digitalWrite(LED, HIGH);
      }
      else if (rxValue.find("B") != -1) {
        Serial.println("Turning OFF!");
        digitalWrite(LED, LOW);
      }
    }
};

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("ESP32 DHT11"); // Give it a name

  // Configura o dispositivo como Servidor BLE
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Cria o servico UART
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Cria uma Característica BLE para envio dos dados
  pCharacteristic = pService->createCharacteristic(
                      DHTDATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  // cria uma característica BLE para recebimento dos dados
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Inicia o serviço
  pService->start();

  // Inicia a descoberta do ESP32
  pServer->getAdvertising()->start();
  Serial.println("Esperando um cliente se conectar...");
}

void loop() {
  if (deviceConnected) {

    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    // testa se retorno é valido, caso contrário algo está errado.
    if (isnan(temperature) || isnan(humidity)) 
    {
      Serial.println("Failed to read from DHT");
    }
    else 
    {
      Serial.print("Umidade: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperatura: ");
      Serial.print(temperature);
      Serial.println(" *C");
    }
    
    char humidityString[2];
    char temperatureString[2];
    dtostrf(humidity, 1, 2, humidityString);
    dtostrf(temperature, 1, 2, temperatureString);

    char dhtDataString[16];
    sprintf(dhtDataString, "%d,%d", temperature, humidity);
    
    pCharacteristic->setValue(dhtDataString);
    
    pCharacteristic->notify(); // Envia o valor para o aplicativo!
    Serial.print("*** Dado enviado: ");
    Serial.print(dhtDataString);
    Serial.println(" ***");
  }
  delay(1000);
}

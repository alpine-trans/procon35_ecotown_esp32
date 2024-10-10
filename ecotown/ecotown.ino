
// includes
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "Ultrasonic.h"


// constants
#define SERVICE_UUID "e7949142-2785-53dc-1994-1deb54135003"
#define CHARACTERISTIC_UUID "3b5122d3-4703-7453-ad58-73a8ebdafb7b"
#define BLE_DEVICE_NAME "ESP32_Bluetooth_Server"

#define RIGHT_SIG_PIN 2
#define LEFT_SIG_PIN 12


// variables
BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;
bool deviceConnected = false;
bool preDeviceConnected = false;

Ultrasonic right_ultrasonic(RIGHT_SIG_PIN);
Ultrasonic left_ultrasonic(LEFT_SIG_PIN);
long distance_to_object[2] = {0,0};
long previous_distance[2] = {0,0};
int amount_of_change[2] = {0,0};
int ditection_threshold = 5;
bool isDetected = false;



// callbacks
class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer *pServer){
    deviceConnected = true;
    Serial.println("onConnect");
  }
  void onDisconnect(BLEServer *pServer){
    deviceConnected = false;
    Serial.println("onDisconnect");
  }
};


// functions
void setup(){
  Serial.begin(115200);

  // BLEの初期化
  BLEDevice::init(BLE_DEVICE_NAME);

  // サーバー,サービス,キャラクタリスティック,コールバックの作成・設定
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_READ
  );

  // キャラクタリスティックの初期値
  pCharacteristic->setValue((uint8_t*)&isDetected,sizeof(isDetected));

  //サービスの開始
  pService->start();

  //アドバタイジングの設定
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);  //スキャンレスポンスの有効化
  pAdvertising->setMinPreferred(0x06);  //最小の接続間隔
  pAdvertising->setMaxPreferred(0x12);  //最大の接続間隔
  // アドバタイジングの開始(繰り返される)
  BLEDevice::startAdvertising();

  Serial.println("Characteristic defined");
}


void loop(){

  // disconnecting
  if(!deviceConnected && preDeviceConnected){
    delay(500);
    pServer->startAdvertising();
    Serial.println("restartAdvertising");
    preDeviceConnected = deviceConnected;
  }  
  // connecting
  if(deviceConnected && !preDeviceConnected){
    preDeviceConnected = deviceConnected;
  }


  if(deviceConnected){  // 接続後
    // 超音波センサーの設定
    distance_to_object[0] = right_ultrasonic.MeasureInCentimeters();
    distance_to_object[1] = left_ultrasonic.MeasureInCentimeters();

    for(int i = 0; i < 2; i++){
      // 検知距離の変化量を取得
      amount_of_change[i] = previous_distance[i] - distance_to_object[i];
      previous_distance[i] = distance_to_object[i];

      if(amount_of_change[i] > ditection_threshold  &&  !isDetected){  // 閾値を超え、検知した状態でないなら
        isDetected = true;
      }
      else if(amount_of_change[i] > ditection_threshold  &&  isDetected){
        isDetected = false;
      }
      // 検知結果の送信
      Serial.println(isDetected);
      pCharacteristic->setValue((uint8_t*)&isDetected,sizeof(isDetected));
      pCharacteristic->notify();
    }
  }


  // String state = pCharacteristic->getValue();
  // bool characteristic_state = (state[0]!=0);
  // Serial.println(characteristic_state);
  Serial.println(deviceConnected);
  delay(50);

}







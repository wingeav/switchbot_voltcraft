#include "Arduino.h"
#include "SwitchBotMeter.h"
std::string SwitchBotMeter::sensor="";
float SwitchBotMeter::temperature=0;
uint8_t SwitchBotMeter::humidity=0;

// Create a single global instance of the callback class to be used by all clients 
static ClientCallbacks clientCB;
	NimBLEUUID bmeServiceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
	NimBLEUUID bmeNotifyCharacteristicsUUID("cba20003-224d-11e6-9fb8-0002a5d5c51b");
	NimBLEUUID bmeWriteCharacteristicsUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");


//  None of these are required as they will be handled by the library with defaults. 
//                       Remove as you see fit for your needs                        
    void ClientCallbacks::onConnect(NimBLEClient* pClient) {
        Serial.println("ONCONNECT - Connected");
        // After connection we should change the parameters if we don't need fast response times.
        // These settings are 150ms interval, 0 latency, 450ms timout.
        // Timeout should be a multiple of the interval, minimum is 100ms.
        // I find a multiple of 3-5 * the interval works best for quick response/reconnect.
        // Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
        //pClient->updateConnParams(120,120,0,60);
        pClient->updateConnParams(60,60,0,60);
    };

    void ClientCallbacks::onDisconnect(NimBLEClient* pClient) {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
    };

    // Called when the peripheral requests a change to the connection parameters.
    // Return true to accept and apply them or false to reject and keep
    //  the currently used parameters. Default will return true.     
    bool ClientCallbacks::onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
        if(params->itvl_min < 24) { // 1.25ms units 
            return false;
        } else if(params->itvl_max > 40) { // 1.25ms units 
            return false;
        } else if(params->latency > 2) { // Number of intervals allowed to skip 
            return false;
        } else if(params->supervision_timeout > 100) { // 10ms units 
            return false;
        }

        return true;
    };


SwitchBotMeter::SwitchBotMeter() {
}

void SwitchBotMeter::InitBLE() {
    NimBLEDevice::init("");
	NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
    // Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9db 
#else
    NimBLEDevice::setPower(9); // +9db 
#endif

}


void SwitchBotMeter::ReadTemp(std::string address) {
  sensor="";
  temperature=0;
  humidity=0;
  connectAddress(*new NimBLEAddress(address,BLE_ADDR_RANDOM));
}

   
//When the BLE Server sends a new temperature reading with the notify property
void SwitchBotMeter::temperatureNotifyCallback(NimBLERemoteCharacteristic* pRemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify) {



  //store temperature value
  /*
    Byte 0: Status - 1 = OK
    Byte 1: decimal Temperatur - 0 bis 9
    Byte 2: Bits 1-7 - Temperatur in Grad Celsius
            Bit  8   - Positiv/Negativ: 0 = Negativ, 1 = Positiv
    Byte 3: Luftfeuchte
  */
  /*
    bitwise AND ausführen für Byte2
    bitwise AND bedeutet wenn eine der beiden Ziffern 0 ist, wird das Ergebnis 0, wenn beide Ziffern 1 sind, wird das Ergebnis 1
    0  0  1  1    Wert
    0  1  0  1    Operand
    ----------
    0  0  0  1    Ergebnis

    Byte2: 10011001 (bit 1 bis 7 geben die Temperatur an, bit 8 das Vorzeichen[positiv/negativ] )
    Operator: 0x7f = 01111111 (127)
    10011001
    01111111
    --------
    00011001 --> damit wird das 8. bit enfernt und man bekommt eine Zahl mit 7 Bits

    um das Vorzeichen zu bekommen, muss er Operator 0x80 = 10000000 (128) werden:
    10011001
    10000000
    --------
    10000000 --> damit den die Bits 1-7 auf 0 gesetzt bit enfernt und man bekommt eine Zahl mit 7 Bits

  */

  /*
    Serial.print("byte 2 komplett: ");
    Serial.println(pData[2],BIN);

    Serial.print("byte 2 bitwise shift 7  ");
    Serial.println(pData[2]>>7,BIN);
  */
  float temperatur = (pData[2] & 0x7f) + (float(pData[1]) / 10);
  if ((pData[2] >> 7 == 0)) { temperatur = temperatur * -1; }  // move bits 7 to right

  uint8_t luftfeuchte = pData[3];

  Serial.printf("Temperatur: %s °C\n", String(temperatur).c_str());
  Serial.printf("Luftfeuche: %s %\n", String(luftfeuchte).c_str());

  Serial.print("BLEAdresse: ");
  Serial.println(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str());

  sensor = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString();
  temperature = temperatur;
  humidity = luftfeuchte;

  Serial.println("Start unsubscribe: ");
   if (NimBLEDevice::getClientListSize()) {
    Serial.println("GetClientListSize ");
    NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    Serial.println("pClient ");
    if (pClient) {
      Serial.println("isConnected? ");
      if (pClient->isConnected()) {
        Serial.println("Ja, isConnected ");
        pClient->disconnect();
      } else {Serial.print("(unsubscribe) not connected ");}
    } else {Serial.print("(unsubscribe) no pClient ");}
  } else {Serial.print("(unsubscribe) no GetClientListSize ");}
}


bool SwitchBotMeter::connectAddress(NimBLEAddress address) {
  NimBLEClient* pClient = nullptr;

  if(NimBLEDevice::getClientListSize()) {
      Serial.println(" - RE CONNECT ...");
      pClient = NimBLEDevice::getClientByPeerAddress(address);
      Serial.println(" - RE CONNECT  2 ...");
      if(pClient){
          Serial.println(" - CLIENT VORHANDEN -- CONNECTING");
          if(!pClient->connect(address, false)) {
              Serial.println("Reconnect 2 failed");
              delay(40000);
          } else {Serial.println("Reconnected client");}
      }
      else {
          Serial.println("no Client in Reconnect");
          //pClient = NimBLEDevice::getDisconnectedClient();
      }
  }



  if (!pClient) {
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(&clientCB, false);
    // Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
    // These settings are safe for 3 clients to connect reliably, can go faster if you have less
    // connections. Timeout should be a multiple of the interval, minimum is 100ms.
    // Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
    pClient->setConnectionParams(12,12,0,51);
    // Set how long we are willing to wait for the connection to complete (seconds), default is 30. 
    pClient->setConnectTimeout(10);
    Serial.println(" - Connecting device...");
    if (pClient->connect(address)) {
      Serial.println(" - Connected to server");
    } else {
      NimBLEDevice::deleteClient(pClient);
      Serial.println(" - Failed to connect, deleted client");
      sensor = "ERROR";
      return false;
    }
  }

  if(!pClient->isConnected()) { 
      Serial.println(" - not connected");
      sensor = "ERROR";
      return false;
  }

  
	NimBLERemoteService* pRemoteService = pClient->getService(bmeServiceUUID);
    if(pRemoteService) {     // make sure it's not null 
        NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(bmeNotifyCharacteristicsUUID);
        if(pRemoteCharacteristic) {     // make sure it's not null 
        
            //  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
            //  Unsubscribe parameter defaults are: response=false.
            if(pRemoteCharacteristic->canNotify()) {
                if(!pRemoteCharacteristic->subscribe(true, temperatureNotifyCallback)) {
                    // Disconnect if subscribe failed 
                    Serial.print("Fehler beim Subscribe");
                    pClient->disconnect();
					sensor = "ERROR";
                    return false;
                } else {
					Serial.print("SUBSCRIBED SUCCESSFULLY ");
                }
            } else {
				Serial.print("Kein CanNotify ");
				sensor = "ERROR";
			}
        }

        pRemoteCharacteristic = pRemoteService->getCharacteristic(bmeWriteCharacteristicsUUID);
        if(pRemoteCharacteristic) {     // make sure it's not null 
            if(pRemoteCharacteristic->canWrite()) {
                byte reqData[3] = { 0x57, 0x0f, 0x31 };
                if(pRemoteCharacteristic->writeValue(reqData, 3, true)) {
                    Serial.print("Wrote new value to: ");
                    Serial.println(pRemoteCharacteristic->getUUID().toString().c_str());
                }
                else {
                    // Disconnect if write failed 
                    Serial.print("TRENNE canwrite 2");
                    pClient->disconnect();
					          sensor = "ERROR";
                    return false;
                }
            }
        }
    } else {
      Serial.println("Remoteservice not found.");
		  sensor = "ERROR";
    }
	return true;
}


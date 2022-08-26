#include "Arduino.h"
#include "VoltCraft.h"
float  VoltCraft::kwh[24] = {0};
std::string VoltCraft::mode="";
uint8_t VoltCraft::poweron =0;
float VoltCraft::watt=0;
uint8_t VoltCraft::volt=0;
float VoltCraft::ampere=0;
uint8_t VoltCraft::frequency=0;
float VoltCraft::consumption=0;
bool VoltCraft::measure_has_data=false;
bool VoltCraft::daily_has_data=false;


// Create a single global instance of the callback class to be used by all clients 
static ClientCallbacksVolt clientCBVolt;
NimBLEUUID bmeVoltServiceUUID("FFF0");
NimBLEUUID bmeVoltNotifyCharacteristicsUUID("FFF4");
NimBLEUUID bmeVoltWriteCharacteristicsUUID("FFF3");


//  None of these are required as they will be handled by the library with defaults. 
//                       Remove as you see fit for your needs                        
    void ClientCallbacksVolt::onConnect(NimBLEClient* pClient) {
        Serial.println("ONCONNECT - Connected");
        // After connection we should change the parameters if we don't need fast response times.
        // These settings are 150ms interval, 0 latency, 450ms timout.
        // Timeout should be a multiple of the interval, minimum is 100ms.
        // I find a multiple of 3-5 * the interval works best for quick response/reconnect.
        // Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
        //pClient->updateConnParams(120,120,0,60);
        pClient->updateConnParams(60,60,0,60);
    };

    void ClientCallbacksVolt::onDisconnect(NimBLEClient* pClient) {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
    };

    // Called when the peripheral requests a change to the connection parameters.
    // Return true to accept and apply them or false to reject and keep
    //  the currently used parameters. Default will return true.     
    bool ClientCallbacksVolt::onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
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


VoltCraft::VoltCraft() {
}

void VoltCraft::InitBLE() {
    NimBLEDevice::init("");
	NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
	NimBLEDevice::setMTU(160); // ?????
    // Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9db 
#else
    NimBLEDevice::setPower(9); // +9db 
#endif

}




   
//When the BLE Server sends a new temperature reading with the notify property
void VoltCraft::NotifyCallbackVolt(NimBLERemoteCharacteristic* pRemoteCharacteristic,
                               uint8_t* pData, size_t length, bool isNotify) {
  // Notification handle = 0x002e value: 0f 33 0a 00 00 0e 00 0e 00 0e 00 0e 00 0c 00 09 00 08 00 0b
  //                                     |  |  |     |     |     |     |     |     |     |     + Current hour - 16, 2 bytes for Wh
  //                                     |  |  |     |     |     |     |     |     |     + Current hour - 17, 2 bytes for Wh
  //                                     |  |  |     |     |     |     |     |     + Current hour - 18, 2 bytes for Wh
  //                                     |  |  |     |     |     |     |     + Current hour - 19, 2 bytes for Wh
  //                                     |  |  |     |     |     |     + Current hour - 20, 2 bytes for Wh
  //                                     |  |  |     |     |     + Current hour - 21, 2 bytes for Wh
  //                                     |  |  |     |     + Current hout - 22, 3 bytes for Wh
  //                                     |  |  |     + Current hour - 23, 2 bytes for Wh
  //                                     |  |  + 0x0a00, Request data for day request
  //                                     |  + Length of payload starting w/ next byte incl. checksum
  //                                     + static start sequence for message, 0x0f

  // Notification handle = 0x002e value: 00 0e 00 0e 00 11 00 0f 00 10 00 0f 00 0d 00 0e 00 0e 00 0e
  // Notification handle = 0x002e value: 00 0e 00 0e 00 0e 00 0e 00 0d 00 00 42 ff ff
  //                                                             |     |     |  + static end sequence of message, 0xffff
  //                                                             |     |     + checksum byte starting with length-byte, ending w/ byte before
  //                                                             |     + Current hour, 2 bytes for Wh
  //                                                             + Current hour - 2 , 2 bytes for Wh    Serial.println("CALLBACK Daily");




  //                                        +- Length of payload starting w/ next byte. -+
  //                                        |                                            |
  // Notification handle = 0x0014 value: 0f 0f 04 00 01 00 88 50 dc 00 d6 32 01 00 00 00 00 67 2a
  //                                     |     |     |  |        |  |     |        |           |  
  //                                     |     |     |  |        |  |     |        |           + checksum byte starting with length-byte
  //                                     |     |     |  |        |  |     |        + total consumption, 4 bytes (hardware versions >= 3 there is a value)
  //                                     |     |     |  |        |  |     + frequency (Hz)
  //                                     |     |     |  |        |  + Ampere/1000 (A), 2 bytes
  //                                     |     |     |  |        + Voltage (V)
  //                                     |     |     |  |
  //                                     |     |     |  + Watt/1000, 3 bytes
  //                                     |     |     + Power, 0 = off, 1 = on
  //                                     |     + Capture measurement response 0x0400
  //                                     + static start sequence for message, 0x0f
  if (pData[1] == 6) {
    Serial.println("AUTH DAILY NOTIFY");
  }

  if (pData[1] == 15) {
    Serial.println("Measure Notify");
    mode ="measure";
    poweron = pData[4];
    watt = float((pData[5] * 256 * 256) + (pData[6] * 256) + (pData[7])) / 1000; // byte 5 um 16 bit "verschieben" = 65536, byte 6 um 8 bit "verschieben" ) = 256, Byte 7 nich verschieben.... Damit werden die einzelnen Bytes zu einer Gesamtzahl -- dividiert durch 1000 ergibt die Watt (das ganze muss vor der division als float definiert werden, sonst wird die Summe als int gewertet)
    volt = pData[8];
    ampere = float((pData[9] * 256) + (pData[10])) / 1000; // byte 9 um 8 bit "verschieben" ) = 256, Byte 10 nich verschieben.... Damit werden die einzelnen Bytes zu einer Gesamtzahl -- dividiert durch 1000 ergibt die Ampere (das ganze muss vor der division als float definiert werden, sonst wird die Summe als int gewertet)
    frequency = pData[11]; // Frequenz scheint nur das erste Byte zu enthalten ohne verschieben
    consumption = float((pData[14] * 256 * 256 *256) + (pData[15] * 256 * 256) + (pData[16] * 256)  + pData[17]) / 1000;
    measure_has_data = true;
    Serial.print("Watt: ");
    printf("%.4lf\n",watt);

    Serial.print("volt: ");
    Serial.println(volt);

    Serial.print("ampere: ");
    Serial.println(ampere);

    Serial.print("frequency: ");
    Serial.println(frequency);

    Serial.print("consumption: ");
    printf("%.4lf\n",consumption);

  }


  if (pData[1] == 51 && pData[2] == 10) { // pData1 = 0x31 und pData2 = 0x0a
    Serial.println("Daily Notify");
    mode ="daily";
    int byte = 4;
    kwh[24] = {0};
    for (int d = 0; d < 24; d++)
    {
      kwh[d] = float((pData[byte] * 256) + (pData[byte+1])) / 1000;
      byte += 2;
    }
    daily_has_data = true;    
    /*int count = 23;
    for (int d = 0; d < 24; d++)
    {
      Serial.print("-" );
      Serial.print(count );
      Serial.print("h: " );
      printf("%.4lf\n",kwh[d]);
    }*/

  }
 
  for (int i = 0; i < length; i++) {
      Serial.print(pData[i],HEX);
      Serial.print(" ");
    }
    Serial.println("");
}

void VoltCraft::ReadVoltCraft(std::string address, bool daily) {
		ReadVoltCraft(*new NimBLEAddress(address,BLE_ADDR_PUBLIC),daily);
}

void VoltCraft::ReadVoltCraft(NimBLEAddress address, bool daily) {
  NimBLEClient* pClient = nullptr;
	
  if(NimBLEDevice::getClientListSize()) {
      Serial.println(" - RE CONNECT ...");
      pClient = NimBLEDevice::getClientByPeerAddress(address);
      Serial.println(" - RE CONNECT  2 ...");
      if(pClient){
          Serial.println(" - CLIENT VORHANDEN -- CONNECTING");
          if(!pClient->connect(address)) {
              Serial.println("Reconnect 2 failed");
              //delay(40000);
          } else {Serial.println("Reconnected client");}
      }
      else {
          Serial.println("no Client in Reconnect");
          //pClient = NimBLEDevice::getDisconnectedClient();
      }
  }



  if (!pClient) {
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(&clientCBVolt, false);
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
      volt = -1;
      return;
    }
  }

  if(!pClient->isConnected()) { 
      Serial.println(" - not connected");
      volt = -1;
      return;
  }

  
  Serial.println("Hole SERVICE");
  NimBLERemoteService* pRemoteService = pClient->getService(bmeVoltServiceUUID);
  if(pRemoteService) {     // make sure it's not null 
    Serial.println("Service gefunden");
    NimBLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(bmeVoltNotifyCharacteristicsUUID);
    Serial.println("Char ermittelt?");
    if(pRemoteCharacteristic) {     // make sure it's not null 
      Serial.println("Char nicht null");
      //  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
      //  Unsubscribe parameter defaults are: response=false.
      if(pRemoteCharacteristic->canNotify()) {
        Serial.println("CAN NOTIFY = OK");
        bool subscribeSucess = false ;
        
        Serial.println("VOLT SUBSCRIBE");
        subscribeSucess = pRemoteCharacteristic->subscribe(true, NotifyCallbackVolt);
        Serial.println("SUBSCRIBE erfolgreich?");
        if(!subscribeSucess) {
          // Disconnect if subscribe failed 
          Serial.print("Fehler beim Subscribe");
          pClient->disconnect();
          volt = -1;
          return;
        } else {
          Serial.print("SUBSCRIBED SUCCESSFULLY ");
        }
      } else {
        Serial.print("Kein CanNotify ");
        volt = -1;
      }
    } else {Serial.println("Char nicht ermittelt = NULL");}


    Serial.println("Hole RemoteChar");
    pRemoteCharacteristic = pRemoteService->getCharacteristic(bmeVoltWriteCharacteristicsUUID);
    if(pRemoteCharacteristic) {     // make sure it's not null 
      Serial.println("RemoteChar erfolgreich");
      if(pRemoteCharacteristic->canWriteNoResponse()) {
         Serial.println("CanWrite ist True");
        //AUTH
        //0f0c170000000000000000000018ffff
          //| | |   | |     | |       | +  static end sequence of message, 0xffff
          //| | |   | |     | |       + checksum byte starting with length-byte, ending w/ byte before
          //| | |   | |     | + always 0x00000000
          //| | |   | + PIN, 4 bytes e.g. 01020304
          //| | |   + 0x00 for authorization request
          //| | + Authorization command 0x1700
          //| + Length of payload starting w/ next byte incl. checksum
          //+ static start sequence for message, 0x0f                
        byte reqAuth[16] = { 0x0f,0x0c,0x17,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0xff,0xff };
        Serial.println("put regAuth Byte");
        if(pRemoteCharacteristic->writeValue(reqAuth, 16, false)) {
            Serial.print("Wrote new value to: ");
            Serial.println(pRemoteCharacteristic->getUUID().toString().c_str());
        }
        else {
            // Disconnect if write failed 
            Serial.println("Error WriteValue");
            pClient->disconnect();
            volt = -1;
            return;
        }

        byte reqMeasure[9];
        if (daily) {
          //0f050b0000000cffff
          //| | | |     | + static end sequence of message, 0xffff
          //| | | |     + checksum byte starting with length-byte, ending w/ byte before
          //| | | + Static 0x000000
          //| | + 0a = last 24h per hour, 0b = last 30 days per day, 0c = last year per month
          //| + Length of payload starting w/ next byte incl. checksum
          //+ static start sequence for message, 0x0f
          Serial.println("set Daily Byte?");
          
          reqMeasure[0] = 0x0f;
          reqMeasure[1] = 0x05;
          reqMeasure[2] = 0x0a;
          reqMeasure[3] = 0x00;
          reqMeasure[4] = 0x00;
          reqMeasure[5] = 0x00;
          reqMeasure[6] = 0x0b;
          reqMeasure[7] = 0xff;
          reqMeasure[8] = 0xff;
          Serial.println("Daily Byte done");
        } else {
          //0f050400000005ffff
          //| | |       | + static end sequence of message, 0xffff
          //| | |       + checksum byte starting with length-byte, ending w/ byte before
          //| | + Capture measurement command 0x040000
          //| + Length of payload starting w/ next byte incl. checksum
          //+ static start sequence for message, 0x0f
          Serial.println("set measure Byte?");
          reqMeasure[0] = 0x0f;
          reqMeasure[1] = 0x05;
          reqMeasure[2] = 0x04;
          reqMeasure[3] = 0x00;
          reqMeasure[4] = 0x00;
          reqMeasure[5] = 0x00;
          reqMeasure[6] = 0x05;
          reqMeasure[7] = 0xff;
          reqMeasure[8] = 0xff;
          //reqMeasure = { 0x0f,0x05,0x04,0x00,0x00,0x00,0x05,0xff,0xff };
          Serial.println("measure Byte done");
        }


        if(pRemoteCharacteristic->writeValue(reqMeasure, 9, false)) {
            Serial.print("Wrote new value to: ");
            Serial.println(pRemoteCharacteristic->getUUID().toString().c_str());
        }
        else {
            // Disconnect if write failed 
            Serial.println("TRENNE canwrite 2");
            pClient->disconnect();
            volt = -1;
            return;
        }
      } else {Serial.println("no CANWRITE");}
    }
  } else {
    Serial.println("Remoteservice not found.");
    volt = -1;
  }
	return;
}


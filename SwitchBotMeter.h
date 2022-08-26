#ifndef SwitchBotMeter_H
#define SwitchBotMeter_H

#include "Arduino.h"
#include <NimBLEDevice.h>

class ClientCallbacks  : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient);
  void onDisconnect(NimBLEClient* pClient);
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params);

};


class SwitchBotMeter {
    public:
        SwitchBotMeter();
        void InitBLE();
        void ReadTemp(std::string address);

        static std::string sensor;
        static float temperature;
        static uint8_t humidity;
    private:
        int scanTime = 5;  //In seconds
        static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData, size_t length, bool isNotify);
        bool connectAddress(NimBLEAddress address);
    
};
#endif
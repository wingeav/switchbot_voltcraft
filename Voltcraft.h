#ifndef VoltCraft_H
#define VoltCraft_H

#include "Arduino.h"
#include <NimBLEDevice.h>

class ClientCallbacksVolt  : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient);
  void onDisconnect(NimBLEClient* pClient);
  bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params);

};


class VoltCraft {
    public:
        VoltCraft();
        void InitBLE();
        void ReadVoltCraft(std::string address, bool daily = false); // public call with MAC Adresse as String
        void ReadVoltCraft(NimBLEAddress address, bool daily = false); //called internally

        static std::string mode;
        static uint8_t poweron;
        static float watt;
        static uint8_t volt;
        static float ampere;
		    static uint8_t frequency;
        static float consumption;
        static float  kwh[24]; // keeps last 23 hour on Daily Request
        static bool measure_has_data; // is true, if NotifyCallBack measure Data is called
        static bool daily_has_data; // is true, if NotifyCallBack daily Data is called
		
    private:
        static void NotifyCallbackVolt(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData, size_t length, bool isNotify);
    
};
#endif
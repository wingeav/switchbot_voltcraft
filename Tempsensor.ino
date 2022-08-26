#include "SwitchBotMeter.h"
#include "HTTPSgsm.h"
#include "VoltCraft.h"

SwitchBotMeter meter;
HTTPSgsm gsm;
VoltCraft voltcraft;

std::string addressVoltcraft = "a3:00:00:00:4f:52";
std::string addressList[2] = {"d2:a2:68:c9:33:35","fc:c8:2d:59:6b:fb" };

hw_timer_t *timer = NULL; // define Timer Variable 
uint64_t alarmInterval = (1000000 * 60 * 20); // 20 minutes
bool sendSuccess = false;

short intervalTempMeasure = 300; // 5 minutes
short intervalPowerMeasure = 30; // 30 seconds
short intervalPowerDaily = 3600; // 1 Hour
short secondsTempMeasure;
short secondsPowerMeasure;
short secondsPowerDaily;


void IRAM_ATTR onTimer() {
  if (sendSuccess) {
    Serial.println("Timer ausgelöst aber keine Sende Fehler. Setze Timer und Status zurück");
    timerWrite (timer, 0);
    sendSuccess = false;
  } else {
    Serial.println("Timer ausgelöst und Daten nicht erfolgreich gesendet.");
    Serial.println("Starte neu...");
    ESP.restart();
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("Init BLE");
  meter.InitBLE();


  Serial.println("Init GSM");
  while(!gsm.InitGSM()){
    Serial.println("Error beim initialisieren. Warte 1 Minute");
    delay(60000);
  }

  Serial.println("Init Timer");
  timer = timerBegin(0, 80, true); // Timer initialisieren mit Timer-Nummer 0, Teiler auf 80 setzen wegen der 80MHz (80 000 000 Hz / 80 = 1 000 000 mal/Sekunden wird der Timer hochgezählt -> Messung erfolgt also in Microsekunden)
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, alarmInterval, true);
  timerAlarmEnable(timer);                                                                                                                      

  voltcraft.ReadVoltCraft(addressVoltcraft);

  voltcraft.ReadVoltCraft(addressVoltcraft,true);
  
  for (auto address : addressList){
    meter.ReadTemp(address);
    while (meter.sensor == "") {delay(100);}
    Serial.printf("MAIN Temperatur: %s °C\n", String(meter.temperature).c_str());
    Serial.printf("MAIN Luftfeuche: %s %\n", String(meter.humidity).c_str());
    if (meter.sensor != "ERROR") {
        sendSuccess = gsm.SendData(meter.sensor,meter.temperature,meter.humidity);
    }
  }
  

}

void loop() {
  secondsTempMeasure += 1;
  secondsPowerMeasure += 1;
  secondsPowerDaily += 1;


  if (secondsTempMeasure >= intervalTempMeasure) {
    Serial.print("TEMP: ");
    Serial.print(secondsTempMeasure);
    Serial.print(" >= ");
    Serial.println(intervalTempMeasure);

    for (auto address : addressList){
      meter.ReadTemp(address);
      while (meter.sensor == "") {delay(100);}
      Serial.printf("MAIN Temperatur: %s °C\n", String(meter.temperature).c_str());
      Serial.printf("MAIN Luftfeuche: %s %\n", String(meter.humidity).c_str());
      if (meter.sensor != "ERROR") {
          sendSuccess = gsm.SendData(meter.sensor,meter.temperature,meter.humidity);
      }
      meter.sensor == ""; // Sensor auf "" setzen, damit er beim nächsten Mal wieder wartet, bis er daten hat
    }
    secondsTempMeasure = 0;
  }
  
  if (secondsPowerMeasure >= intervalPowerMeasure) {
    Serial.print("POWER MEASUR: ");
    Serial.print(secondsPowerMeasure);
    Serial.print(" >= ");
    Serial.println(intervalPowerMeasure);

    voltcraft.ReadVoltCraft(addressVoltcraft);
    secondsPowerMeasure = 0;
  }

  if (secondsPowerDaily >= intervalPowerDaily) {
    Serial.print("POWER DAILY: ");
    Serial.print(secondsPowerDaily);
    Serial.print(" >= ");
    Serial.println(intervalPowerDaily);

    voltcraft.ReadVoltCraft(addressVoltcraft,true);
    secondsPowerDaily = 0;
  }

  if (voltcraft.measure_has_data == true) {
    Serial.println("MEASURE HAS DATA");
    sendSuccess = gsm.SendVolt(addressVoltcraft, "measure", voltcraft.poweron,  voltcraft.watt, voltcraft.volt, voltcraft.ampere, voltcraft.frequency, voltcraft.consumption);
    voltcraft.measure_has_data = false;
  }
  if (voltcraft.daily_has_data == true) {
    Serial.println("DAILY HAS DATA");
    sendSuccess = gsm.SendVolt(addressVoltcraft,"daily", 0, 0, 0, 0, 0, 0,  voltcraft.kwh);
    voltcraft.daily_has_data = false;
  }

  delay(1000); 
}

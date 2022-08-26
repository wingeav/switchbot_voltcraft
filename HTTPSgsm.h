#ifndef HTTPSgsm_H
#define HTTPSgsm_H

//https://github.com/govorox/SSLClient
#define SIM800L_IP5306_VERSION_20200811

// See all AT commands, if wanted
//#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

//#include "utilities.h"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */


#define MODEM_RST             5
#define MODEM_PWRKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

#define MODEM_DTR            32
#define MODEM_RI             33

#define I2C_SDA              21
#define I2C_SCL              22
#define LED_GPIO             13
#define LED_ON               HIGH
#define LED_OFF              LOW

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

//Please enter your CA certificate in ca_cert.h
#include "ca_cert.h"

#include "Arduino.h"
#include <TinyGsmClient.h>
#include <Wire.h>
#include "SSLClient.h"
#include <ArduinoHttpClient.h>


class HTTPSgsm {

public:
        HTTPSgsm();
        bool InitGSM();
        bool SendData(std::string sensor, float temperature, uint8_t humidity);
        bool SendVolt(std::string sensor, std::string mode, uint8_t poweron=0, float watt=0, uint8_t volt=0, float ampere=0, uint8_t frequency=0, float consumption=0, float kwh[24] = {0});

private:


	bool setupPMU();
	void setupModem();
	void turnOffNetlight();
	void turnOnNetlight();

	// Server details
	const std::string server = "wingeav.de";
	const std::string resource = "/tempmessung.php?passwort=<PASSWORD>";
  const std::string urlVolt = "/strommessung.php?passwort=<PASSWORD>";
	const int  port = 443;

	// Your GPRS credentials (leave empty, if missing)
	const std::string apn      = "web.vodafone.de"; // Your APN
	const std::string gprsUser = ""; // User
	const std::string gprsPass = ""; // Password
	const std::string simPIN   = ""; // SIM card PIN code, if any


};
#endif
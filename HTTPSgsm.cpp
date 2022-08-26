#include "Arduino.h"
#include "HTTPSgsm.h"

// See all AT commands, if wanted
//#define DUMP_AT_COMMANDS


#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif


HTTPSgsm::HTTPSgsm() {
}


bool HTTPSgsm::setupPMU()
{
    bool en = true;
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en) {
        Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    } else {
        Wire.write(0x35); // 0x37 is default reg value
    }
    return Wire.endTransmission() == 0;
}

void HTTPSgsm::setupModem()
{
#ifdef MODEM_RST
    // Keep reset high
    SerialMon.println("MODEM_RST...");
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, HIGH);
#endif

    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);

    // Turn on the Modem power first
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);
}

void HTTPSgsm::turnOffNetlight()
{
    SerialMon.println("Turning off SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=0");
}

void HTTPSgsm::turnOnNetlight()
{
    SerialMon.println("Turning on SIM800 Red LED...");
    modem.sendAT("+CNETLIGHT=1");
}


bool HTTPSgsm::InitGSM()
{

    // Set console baud rate
    // Start power management
   if (setupPMU() == false) {
        Serial.println("Setting power error");
    }
    // Some start operations
    setupModem();

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // Or, use modem.init() if you don't need the complete restart

    // Turn off network status lights to reduce current consumption
    turnOffNetlight();

    // The status light cannot be turned off, only physically removed
    //turnOffStatuslight();

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork(240000L)) {
        SerialMon.println(" fail");
        delay(10000);
        return false;
    }
    SerialMon.println(" OK");

    // When the network connection is successful, turn on the indicator
    digitalWrite(LED_GPIO, LED_ON);

    if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
    }

    SerialMon.print(F("Connecting to APN: "));
    SerialMon.print(apn.c_str());
    if (!modem.gprsConnect(apn.c_str(), gprsUser.c_str(), gprsPass.c_str())) {
        SerialMon.println(" fail");
        delay(10000);
        return false;
    }
    SerialMon.println(" OK");
    return true;
}

bool HTTPSgsm::SendData(std::string sensor, float temperature, uint8_t humidity)
{
    TinyGsmClient client(modem);
    SSLClient secure_presentation_layer(&client);
    HttpClient http_client(secure_presentation_layer, server.c_str(), port);


    std::string data  = resource;
      data.append("&sensor=" + sensor);
      data.append("&temperatur=" +std::to_string(temperature));
      data.append("&feuchte="+std::to_string(humidity));

    SerialMon.print(F("Performing HTTPS GET request... "));
    int err = http_client.get(data.c_str());
    if (err != 0) {
      SerialMon.println(F("failed to connect"));
      SerialMon.print(err);
      SerialMon.print("\n");
      return false;
    } 

    int status_code = http_client.responseStatusCode();
    String response = http_client.responseBody();


    Serial.print("Status code: ");
    Serial.println(status_code);
    Serial.print("Response: ");
    Serial.println(response);

    http_client.stop();
    secure_presentation_layer.stop();
    client.stop();
    SerialMon.println(F("Server disconnected"));

    if (status_code == 200) {return true;} else {return false;}
}

// Send Voltcraft Data to Webserver
bool HTTPSgsm::SendVolt(std::string sensor, std::string mode, uint8_t poweron, float watt, uint8_t volt, float ampere, uint8_t frequency, float consumption, float kwh[24])
{
    TinyGsmClient client(modem);
    SSLClient secure_presentation_layer(&client);
    HttpClient http_client(secure_presentation_layer, server.c_str(), port);
    SerialMon.println("SEND VOLT");

    char buffer [330];
    if (mode == "daily") {
      SerialMon.println("create DAILY BUFFER");
      SerialMon.print("DAILY 22: ");
      SerialMon.println(kwh[22]);
      SerialMon.print("DAILY 21: ");
      SerialMon.println(kwh[21]);
      char temp [10];
      sprintf (buffer, "%s&sensor=%s&mode=%s&array=",urlVolt.c_str(),sensor.c_str(), mode.c_str());
      int count = 23;
      for (int d = 0; d < 24; d++)
      {
        if (d > 0) {strcat (buffer,"!");} // data will be split by "!" on Webserver
        sprintf (temp,"%5.4f",kwh[d]);
        strcat (buffer,temp);
      }
    } 
    if (mode == "measure") {
      SerialMon.println("create MEASURE BUFFER");
      SerialMon.print("Sensor ");
      SerialMon.println(sensor.c_str());
      SerialMon.print("Watt ");
      SerialMon.println(watt);
      SerialMon.print("ampere ");
      SerialMon.println(ampere);

      sprintf (buffer, "%s&sensor=%s&mode=%s&poweron=%d&watt=%5.4f&voltage=%d&ampere=%2.2f&frequency=%d&consumption=%5.4f",urlVolt.c_str(), sensor.c_str(), mode.c_str(), poweron, watt, volt, ampere, frequency, consumption);
    }
    SerialMon.print("BUFFER: ");
    SerialMon.println(buffer);
    SerialMon.print(F("Performing HTTPS GET request... "));
    int err = http_client.get(buffer);
    if (err != 0) {
      SerialMon.println(F("failed to connect"));
      SerialMon.print(err);
      SerialMon.print("\n");
      return false;
    } 

    int status_code = http_client.responseStatusCode();
    String response = http_client.responseBody();


    Serial.print("Status code VOLT: ");
    Serial.println(status_code);
    Serial.print("Response VOLT: ");
    Serial.println(response);

    http_client.stop();
    secure_presentation_layer.stop();
    client.stop();
    SerialMon.println(F("Server disconnected"));

    if (status_code == 200) {return true;} else {return false;}

}

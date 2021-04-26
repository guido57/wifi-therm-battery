#include <Arduino.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "captive_portal.h"
#include <myMQTT.h>
#include <EspNow.h>
#include <handleHttp.h>

//============================================
// Power supply measurement
//============================================
int analog_input_pin = 34; 

//============================================
// DS18B20 Temperature Sensor
//============================================
// GPIO where the DS18B20 is connected to
const int oneWireBus = 18;     
// GPIO giving the +3.3 to DS18B20
const int DS18B20_power_pin = 17; 
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

float readTemperature(){
  // Start the DS18B20 sensor
  sensors.begin();
  int attempts = 5;
  float read_temp = -127.0;
  while(attempts-->0 && (read_temp==-127.0 || read_temp ==85.0)){
    sensors.requestTemperatures(); 
    read_temp = sensors.getTempCByIndex(0);
  }
  return read_temp;
}

float readVoltage(){
  uint16_t adv = analogRead(analog_input_pin);
  return 1.19*2*3.3*float(adv)/4095.0; // yes, a resistive divider by two is used  
}
//============================================
// status LED
//============================================
int gpio14_status = 0; // TMS in the silkscreen of ESP32 mini
int last_gpio14_status = 0;

enum _LED_status {
  _LED_status_idle,       
  _LED_status_sending,
  _LED_status_disconnected
} ;
int LED_status;
uint8_t LED_loop_count = 0;
unsigned long last_LED_loop_msecs = 0L;
void LED_loop(){
  if(millis() < last_LED_loop_msecs + 200) // the perio is 200 msecs
    return;
  switch (LED_status)
  {
    case _LED_status_idle:
    // 1000 msec off - 1000 msec on
    gpio14_status = ((LED_loop_count % 10) < 5) ? HIGH : LOW;
    break;
    case _LED_status_sending:
    // always sending. When sending, the loop is busy so this function won't be called
    gpio14_status = HIGH;
    break;
    case _LED_status_disconnected:
    // 200 msec on - 200 msec off
    gpio14_status = ((LED_loop_count % 2) == 0) ? HIGH : LOW;
    break;
  }
  digitalWrite(14, gpio14_status);
  last_gpio14_status = gpio14_status;
  LED_loop_count++;

  last_LED_loop_msecs = millis();
}

int sendTempVoltageToThermostat(){

      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      String url = "http://"  + String(espNow->settings.entries.thermostat_IP) + ":" 
              + String(espNow->settings.entries.thermostat_port) + "/";
      //String url = "http://"  + thermostat_IP.toString() + ":" + "3000" + "/";
      http.begin(url);
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");
      // Data to send with HTTP POST
      String data = "{\"temp\":\""         + String(readTemperature(),1)
                  + "\",\"macAddress\":\"" + WiFi.macAddress() 
//                  + "\",\"voltage\":\""    + String(readVoltage(),1) 
                  + "\"}";
      String httpRequestData = data;           
      // Send HTTP POST request
      Console.printf("Sending %s to url %s after %lu millis\r\n",data.c_str(),url.c_str(),millis());
      int httpResponseCode = http.POST(httpRequestData);
      Console.printf("HTTP Response code: %d after %lu millis\r\n",httpResponseCode,millis());
      // Free resources
      http.end();
      return httpResponseCode;
}



unsigned long connecting_timeout;
//==============================================
void setup() {

       
    //   power supply DS18B20 
    pinMode(DS18B20_power_pin,OUTPUT);
    digitalWrite(DS18B20_power_pin,HIGH);

    //   flashing LED rapidly
    pinMode(14,OUTPUT);
    LED_status = _LED_status_disconnected;
    
    // setup serial port
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.printf("Setup started at %lu millis\r\n",millis()); 
    
    // start ESP-NOW
    espNow = new EspNow();
    Serial.printf("EspNow started at %lu millis\r\n",millis()); 

    connecting_timeout = millis() + 60000; // 60 seconds to connect -> enough  time to send config by ESP-NOW
    // begin connecting
    WiFi.begin(espNow->settings.entries.ssid, espNow->settings.entries.pwd);
    Serial.printf("WiFi started connecting at %lu millis\r\n",millis()); 
}


// loop() variables
unsigned long last_send_temp_msecs = 0L; 
bool send_temp_OK = false;
unsigned long send_temp_interval_msecs = 60000L;
void loop()
{

  if(WiFi.isConnected()){
    // the LED is fixed light 
    LED_status = _LED_status_sending;
    
    // MQTT is ready to publish (Console.printf) and we know config_mode or not
    if( config_status != not_ready && ! ready_to_deep_sleep_flag){
      // send Voltage and Temperature to the Thermostat every send_temp_interval_msecs   
      if(last_send_temp_msecs == 0L || millis() > last_send_temp_msecs + send_temp_interval_msecs )
      {
        // sed temperature and set repeat interval
        if(200==sendTempVoltageToThermostat()){
          send_temp_interval_msecs = SUCCESSFUL_DEEP_SLEEP_SECS*1000;
          Console.printf("Send_temp_OK after %lu msecs -> repeat after %d seconds \r\n", 
                                        millis(),send_temp_interval_msecs/1000 );
        }else{
          send_temp_interval_msecs = UNSUCCESSFUL_DEEP_SLEEP_SECS*1000;
          Console.printf("Send_temp_KO after %lu msecs -> repeat after %d seconds \r\n", 
                                        millis(),send_temp_interval_msecs/1000 );
        }  
        ready_to_deep_sleep(send_temp_interval_msecs/1000);
        last_send_temp_msecs = millis();
      }
    }

    // if config_mode is set the LED to idle (blinking slowly)    
    if(config_status == config_mode)
      LED_status = _LED_status_idle;
  
  }else{ // end WiFi.isConnected()
    // WiFi is not connected -> set the LED to blink rapidly
    LED_status = _LED_status_disconnected;
    if(millis()>connecting_timeout){
      Serial.printf("Couldn't connect to %s with pwd %s after %lu seconds\r\n", 
                espNow->settings.entries.ssid, espNow->settings.entries.pwd, connecting_timeout/1000);
      ready_to_deep_sleep(UNSUCCESSFUL_DEEP_SLEEP_SECS);
    }
  }
  LED_loop();
  MQTT_loop();
  espNow->Loop();
  web_server.Loop();    
  OTAhandle();
   
}

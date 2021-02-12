#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include "captive_portal.h"
#include "mytask.h"

#define UNSUCCESSFUL_DEEP_SLEEP_SECS  10 
#define MAX_NO_CONN_BEFORE_AP 20 

extern IPAddress thermostat_IP;
extern int port;
extern int sleep_seconds;

//============================================
// Power supply measurement
//============================================
int analog_input_pin = 34; 
//============================================
// DS18B20 Temperature Sensor
//============================================
// GPIO where the DS18B20 is connected to
const int oneWireBus = 18;     

const int DS18B20_power_pin = 17; // the +3.3 of DS18B20 is connected here

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

float readTemperature(){
  // Start the DS18B20 sensor
  sensors.begin();
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  //float temperatureF = sensors.getTempFByIndex(0);
  //Serial.print(temperatureC);
  //Serial.println("°C");
  return temperatureC;
}

float readVoltage(){
  uint16_t adv = analogRead(analog_input_pin);
  return 1.19*2*3.3*float(adv)/4095.0; // yes, a resistive divider by two is used  
}
//============================================
//Push Button
//============================================
int gpio32 = 1;
int last_gpio32 = 1;

void button_loop(){
  //Serial.printf("millis=%lu gpio12=%d\r",millis(),gpio12);
  if(digitalRead(32)==1){
    //Serial.printf("GPIIO32=1\r");  
    gpio32 = gpio32 < 10 ? gpio32 + 1 : 10; 
  }else{
    //Serial.printf("GPIIO32=0\r");  
    gpio32 = gpio32 > 0 ? gpio32 - 1 : 0; 
  }
  if( (gpio32 == 10 || gpio32 == 0) && gpio32 != last_gpio32){
    //Serial.printf("gpio32 turned from %d to %d\r\n",last_gpio32,gpio32);

    if(last_gpio32 == 10 and gpio32 == 0){
      // someone pushed the button
    }
    last_gpio32 = gpio32;
  }
}

class  MyTaskButton:MyTask{
  public:
    void (*_myCallback)();
    ~MyTaskButton(){};
    MyTaskButton(unsigned long interval, Scheduler* aS, void (* myCallback)() )
                  :  MyTask(interval, aS, myCallback){};
    
} * myTaskButton;
//============================================
// status LED
//============================================
int gpio14 = 0; // TMS in the silkscreen of ESP32 mini
int last_gpio14 = 0;

enum _LED_status {
  _LED_status_idle,  // 
  _LED_status_not_sent,
  _LED_status_disconnected
} ;
int LED_status;
uint8_t LED_loop_count = 0;
void LED_loop(){
  //Serial.printf("millis=%lu gpio12=%d\r",millis(),gpio12);
  switch(LED_status){
    case _LED_status_idle:
      // 500 msec off - 500 msec on
      //Serial.printf("LED_status = idle\r\n");
      gpio14 = ( (LED_loop_count % 10) < 5) ? HIGH : LOW;
      break;
    case _LED_status_not_sent:
      // 100 msec on - 300 msec off
      //Serial.printf("LED_status = sending\r\n");
      gpio14 = (LED_loop_count % 4 == 0) ? HIGH : LOW;
      break;
    case _LED_status_disconnected:
      // 100 msec on - 100 msec off
      //Serial.printf("LED_status = disconnected\r\n");
       gpio14 = (LED_loop_count % 2 == 0) ? HIGH : LOW;
      break;
  }
  digitalWrite(14, gpio14);
  last_gpio14 = gpio14; 
  LED_loop_count ++;  
}

class  MyTaskLED:MyTask{
  public:
    void (*_myCallback)();
    ~MyTaskLED(){};
    MyTaskLED(Scheduler* aS, void (* myCallback)() )
                  :  MyTask(100, aS, myCallback){}; // callback every 100 msecs
    
} * myTaskLED;
//================================================
// Send Temperature. 
// if succeeds -> go to sleep after 10 seconds for deep_sleep_secs
// otherwise   -> go to sleep after 10 seconds for UNSUCCESSL_DEEP_SLEEP_SECS
//================================================
bool goto_sleep = false;
int _sleep_seconds;
bool web_server_setup = false;
void sending_loop(){
  if(goto_sleep){
    Serial.printf("goto sleep! Wake up in %d seconds\r\n", _sleep_seconds);
    esp_sleep_enable_timer_wakeup(_sleep_seconds * 1000000);
    esp_deep_sleep_start();
  }
  loadCredentials();
  
  wl_status_t wst = connectWifi();
  
  if(wst == WL_CONNECTED){
      Serial.printf("Connected to SSID %s with IP %s\r\n",
                ssid.c_str(),WiFi.localIP().toString().c_str());
      no_conn_count = 0;
      saveCredentials();//save the just modified no_conn_count      
      if(web_server_setup == false){
        WebServerSetup();    
        web_server_setup = true;
      }

      LED_status = _LED_status_idle;
      printf("Read temperature %.1f °C\r\n", readTemperature() );
      printf("Read voltage %.1f\r\n",readVoltage());

      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      String url = "http://"  + thermostat_IP.toString() + ":" + String(port) + "/";
      //String url = "http://"  + thermostat_IP.toString() + ":" + "3000" + "/";
      http.begin(url);
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");
      // Data to send with HTTP POST
      String data = "{\"temp\":\""         + String(readTemperature(),1)
                  + "\",\"macAddress\":\"" + WiFi.macAddress() 
                  + "\",\"voltage\":\""    + String(readVoltage(),1) 
                  + "\"}";
      String httpRequestData = data;           
      // Send HTTP POST request
      printf("Sending %s to url %s \r\n",data.c_str(),url.c_str());
      int httpResponseCode = http.POST(httpRequestData);
      //Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      // Free resources
      http.end();
      if(httpResponseCode == 200){
        Serial.println("Temperature successfully sent -> it is time to deep sleep"); 
        LED_status = _LED_status_idle;
        _sleep_seconds = sleep_seconds;
        goto_sleep = true;
      }else{
        Serial.printf("Temperature not sent: httpResponseCode = %d\r\n",httpResponseCode); 
        LED_status = _LED_status_not_sent;
        _sleep_seconds = UNSUCCESSFUL_DEEP_SLEEP_SECS;
        Serial.printf("Going to sleep in %d seconds\r\n",_sleep_seconds); 
        goto_sleep = true;
      }
  }else{ // not connected
      Serial.printf("not connected to %s\r\n",ssid.c_str());
      LED_status = _LED_status_disconnected;
      no_conn_count ++;
      saveCredentials();//save the just incremented no_conn_count
      if( no_conn_count > MAX_NO_CONN_BEFORE_AP){    
        AccessPointSetup();
        if(web_server_setup == false){
          WebServerSetup();    
          web_server_setup = true;
        }
      }else{
        _sleep_seconds = UNSUCCESSFUL_DEEP_SLEEP_SECS;
        goto_sleep = true;
      }
  }
}

class  MyTaskSending:MyTask{
  public:
    void (*_myCallback)();
    ~MyTaskSending(){};
    MyTaskSending(Scheduler* aS, void (* myCallback)() )
                  :  MyTask(10000, aS, myCallback){};
    
} * myTaskSending;
//==============================================
void setup() {
    //   power supply DS18B20 
    pinMode(DS18B20_power_pin,OUTPUT);
    digitalWrite(DS18B20_power_pin,HIGH);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    pinMode(32, INPUT_PULLUP); // the button will short to ground
    myTaskButton = new MyTaskButton(20,&myScheduler,button_loop); 

    pinMode(14,OUTPUT);
    LED_status = _LED_status_idle;
    myTaskLED = new MyTaskLED(&myScheduler, LED_loop);

    myTaskSending = new MyTaskSending(&myScheduler, sending_loop);


}


void loop()
{
  myScheduler.execute();

}

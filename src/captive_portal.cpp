#include "captive_portal.h"
#include <TaskScheduler.h>
Scheduler myScheduler;
//=== CaptivePortal stuff ================
String softAP_ssid;
String softAP_password  = APPSK;

/* hostname for mDNS. Should work at least on windows. Try http://esp32.local */
const char *myHostname = "esp32";

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // GMT + 1
//Change the Daylight offset in milliseconds. If your country observes Daylight saving time set it to 3600. Otherwise, set it to 0.
const int   daylightOffset_sec = 3600;

/* Don't set these wifi credentials. They are configurated at runtime and stored on EEPROM */
String ssid;
String password;
IPAddress thermostat_IP(0,0,0,0); 
int port;
int sleep_seconds;
int no_conn_count;

// DNS server
//const byte DNS_PORT = 53;
//DNSServer dnsServer;

// Web server
WebServer web_server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
unsigned long lastConnectTry = 0;

/** Last time (milliseconds) I tried to ping an external IP address (usually the gateway) */
unsigned long lastPing = 0;

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

// =====================================================
wl_status_t connectWifi() {
    Serial.println("Connecting as wifi client...");
    WiFi.disconnect();
    //WiFi.mode(WIFI_STA);
    Serial.print("ssid=");Serial.println(ssid);
    Serial.print("password=");Serial.println(password);
    WiFi.begin(ssid.c_str(), password.c_str());
    return (wl_status_t) WiFi.waitForConnectResult();
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// =====================================================
// Callback for the TaskWIFI  
/**
   check WIFI conditions and try to connect to WIFI.
 * @return void
 */
void webserver_loop(void){
  //HTTP
  web_server.handleClient();
}
// =====================================================
// tosk run by Taskscheduler to handle WIFI  
class TaskWebServer : public Task {
  public:
    void (*_myCallback)();
    ~TaskWebServer() {};
    TaskWebServer(unsigned long interval, Scheduler* aS, void (* myCallback)() ) :  Task(interval, TASK_FOREVER, aS, true) {
      _myCallback = myCallback;
    };
    bool Callback(){
      _myCallback();
      return true;     
    };
};
Task * myTaskWebServer;

//===================================================
/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(2048);
  size_t len = 0;
  ssid = EEPROM.readString(len); // load ssid
  len += ssid.length() + 1;
  password = EEPROM.readString(len); // load password
  len += password.length() + 1;
  String th_ip = EEPROM.readString(len); // load thermostat IP address. 
  len += th_ip.length() + 1;
  port = EEPROM.readInt(len); // load port number
  len += 4; // a 32 bits Int occupies 4 bytes 
  sleep_seconds = EEPROM.readInt(len); // load seconds to sleep between reading temperaiure
  len += 4;
  no_conn_count = EEPROM.readInt(len); // load number of no "WIFI connection" count
  len += 4;
  String ok = EEPROM.readString(len); // load ok. ok means that stored data are valid
  EEPROM.end();

  thermostat_IP.fromString(th_ip);

  if (ok != String("OK")) {
    Serial.printf("ok=%s != OK -> set default settings\r\n",ok.c_str());
    ssid = "";
    password = "";
    no_conn_count = 0;
    thermostat_IP.fromString("0.0.0.0");
    port = 65535;
  }
  /* 
  Serial.println("Recovered settings:");
  Serial.println(ssid.length() > 0 ? ssid : "<no ssid>");
  Serial.println(password.length() > 0 ? password : "<no password>");
  Serial.printf("thermostat TCP address:port = %s:%d\r\n", th_ip.c_str(),port); 
  Serial.printf("deep sleep seconds = %d\r\n", sleep_seconds); 
  Serial.printf("number of unsuccessful WIFI connection attempts = %d\r\n", no_conn_count); 
  */
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  /*
  Serial.println("Saving settings ...");
  
  Serial.printf("ssid=%s\r\n",ssid.c_str());
  Serial.printf("password=%s\r\n",password.c_str());
  Serial.printf("Thermostat IP Address:%s\r\n", thermostat_IP.toString().c_str()); 
  Serial.printf("number of unsuccessful WIFI connection attempts = %d\r\n", no_conn_count); 
  */
  EEPROM.begin(2048);
  size_t len = 0;
  len += EEPROM.writeString(len, ssid) + 1;
  len += EEPROM.writeString(len, password) + 1;
  len += EEPROM.writeString(len, thermostat_IP.toString()) + 1;
  len += EEPROM.writeInt(len, port);
  len += EEPROM.writeInt(len, sleep_seconds );
  len += EEPROM.writeInt(len, no_conn_count );
  len += EEPROM.writeString(len,"OK") + 1; 
  EEPROM.commit();
  EEPROM.end();
}

void AccessPointSetup(){

  softAP_ssid = "ESP32_" + WiFi.macAddress();

  // Access Point Setup
  if(WiFi.getMode() != WIFI_MODE_AP && WiFi.getMode() != WIFI_MODE_APSTA){
      
    WiFi.mode(WIFI_MODE_AP);
    
    /* You can remove the password parameter if you want the AP to be open. */
    WiFi.softAP(softAP_ssid.c_str(), softAP_password.c_str());
    delay(2000); 
    WiFi.softAPConfig(apIP, apIP, netMsk);
    delay(100); // Without delay I've seen the IP address blank
    Serial.println("Access Point set:");
  }else
  {
    Serial.println("Access Point already set:");
  }
  Serial.printf("    SSID: %s\r\n", softAP_ssid.c_str());
  Serial.print("    IP address: ");
  Serial.println(WiFi.softAPIP());
}

void WebServerSetup(){

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  web_server.on("/", handleRoot);
  web_server.on("/settingssave", handleSettingsSave);
  web_server.on("/wifi", handleWifi);
  web_server.on("/wifisave", handleWifiSave);
  web_server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  web_server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  web_server.onNotFound(handleNotFound);
  web_server.begin(); // Web server start
  Serial.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  //connect = ssid.length() > 0; // Request WLAN connect if there is a SSID

  //_PL("TaskScheduler WIFI Task");
  myTaskWebServer = new TaskWebServer(30,&myScheduler,webserver_loop);
}
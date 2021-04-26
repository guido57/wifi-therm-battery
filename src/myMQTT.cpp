#include <myMQTT.h>

#define SECURE_MQTT // Comment this line if you are not using MQTT over SSL

#ifdef SECURE_MQTT
#include "esp_tls.h"

#include <EspNow.h>
#include <esp_log.h>
#include <handleHttp.h>

esp_mqtt_client_handle_t mqtt_client;
esp_mqtt_client_config_t mqtt_cfg;

// Let's Encrypt CA certificate. Change with the one you need
static const unsigned char DSTroot_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT
DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow
PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD
Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O
rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq
OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b
xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw
7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD
aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG
SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69
ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr
AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz
R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5
JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo
Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ
-----END CERTIFICATE-----
)EOF";
#endif // SECURE_MQTT

const char* MQTT_HOST = "janus.dogsally.com";
#ifdef SECURE_MQTT
const uint32_t MQTT_PORT = 8883;
#else
const uint32_t MQTT_PORT = 1883;
#endif // SECURE_MQTT
const char* MQTT_USER = "pubsub";
const char* MQTT_PASSWD = "ESP32";

MQTTlog myMQTTlog;

// variables used to check the MQTT status
bool mqtt_started = false;
bool mqtt_connected = false;
bool mqtt_subscribed = false;
bool mqtt_data_received = false;
config_status_t config_status = not_ready;
esp_mqtt_event_handle_t mqtt_last_event_received;
int last_message_published_id = -1;

int MQTTlog::printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	// get the total string length
	int _len = vsnprintf(NULL, 0, format, args);
	//Serial.printf("Lunghezza totale=%d\r\n",_len);
	// allocate a sufficient buffer
	if (MQTTlog::buffer != nullptr)
		delete (MQTTlog::buffer);
	MQTTlog::buffer = new char[_len + 1];
	MQTTlog::buffer[_len] = 0;
	// print the formatted string to the buffer
	vsprintf(MQTTlog::buffer, format, args);
	Serial.printf("%s", MQTTlog::buffer); // and to the Serial Console
	va_end(args);
	if(mqtt_connected)
		return esp_mqtt_client_publish(mqtt_client, (String(espNow->settings.entries.MQTTMainTopic) +  "/log").c_str(), MQTTlog::buffer, _len, 2, false);
	else
		return -1;
}
	


static esp_err_t mqtt_event_handler (esp_mqtt_event_handle_t event) {
	if (event->event_id == MQTT_EVENT_CONNECTED) {
		ESP_LOGI("MQTT", "MQTT msgid= %d event: %d. MQTT_EVENT_CONNECTED after %lu msecs", event->msg_id, event->event_id, millis());
		mqtt_connected = true;
		esp_mqtt_client_subscribe (mqtt_client, (String(espNow->settings.entries.MQTTMainTopic) + "/config").c_str() , 2);
	} 
	else if (event->event_id == MQTT_EVENT_DISCONNECTED) {
		ESP_LOGI("MQTT", "MQTT msgid= %d event: %d. MQTT_EVENT_DISCONNECTED", event->msg_id, event->event_id);
		mqtt_connected = false;
    	
		//esp_mqtt_client_reconnect (event->client); //not needed if autoconnect is enabled
	} else  if (event->event_id == MQTT_EVENT_SUBSCRIBED) {
		ESP_LOGI("MQTT","MQTT msgid= %d event: %d. MQTT_EVENT_SUBSCRIBED after %lu msecs", event->msg_id, event->event_id, millis());
		mqtt_subscribed = true;
		config_status = not_ready;

	} else  if (event->event_id == MQTT_EVENT_UNSUBSCRIBED) {
		mqtt_subscribed = false;
		ESP_LOGI("MQTT", "MQTT msgid= %d event: %d. MQTT_EVENT_UNSUBSCRIBED", event->msg_id, event->event_id);
		config_status = not_ready;
	} else  if (event->event_id == MQTT_EVENT_PUBLISHED) {
		ESP_LOGI("MQTT", "MQTT msg_id: %d. MQTT_EVENT_PUBLISHED", event->msg_id);
		last_message_published_id = event->msg_id;
		} else  if (event->event_id == MQTT_EVENT_DATA) {
		mqtt_last_event_received = event;
		ESP_LOGI("MQTT", "MQTT msgid= %d event: %d. MQTT_EVENT_DATA after %lu msecs", event->msg_id, event->event_id, millis());
		//ESP_LOGI("MQTT", "Topic length %d. Data length %d", event->topic_len, event->data_len);
		ESP_LOGI("MQTT", "Incoming data: %.*s %.*s", event->topic_len, event->topic, event->data_len, event->data);
		mqtt_data_received = true;
		char dd[20];
		memcpy(dd,event->data,event->data_len); dd[event->data_len] = 0;
		config_status = (String(dd) == "true") ? config_mode : not_config_mode;
		ESP_LOGI("MQTT", "data is %s config_status is %d", dd, config_status);
		  

	} else  if (event->event_id == MQTT_EVENT_BEFORE_CONNECT) {
		ESP_LOGI("MQTT", "MQTT event: %d. MQTT_EVENT_BEFORE_CONNECT after %lu msecs", event->event_id, millis());
	}
	return ESP_OK;
}

esp_err_t mqtt_setup () {

	//Serial.printf("wss_mqtt_setup started\r\n");
	
	//Serial.printf("wss_mqtt_setup esp_mqtt_client_destroy\r\n");
	esp_mqtt_client_destroy(mqtt_client);

	//mqtt_cfg.host = MQTT_HOST;
	mqtt_cfg.host = espNow->settings.entries.MQTTServer ;
	//mqtt_cfg.port = MQTT_PORT;
	mqtt_cfg.port = espNow->settings.entries.MQTTServerPort;
	//mqtt_cfg.uri = MQTT_URI; //something like wss://my_wonderful_wss_MQTT_broker_url:8083
	//mqtt_cfg.username = MQTT_USER;
	//mqtt_cfg.password = MQTT_PASSWD;
	mqtt_cfg.username = espNow->settings.entries.MQTTUser;
	mqtt_cfg.password = espNow->settings.entries.MQTTPassword;
	mqtt_cfg.keepalive = 15;
	mqtt_cfg.event_handle = mqtt_event_handler;
#ifdef SECURE_MQTT
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_SSL;
#else
	mqtt_cfg.transport = MQTT_TRANSPORT_OVER_TCP;
#endif // SECURE_MQTT
	String lwt_topic_str = String(espNow->settings.entries.MQTTMainTopic);
	mqtt_cfg.lwt_topic = (char *) lwt_topic_str.c_str();
	String lwt_str = "Last Will and Testament for " + WiFi.macAddress();
	mqtt_cfg.lwt_msg = (char *) lwt_str.c_str() ;
	mqtt_cfg.lwt_msg_len = lwt_str.length() + 1;
	
#ifdef SECURE_MQTT
	esp_err_t err = esp_tls_set_global_ca_store (DSTroot_CA, sizeof (DSTroot_CA));
	if(err != ESP_OK)
		return err;	
#endif // SECURE_MQTT
	//Serial.printf("wss_mqtt_setup esp_mqtt_client_init\r\n");
	mqtt_client = esp_mqtt_client_init (&mqtt_cfg);
	
	//Serial.printf("wss_mqtt_setup esp_mqtt_client_start\r\n");
	err = esp_mqtt_client_start (mqtt_client);
	if(err == ESP_OK)
		mqtt_started = true; // simply started, not connected yet!
	else
	{
		mqtt_started = false;
		Serial.printf("wss_mqtt_setup esp_mqtt_client_start error\r\n");
	}
	return err;
}

void mqtt_publish_reset_reasons(String topic){

	if(mqtt_connected == false)
		return ;

	RESET_REASON reset_reason_cpu0 = rtc_get_reset_reason(0);
	RESET_REASON reset_reason_cpu1 = rtc_get_reset_reason(1);
	String cpu0_reset_reason_str = ResetReasons::get_reset_reason_str(reset_reason_cpu0);
	String cpu1_reset_reason_str = ResetReasons::get_reset_reason_str(reset_reason_cpu1);
	String cpu0_verbose_reset_reason_str = ResetReasons::get_verbose_reset_reason_str(reset_reason_cpu0);
	String cpu1_verbose_reset_reason_str = ResetReasons::get_verbose_reset_reason_str(reset_reason_cpu1);
	String msg = "========== LAST CPU0 RESET REASON =============\r\n";
	msg += cpu0_reset_reason_str + "\r\n";
	msg += cpu0_verbose_reset_reason_str + "\r\n";
	msg += "========== LAST CPU1 RESET REASON =============\r\n";
	msg += cpu1_reset_reason_str + "\r\n";
	msg += cpu1_verbose_reset_reason_str + "\r\n";
	msg += "===============================================\r\n";
	//Serial.printf("%s %s\r\n",msg.c_str(),topic.c_str());
	esp_mqtt_client_publish (mqtt_client, topic.c_str(), msg.c_str(), msg.length()+1, 0, false);
}

bool ready_to_deep_sleep_flag = false;	
unsigned long deep_sleep_duration = 0L;
void ready_to_deep_sleep(int seconds){
	ready_to_deep_sleep_flag = true;
	deep_sleep_duration = 1000L * seconds;
}	

/*
  Go to deep sleep for sleep_time_seconds
*/
unsigned long mqtt_data_received_timeout;
void deep_sleep(int sleep_time_seconds){
  // Wait for this final message before sleeping			
  int final_message_id = Console.printf("goto sleep after %lu millis! Wake up in %d seconds\r\n", millis(), sleep_time_seconds);
  // wait for final_message publishing
  while (last_message_published_id != final_message_id && millis() < mqtt_data_received_timeout){
    delay(10);
  }
  Serial.printf("esp_deep_sleep_start when msg_id=%d was published, after %lu millis! Wake up in %d seconds\r\n", 
           final_message_id, millis(), sleep_time_seconds);
  esp_sleep_enable_timer_wakeup( (uint64_t)1000000L * sleep_time_seconds );
  esp_deep_sleep_start();
  
};

bool MQTT_ready = false;
bool last_mqtt_connected = false;
void MQTT_loop(){

	if(WiFi.isConnected() == false )
		return;

    // Init MQTT only once
    if(MQTT_ready == false){
      	mqtt_setup();
		MQTT_ready = true;

	    // wait for max 6 secs before declaring that no config status is received 
        mqtt_data_received_timeout = millis() + 6000;
    }
	
	if(mqtt_connected && !last_mqtt_connected ){
        Console.printf("my IP is %s\r\n", WiFi.localIP().toString().c_str());
	}
	last_mqtt_connected = mqtt_connected;

    // Check if MQTT config is set
    // if a config (a "retained message" on /config) is present, don't go to deepsleep 
    if(mqtt_data_received)
    {
      if(config_status == config_mode){
        Console.printf("Config true received. data_len=%d\r\n", mqtt_last_event_received->data_len);  
        Console.printf("Don't go to sleep until a false retained message is received on topic %s\r\n", 
            (String(espNow->settings.entries.MQTTMainTopic) + "/config").c_str());  
        // start OTA and webserver
        if(OTA_webserver_ready == false)
          OTA_webserver_setup();
      }else if(config_status == not_config_mode) {
        Console.printf("config not received or is false->Go to deep sleep. data_len=%d\r\n", mqtt_last_event_received->data_len);  
      }
      mqtt_data_received = false;
    }

	if(   config_status == not_ready      && millis() > mqtt_data_received_timeout)
		config_status = not_config_mode; 


	if(ready_to_deep_sleep_flag){
		// now decide to go to deep sleep and how long to sleep
		if(   (config_status == not_ready      && millis() > mqtt_data_received_timeout)
			|| config_status == not_config_mode 
		){
			// goto to deep sleep if config_mode != true or mqtt_data_received_timeout
			deep_sleep(deep_sleep_duration/1000);
		}
		else
			ready_to_deep_sleep_flag = false;
	}
}


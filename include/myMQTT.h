#include <Arduino.h>
#include <mqtt_client.h>
#include <reset_reason.h>


#define SUCCESSFUL_DEEP_SLEEP_SECS  60
#define UNSUCCESSFUL_DEEP_SLEEP_SECS  10 
#define NO_CONN_DEEP_SLEEP_SECS  10 
#define WAIT_ESP_NOW_MSG_SECS 5
#define CHECK_CONFIG_SECS 5

/*
MQTT configuration
    it is setup by mqtt_setup() with default values 
    but it can be changed anytime 
*/
extern esp_mqtt_client_config_t mqtt_cfg;

/*
MQTT client
    Used with publish and subsribe 
    after setup
*/
extern esp_mqtt_client_handle_t mqtt_client;


/*
MQTT client
    if the mqtt_client is ready (connected or not)
*/
extern bool MQTT_ready;

/*
MQTT client
    if the mqtt_client is connected to the broker or not
*/
extern bool mqtt_connected;

/*
MQTT client
    if the mqtt_client has subscribed or not
*/
extern bool mqtt_subscribed;

/*
MQTT client
    if mqtt_client received data
    Set it to false before subscribing and after reading the received data
*/
extern bool mqtt_data_received;

/*
MQTT client
    the last event with data received
    Use it to read the data received
*/
extern esp_mqtt_event_handle_t mqtt_last_event_received;

/*
MQTT client
    this flag signals if it will go to deep sleep after publishing the final message
*/
extern bool ready_to_deep_sleep_flag;

/*
MQTT client
    signal ti be ready to deep sleep after publishing the final message and if not in config_mode
*/
extern void ready_to_deep_sleep(int seconds);


extern int last_message_published_id;

extern void MQTT_loop();

/**
 * config_status structure
 */
typedef enum {
    not_ready,
	config_mode,
	not_config_mode
} config_status_t;
extern config_status_t config_status;

// call this when you want to deep sleep
extern void goto_deep_sleep(unsigned long msecs);

/*
MQTT initial setup
    setup wss MQTT default values 
    that can be changed by mqtt_cfg 
*/
esp_err_t mqtt_setup();
/*
MQTT publish reset reasons
    Publish the reset reasons for both
    CPU0 and CPU1 to MQTT 
*/
void mqtt_publish_reset_reasons(String topic);


class MQTTlog {

  char * buffer = nullptr;
public:    
  //MQTTlog();
  //~MQTTlog();
  int printf(const char* format, ...);

};
// MQTT communicator 
extern MQTTlog myMQTTlog;

#define Console myMQTTlog // you can use Console.printf(...)


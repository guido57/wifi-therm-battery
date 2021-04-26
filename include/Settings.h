#ifndef MY_SETINGS_H
#define MY_SETTINGS_H

#include <Arduino.h>


class Settings {
  public:
    /* GLOBAL SETTINGS */
    struct entries_struct {
      char ssid[32] = "my_ssid";     // don't modify this. It will be set by a ESP-NOW message
      char pwd[32] = "my_ssid_pwd";  // don't modify this. It will be set by a ESP-NOW message
      char MQTTServer[32] = "the_MQTT_server";
      int MQTTServerPort = 8000;
      char MQTTUser[64] = "the_MQTT_user";
      char MQTTPassword[64] = "the_MQTT_password";
      char MQTTMainTopic[64] = "the_MQTT_main_topic";
      char MQTTSubscribedTopic[64] = "the_MQTT_subscribed_topic";
      // From here on add or delete entries as you like. Don't use String but use char xyz[N] instead.
      // if you want to update them by esp-now messages, add a specific section in EspNow::Loop as well
      char thermostat_IP[17] ="000:000:000:000";
      int thermostat_port = 2048;
      int sleep_seconds = 60;
      int no_conn_count = 3;
} entries;
    
    Settings();
    ~Settings();
    void Init();

    void Load();
    void Save();

};


#endif
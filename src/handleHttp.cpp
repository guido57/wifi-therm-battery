/** Handle root or redirect to captive portal */
#include <arduino.h>
#include <handleHttp.h>
#include <main.h>
#include <tools.h>

#define S String

extern IPAddress thermostat_IP;
extern int port;
extern int sleep_seconds;
// ==================================================================================================
void handleRoot() {

  if (captivePortal()) { // If captive portal redirect instead of displaying the page.
    return;
  }
  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.sendHeader("Pragma", "no-cache");
  web_server.sendHeader("Expires", "-1");

  String Page;
  Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>WIFI Thermometer</title></head><body>"
            "<h1>This is your WIFI Thermometer</h1>");
  if (web_server.client().localIP() == apIP) {
    Page += S("<p>You are connected through the soft AP: ") + softAP_ssid + S("</p>");
  } else {
    Page += S("<p>You are connected through the wifi network: ") + ssid + S("</p>");
  }
  
  Page += S("<p>Temperature is ")  +  String(readTemperature()) + S(" ^C</p>");
  Page += S("<p>Voltage is ") + String(readVoltage()) + S(" Volts</p>");
  
  Page += S(
            "<form method='POST' action='settingssave'>"
          );
  Page += S("Send temperature to:<br/>");
  Page += S("IP Address: <input type='text' name='th_IP' minlength='7' maxlength='15' size='15' pattern='^((\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d\\d|2[0-4]\\d|25[0-5])$' value='");
  Page += thermostat_IP.toString() + S("'><br>");
  Page += S("TCP port: <input type='text' name='port' value='" + String(port) + "'><br>" );
  Page += S("if it succeeds -> goto deep sleep after 10 seconds (to give time to change config!)<br/>");
  Page += S("otherwise      -> retry after 10 seconds<br/>");
  Page += S("Deep sleep (seconds): <input type='text' name='sleep_secs' value='" + String(sleep_seconds) + "'><br>" );
  Page += F(
            "<br /><input id='id_submit' type='submit' value='Save'/>"
            "</form>"
            "<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
            "</body></html>");

  web_server.send(200, "text/html", Page);
}
// ==================================================================================================
/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  Serial.println("==== Captive portal verification ...");
  Serial.print("hostHeader: "); Serial.println(web_server.hostHeader());
  if (!isIp(web_server.hostHeader()) && web_server.hostHeader() != (String(myHostname) + ".local")) {
    //Serial.print("myHostname: "); Serial.println(myHostname);
    //Serial.println("Request redirected to captive portal");
    Serial.print("Captive portal location: "); Serial.println(String("http://") + toStringIp(web_server.client().localIP()));
    web_server.sendHeader("Location", String("http://") + toStringIp(web_server.client().localIP()), true);
    web_server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    web_server.client().stop(); // Stop is needed because we sent no content length
    Serial.println("==== Captive portal verification returned True");
    return true;
  }
  Serial.println("==== Captive portal verification returned False");
  return false;
}
// ==================================================================================================
/** Wifi config page handler */
void handleWifi() {
  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.sendHeader("Pragma", "no-cache");
  web_server.sendHeader("Expires", "-1");

  String Page;
  Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>CaptivePortal</title></head><body>"
            "<h1>Wifi config</h1>");
  if (web_server.client().localIP() == apIP) {
    Page += String(F("<p>You are connected through the soft AP: ")) + softAP_ssid + F("</p>");
  } else {
    Page += String(F("<p>You are connected through the wifi network: ")) + ssid + F("</p>");
  }
  
  Page +=
    String(F(
             "\r\n<br />"
             "<table><tr><th align='left'>SoftAP config</th></tr>"
             "<tr><td>SSID ")) +
    softAP_ssid +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.softAPIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br />"
      "<table><tr><th align='left'>WLAN config</th></tr>"
      "<tr><td>SSID ") +
    ssid +
    F("</td></tr>"
      "<tr><td>IP ") +
    toStringIp(WiFi.localIP()) +
    F("</td></tr>"
      "</table>"
      "\r\n<br />"
      "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>");
  
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      Page += String(F("\r\n<tr><td>SSID ")) + WiFi.SSID(i) + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? F(" ") : F(" *")) + F(" (") + WiFi.RSSI(i) + F(")</td></tr>");
    }
  } else {
    Page += F("<tr><td>No WLAN found</td></tr>");
  }
  
  Page += F(
            "</table>"
            "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
            "<input type='text' placeholder='network' name='n'/>"
            "<br /><input type='password' placeholder='password' name='p'/>"
            "<br /><input type='submit' value='Connect/Disconnect'/></form>"
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>");
  
  web_server.send(200, "text/html", Page);
  web_server.client().stop(); // Stop is needed because we sent no content length
}
// ==================================================================================================
/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  //Serial.println("wifi save");
  ssid = web_server.arg("n");
  password = web_server.arg("p");
  web_server.sendHeader("Location", "wifi", true);
  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.sendHeader("Pragma", "no-cache");
  web_server.sendHeader("Expires", "-1");
  web_server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  web_server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = ssid.length() > 0; // Request WLAN connect with new credentials if there is a SSID
}
// ==================================================================================================
/** Handle the WLAN save form and redirect to WLAN config page again */
void handleSettingsSave() {
  
  //for(int a=0; a<web_server.args();a++){
  //  printf("arg=%s value=%s\r\n",web_server.argName(a).c_str(),web_server.arg(a).c_str());
  //}

  String th_IP = web_server.arg("th_IP");
  Serial.printf("Got thermostat IP Address %s\r\n",th_IP.c_str());
  thermostat_IP.fromString(th_IP);
  Serial.printf("thermostat_IP as string is %s\r\n",thermostat_IP.toString().c_str());
  String port_str = web_server.arg("port");  
  String sleep_secs_str = web_server.arg("sleep_secs");  
  port = port_str.toInt();
  sleep_seconds = sleep_secs_str.toInt();

  web_server.sendHeader("Location", "/", true);
  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.sendHeader("Pragma", "no-cache");
  web_server.sendHeader("Expires", "-1");
  web_server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  web_server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
}
// ==================================================================================================
void handleNotFound() {
  if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += web_server.uri();
  message += F("\nMethod: ");
  message += (web_server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += web_server.args();
  message += F("\n");

  for (uint8_t i = 0; i < web_server.args(); i++) {
    message += String(F(" ")) + web_server.argName(i) + F(": ") + web_server.arg(i) + F("\n");
  }
  web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web_server.sendHeader("Pragma", "no-cache");
  web_server.sendHeader("Expires", "-1");
  web_server.send(404, "text/plain", message);
}
// ==================================================================================================

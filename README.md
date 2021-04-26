# WiFi Battery Powered Temperature Sensor

This is a WiFi Thermometer

## Show

[![Watch the video](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Internet%20Radio%20ESP32%20I2S%20PCM5102%20WS2812.PNG)](https://youtu.be/EDWJl283ymY)

## Overview


## Schematics

[![Watch](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/fritzing%20schematics.PNG)](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/fritzing%20schematics.PNG)

For instructions to build your own Funny Spectrum device, see https://hackaday.io/project/176937-funny-audio-spectrum-by-colors

## Very basic web pages

# Web page at /wifi url to set/change the WIFI credentials accessing the builtin hotspot 
If no Internet is available, the device becomes an access point, which you can connect to by smartphone or PC and access this page at http://192.168.4.1

[![Watch](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Internet%20settings%20by%20Access%20Point.png)](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Internet%20settings%20by%20Access%20Point.png)

After getting a local IP address e.g. 192.168.1.242, going to the url http://192.168.1.242/wifi you see something like this:

[![Watch](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Internet%20settings.PNG)](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Internet%20settings.PNG)


# Web page at the root of the web server (i.e. the ip address got by ESP32), to set/change up to five internet radios and set the volume
[![Watch](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Choose%20your%20Internet%20Radio.PNG)](https://github.com/guido57/Funny-Spectrum/blob/master/pictures/Choose%20your%20Internet%20Radio.PNG)


## Issues
- There's no button or anything else to turn it on/off but you can set turn on(/off time mye the config page
- Even if:
a) "FFT calculations"  
b) "http receiving" 
c) "MP3/AAC decoding" 
d) LEDs updating 
run concurrently on core 1, the FFT can run real time without any audio click.  




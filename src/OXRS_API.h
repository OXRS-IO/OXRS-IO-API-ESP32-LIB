/*
 * OXRS_API.h
 */

#ifndef OXRS_API_H
#define OXRS_API_H

#include <ArduinoJson.h>
#include <Ethernet.h>
#include <WiFi.h>
#include <aWOT.h>

#include <OXRS_MQTT.h>

class OXRS_API
{
  public:
    OXRS_API(OXRS_MQTT& mqtt);

    void begin(void);
    
    void checkEthernet(EthernetClient * client);
    void checkWifi(WiFiClient * client);

  private:
    Application _api;

    void _initialiseRestApi(void);
};

#endif

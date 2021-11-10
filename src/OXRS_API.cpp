/*
 * OXRS_API.cpp
 */

#include "Arduino.h"
#include "OXRS_API.h"
#include "bootstrap_html.h"

// Filename where MQTT config are persisted on the file system
static const char * MQTT_CONFIG_FILENAME = "/mqtt.json";

// Filename where device config is persisted on the file system
static const char * DEVICE_CONFIG_FILENAME = "/deviceConfig.json";

OXRS_MQTT * _apiMqtt;

// Flag used to trigger a restart
uint8_t restart = 0;

/* File system helpers */
boolean _mountFS()
{
  return SPIFFS.begin();
}

boolean _formatFS()
{
  return SPIFFS.format();
}

boolean _readJson(DynamicJsonDocument * json, const char * filename)
{
  File file = SPIFFS.open(filename, "r");

  if (!file) 
    return false;
  
  if (file.size() == 0)
    return false;
  
  DeserializationError error = deserializeJson(*json, file);
  if (error) 
  {
    file.close();
    return false;
  }
  
  file.close();
  return json->isNull() ? false : true;
}

boolean _writeJson(DynamicJsonDocument * json, const char * filename)
{
  File file = SPIFFS.open(filename, "w");

  if (!file) 
    return false;

  serializeJson(*json, file);

  file.close();
  return true;
}

boolean _deleteFile(const char * filename)
{
  return SPIFFS.remove(filename);
}

void _getBootstrap(Request &req, Response &res)
{
  res.set("Content-Type", "text/html");
  res.print(BOOTSTRAP_HTML);
}

void _postRestart(Request &req, Response &res)
{
  restart = 1;
  res.sendStatus(204);
}

void _postFactoryReset(Request &req, Response &res)
{
  if (!_formatFS())
  {
    res.sendStatus(500);
    return;
  }

  restart = 1;
  res.sendStatus(204);
}

void _getMqttConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(2048);
  _apiMqtt->getMqttConfig(json.as<JsonVariant>());

  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postMqttConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(2048);
  deserializeJson(json, req);

  _apiMqtt->setMqttConfig(json.as<JsonVariant>());
  _apiMqtt->reconnect();
  
  if (!_writeJson(&json, MQTT_CONFIG_FILENAME))
  {
    res.sendStatus(500);
  }
  else
  {
    res.sendStatus(204);
  }    
}

void _postDeviceConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(4096);
  deserializeJson(json, req);

  _apiMqtt->setDeviceConfig(json.as<JsonVariant>());
  
  if (!_writeJson(&json, DEVICE_CONFIG_FILENAME))
  {
    res.sendStatus(500);
  }
  else
  {
    res.sendStatus(204);
  }    
}

OXRS_API::OXRS_API(OXRS_MQTT& mqtt)
{
  _apiMqtt = &mqtt;
}

void OXRS_API::begin()
{
  // Mount the file system
  if (!_mountFS())
  {
    // Format and re-mount if mount fails - should only happen on first boot
    _formatFS();
    _mountFS();
  }

  DynamicJsonDocument json(4096);

  // Restore any persisted MQTT config
  if (_readJson(&json, MQTT_CONFIG_FILENAME))
  {
    _apiMqtt->setMqttConfig(json.as<JsonVariant>());
  }
  
  // Restore any persisted device config
  if (_readJson(&json, DEVICE_CONFIG_FILENAME))
  {
    _apiMqtt->setDeviceConfig(json.as<JsonVariant>());
  }
  
  // Initialise the REST API endpoints
  _initialiseRestApi();
}

void OXRS_API::checkEthernet(EthernetClient * client)
{
  _checkRestart();
  
  if (client->connected()) {
    _api.process(client);
    client->stop();
  }    
}

void OXRS_API::checkWifi(WiFiClient * client)
{
  _checkRestart();

  if (client->connected()) {
    _api.process(client);
    client->stop();
  }    
}

void OXRS_API::_initialiseRestApi(void)
{  
  _api.get("/", &_getBootstrap);
  _api.post("/restart", &_postRestart);
  _api.post("/factoryReset", &_postFactoryReset);
  _api.get("/mqtt", &_getMqttConfig);
  _api.post("/mqtt", &_postMqttConfig);
  _api.post("/deviceConfig", &_postDeviceConfig);
}

void OXRS_API::_checkRestart(void)
{
  if (restart) { ESP.restart(); }  
}
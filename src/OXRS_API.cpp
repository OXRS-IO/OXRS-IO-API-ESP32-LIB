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
  Serial.print(F("[api ] mounting SPIFFS..."));
  if (!SPIFFS.begin())
  { 
    Serial.println(F("failed"));
    return false; 
  }
  Serial.println(F("done"));
  return true;
}

boolean _formatFS()
{
  Serial.print(F("[api ] formatting SPIFFS..."));
  if (!SPIFFS.format())
  { 
    Serial.println(F("failed"));
    return false; 
  }
  Serial.println(F("done"));
  return true;
}

boolean _readJson(DynamicJsonDocument * json, const char * filename)
{
  Serial.print(F("[api ] reading "));  
  Serial.print(filename);
  Serial.print(F("..."));

  File file = SPIFFS.open(filename, "r");
  if (!file) 
  {
    Serial.println(F("failed to open file"));
    return false;
  }
  
  if (file.size() == 0)
  {
    Serial.println(F("not found"));
    return false;
  }

  Serial.print(file.size());
  Serial.println(F(" bytes read"));
  
  DeserializationError error = deserializeJson(*json, file);
  if (error) 
  {
    Serial.print(F("[api ] failed to deserialise JSON: "));
    Serial.println(error.f_str());

    file.close();
    return false;
  }
  
  file.close();
  return json->isNull() ? false : true;
}

boolean _writeJson(DynamicJsonDocument * json, const char * filename)
{
  Serial.print(F("[api ] writing "));
  Serial.print(filename);
  Serial.print(F("..."));

  File file = SPIFFS.open(filename, "w");
  if (!file) 
  {
    Serial.println(F("failed to open file"));
    return false;
  }

  Serial.print(serializeJson(*json, file));
  Serial.println(F(" bytes written"));

  file.close();
  return true;
}

boolean _deleteFile(const char * filename)
{
  Serial.print(F("[api ] deleting "));
  Serial.print(filename);
  Serial.print(F("..."));

  if (!SPIFFS.remove(filename))
  {
    Serial.println(F("failed to delete file"));
    return false;
  }

  Serial.println(F("done"));
  return true;
}

void _getBootstrap(Request &req, Response &res)
{
  Serial.println(F("[api ] / [get]"));

  res.set("Content-Type", "text/html");
  res.print(BOOTSTRAP_HTML);
}

void _postRestart(Request &req, Response &res)
{
  Serial.println(F("[api ] /restart [post]"));

  restart = 1;
  res.sendStatus(204);
}

void _postFactoryReset(Request &req, Response &res)
{
  Serial.println(F("[api ] /factoryReset [post]"));

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
  Serial.println(F("[api ] /mqtt [get]"));

  DynamicJsonDocument json(2048);
  _apiMqtt->getMqttConfig(json.as<JsonVariant>());

  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postMqttConfig(Request &req, Response &res)
{
  Serial.println(F("[api ] /mqtt [post]"));

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
  Serial.println(F("[api ] /deviceConfig [post]"));

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
    _formatFS();
  }

  DynamicJsonDocument json(4096);

  // Restore any persisted MQTT config
  if (_readJson(&json, MQTT_CONFIG_FILENAME))
  {
    Serial.print(F("[api ] restore MQTT config from SPIFFS..."));
    _apiMqtt->setMqttConfig(json.as<JsonVariant>());
    Serial.println(F("done"));
  }
  
  // Restore any persisted device config
  if (_readJson(&json, DEVICE_CONFIG_FILENAME))
  {
    Serial.print(F("[api ] restore device config from SPIFFS..."));
    _apiMqtt->setDeviceConfig(json.as<JsonVariant>());
    Serial.println(F("done"));
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
  Serial.println(F("[api ] adding / handler [get]"));
  _api.get("/", &_getBootstrap);
  
  Serial.println(F("[api ] adding /restart handler [post]"));
  _api.post("/restart", &_postRestart);

  Serial.println(F("[api ] adding /factoryReset handler [post]"));
  _api.post("/factoryReset", &_postFactoryReset);

  Serial.println(F("[api ] adding /mqtt handler [get]"));
  _api.get("/mqtt", &_getMqttConfig);

  Serial.println(F("[api ] adding /mqtt handler [post]"));
  _api.post("/mqtt", &_postMqttConfig);

  Serial.println(F("[api ] adding /deviceConfig handler [post]"));
  _api.post("/deviceConfig", &_postDeviceConfig);
}

void OXRS_API::_checkRestart(void)
{
  if (restart) 
  { 
    Serial.println(F("[api ] restarting..."));
    ESP.restart(); 
  }  
}
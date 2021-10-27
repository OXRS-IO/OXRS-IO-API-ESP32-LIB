/*
 * OXRS_API.cpp
 */

#include "Arduino.h"
#include "OXRS_API.h"
#include "bootstrap_html.h"

// Filename where MQTT settings are persisted on the file system
static const char * MQTT_JSON_FILENAME = "/mqtt.json";

OXRS_MQTT * _apiMqtt;

// Flag used to trigger a restart
uint8_t restart = 0;

/* File system helpers */
void _mountFS()
{
  Serial.print(F("[api ] mounting SPIFFS..."));
  if (!SPIFFS.begin())
  { 
    Serial.println(F("failed, might need formatting?"));
    return; 
  }
  Serial.println(F("done"));
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

  // Schedule a restart
  restart = 1;
  res.sendStatus(204);
}

void _postFactoryReset(Request &req, Response &res)
{
  Serial.println(F("[api ] /factoryReset [post]"));

  DynamicJsonDocument json(64);
  deserializeJson(json, req);
  
  // Factory reset - either wiping setup/config data only or format file system
  if (json.isNull() || !json["formatFileSystem"].as<boolean>())
  {
    if (!_deleteFile(MQTT_JSON_FILENAME))
    {
      res.sendStatus(500);
      return;
    }
  }
  else
  {
    if (!_formatFS())
    {
      res.sendStatus(500);
      return;
    }
  }

  // Schedule a restart
  restart = 1;
  res.sendStatus(204);
}

void _getMqtt(Request &req, Response &res)
{
  Serial.println(F("[api ] /mqtt [get]"));

  DynamicJsonDocument json(2048);
  _apiMqtt->getJson(json.as<JsonVariant>());

  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postMqtt(Request &req, Response &res)
{
  Serial.println(F("[api ] /mqtt [post]"));

  DynamicJsonDocument json(2048);
  deserializeJson(json, req);

  _apiMqtt->setJson(json.as<JsonVariant>());
  _apiMqtt->reconnect();
  
  if (!_writeJson(&json, MQTT_JSON_FILENAME))
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
  _mountFS();

  // Restore any persisted MQTT settings
  DynamicJsonDocument json(2048);
  if (_readJson(&json, MQTT_JSON_FILENAME))
  {
    Serial.print(F("[api ] restore MQTT settings from file..."));
    _apiMqtt->setJson(json.as<JsonVariant>());
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
  _api.get("/mqtt", &_getMqtt);

  Serial.println(F("[api ] adding /mqtt handler [post]"));
  _api.post("/mqtt", &_postMqtt);
}

void OXRS_API::_checkRestart(void)
{
  if (restart) 
  { 
    Serial.println(F("[api ] restarting..."));
    ESP.restart(); 
  }  
}
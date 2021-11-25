/*
 * OXRS_API.cpp
 */

#include "Arduino.h"
#include "OXRS_API.h"
#include "bootstrap_html.h"
#include "ota_html.h"

// Filename where MQTT connection properties are persisted on the file system
static const char * MQTT_FILENAME = "/mqtt.json";

// Filename where config is persisted on the file system
static const char * CONFIG_FILENAME = "/config.json";

// Pointer to the MQTT lib so we can get/set config
OXRS_MQTT * _apiMqtt;

// Optional firmware details
const char * _apiFwName;
const char * _apiFwShortName;
const char * _apiFwMaker;
const char * _apiFwVersion;

// Flag used to trigger a restart
boolean restart = false;

/* File system helpers */
boolean _mountFS()
{
  return SPIFFS.begin();
}

boolean _formatFS()
{
  return SPIFFS.format();
}

void _setMqtt(JsonVariant json)
{
  // broker is mandatory so don't clear if not explicitly specified
  if (json.containsKey("broker"))
  { 
    if (json.containsKey("port"))
    { 
      _apiMqtt->setBroker(json["broker"], json["port"].as<uint16_t>());
    }
    else
    {
      _apiMqtt->setBroker(json["broker"], MQTT_DEFAULT_PORT);
    }
  }
  
  // client id is mandatory so don't clear if not explicitly specified
  if (json.containsKey("clientId"))
  { 
    _apiMqtt->setClientId(json["clientId"]);
  }
  
  if (json.containsKey("username") && json.containsKey("password"))
  { 
    _apiMqtt->setAuth(json["username"], json["password"]);
  }
  else
  {
    _apiMqtt->setAuth(NULL, NULL);
  }
  
  if (json.containsKey("topicPrefix"))
  { 
    _apiMqtt->setTopicPrefix(json["topicPrefix"]);
  }
  else
  {
    _apiMqtt->setTopicPrefix(NULL);
  }

  if (json.containsKey("topicSuffix"))
  { 
    _apiMqtt->setTopicSuffix(json["topicSuffix"]);
  }
  else
  {
    _apiMqtt->setTopicSuffix(NULL);
  }
}

void _setConfig(JsonVariant json)
{
  _apiMqtt->setConfig(json);  
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

void _getFirmware(Request &req, Response &res)
{
  if (_apiFwName == NULL)
  {
    res.sendStatus(404);
    return;
  }

  DynamicJsonDocument json(256);
  
  json["name"] = _apiFwName;
  json["shortName"] = _apiFwShortName;
  json["maker"] = _apiFwMaker;
  json["version"] = _apiFwVersion;
  
  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postRestart(Request &req, Response &res)
{
  restart = true;
  res.sendStatus(204);
}

void _postFactoryReset(Request &req, Response &res)
{
  if (!_formatFS())
  {
    res.sendStatus(500);
    return;
  }

  restart = true;
  res.sendStatus(204);
}

void _getMqtt(Request &req, Response &res)
{
  DynamicJsonDocument json(2048);  

  if (!_readJson(&json, MQTT_FILENAME))
  {
    // if no persisted config still return the client id
    json["clientId"] = _apiMqtt->getClientId();
  }
  else
  {
    // remove any sensitive config
    json.remove("password");
  }

  // always return the connected state
  json["connected"] = _apiMqtt->connected();
  
  // if we are connected then add the various MQTT topics
  if (_apiMqtt->connected())
  {
    JsonObject topics = json.createNestedObject("topics");
    char topic[64];
    
    topics["lwt"] = _apiMqtt->getLwtTopic(topic);
    topics["adopt"] = _apiMqtt->getAdoptTopic(topic);

    topics["config"] = _apiMqtt->getConfigTopic(topic);
    topics["command"] = _apiMqtt->getCommandTopic(topic);

    topics["status"] = _apiMqtt->getStatusTopic(topic);
    topics["telemetry"] = _apiMqtt->getTelemetryTopic(topic);    
  }
  
  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postMqtt(Request &req, Response &res)
{
  DynamicJsonDocument json(2048);
  deserializeJson(json, req);

  if (!_writeJson(&json, MQTT_FILENAME))
  {
    res.sendStatus(500);
    return;
  }

  _setMqtt(json.as<JsonVariant>());
  _apiMqtt->reconnect();

  res.sendStatus(204);
}

void _getConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(4096);
  
  if (!_readJson(&json, CONFIG_FILENAME))
  {
    res.sendStatus(404);
    return;
  }

  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(4096);
  deserializeJson(json, req);
  
  if (!_writeJson(&json, CONFIG_FILENAME))
  {
    res.sendStatus(500);
    return;
  }

  _setConfig(json.as<JsonVariant>());

  res.sendStatus(204);
}

void _getOta(Request &req, Response &res)
{
  res.set("Content-Type", "text/html");
  res.print(OTA_HTML);
}

void _postOta(Request &req, Response &res)
{
  int contentLength = req.left();

  if (!Update.begin(contentLength))
  {
    res.status(500);
    return Update.printError(req);
  }

  uint32_t start = millis();
  while (!req.available() && (millis() - start <= 5000)) {}

  if (!req.available())
  {
    return res.sendStatus(408);
  }

  if (Update.writeStream(req) != contentLength)
  {
    res.status(400);
    return Update.printError(req);
  }

  if (!Update.end(true))
  {
    res.status(500);
    return Update.printError(req);
  }

  restart = true;
  res.sendStatus(204);
}

OXRS_API::OXRS_API(OXRS_MQTT& mqtt)
{
  _apiMqtt = &mqtt;
}

void OXRS_API::begin()
{
  if (!_mountFS())
  {
    _formatFS();
    _mountFS();
  }

  DynamicJsonDocument json(4096);

  if (_readJson(&json, MQTT_FILENAME))
  {
    _setMqtt(json.as<JsonVariant>());
  }
  
  if (_readJson(&json, CONFIG_FILENAME))
  {
    _setConfig(json.as<JsonVariant>());
  }
  
  _initialiseRestApi();
}

void OXRS_API::setFirmware(const char * fwName, const char * fwShortName, const char * fwMaker, const char * fwVersion)
{
  _apiFwName      = fwName;
  _apiFwShortName = fwShortName;
  _apiFwMaker     = fwMaker;
  _apiFwVersion   = fwVersion;
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
  _api.get("/firmware", &_getFirmware);

  _api.post("/restart", &_postRestart);
  _api.post("/factoryReset", &_postFactoryReset);

  _api.get("/mqtt", &_getMqtt);
  _api.post("/mqtt", &_postMqtt);

  _api.get("/config", &_getConfig);
  _api.post("/config", &_postConfig);

  _api.get("/ota", &_getOta);
  _api.post("/ota", &_postOta);
}

void OXRS_API::_checkRestart(void)
{
  if (restart) { ESP.restart(); }  
}
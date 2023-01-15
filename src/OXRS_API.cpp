/*
 * OXRS_API.cpp
 */

#include "Arduino.h"
#include "OXRS_API.h"

// Filename where MQTT connection properties are persisted on the file system
static const char * MQTT_FILENAME = "/mqtt.json";

// Filename where config is persisted on the file system
static const char * CONFIG_FILENAME = "/config.json";

// Pointer to the MQTT lib so we can get/set config
OXRS_MQTT * _apiMqtt;

// Callback for building the adoption payload
jsonCallback _apiAdopt;

// Flag used to trigger a restart
boolean restart = false;

/* File system helpers */
boolean _mountFS()
{
  return LittleFS.begin();
}

boolean _formatFS()
{
  return LittleFS.format();
}

boolean _eraseWifiCreds()
{
  // ignore if no creds exist
  if (WiFi.SSID().length() == 0)
    return true;
  
  // must be STA to disconnect/erase creds
  #if defined(ESP8266)
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
  #else
    WiFi.enableSTA(true);
    WiFi.disconnect(true, true);
  #endif
  
  return true;
}

boolean _readJson(DynamicJsonDocument * json, const char * filename)
{
  File file = LittleFS.open(filename, "r");

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
  File file = LittleFS.open(filename, "w");

  if (!file) 
    return false;

  serializeJson(*json, file);

  file.close();
  return true;
}

boolean _deleteFile(const char * filename)
{
  return LittleFS.remove(filename);
}

/* MQTT handlers */
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

void _setCommand(JsonVariant json)
{
  _apiMqtt->setCommand(json);  
}

/* API middleware handlers */
void _apiCorsOptions(Request &req, Response &res)
{
  res.set("Access-Control-Allow-Origin", "*");
  res.set("Access-Control-Allow-Headers", "*");
  
  res.sendStatus(204);
}

void _apiCors(Request &req, Response &res)
{
  res.set("Access-Control-Allow-Origin", "*");
  res.set("Access-Control-Allow-Headers", "*");
}

/* API endpoint handlers */
void _getApiAdopt(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_ADOPT_MAX_SIZE);

  if (_apiAdopt)
  { 
    _apiAdopt(json.as<JsonVariant>());
  }
  
  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _getApiMqtt(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_MQTT_MAX_SIZE);  

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

void _postApiMqtt(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_MQTT_MAX_SIZE);

  DeserializationError error = deserializeJson(json, req);
  if (error) 
  {
    res.sendStatus(500);
    return;
  }

  if (!_writeJson(&json, MQTT_FILENAME))
  {
    res.sendStatus(500);
    return;
  }

  _setMqtt(json.as<JsonVariant>());
  _apiMqtt->reconnect();

  res.sendStatus(204);
}

void _getApiConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_CONFIG_MAX_SIZE);
  
  if (!_readJson(&json, CONFIG_FILENAME))
  {
    res.sendStatus(204);
    return;
  }

  res.set("Content-Type", "application/json");
  serializeJson(json, res);
}

void _postApiConfig(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_CONFIG_MAX_SIZE);

  DeserializationError error = deserializeJson(json, req);
  if (error) 
  {
    res.sendStatus(500);
    return;
  }

  if (!_writeJson(&json, CONFIG_FILENAME))
  {
    res.sendStatus(500);
    return;
  }

  _setConfig(json.as<JsonVariant>());
  res.sendStatus(204);
}

void _postApiCommand(Request &req, Response &res)
{
  DynamicJsonDocument json(JSON_COMMAND_MAX_SIZE);

  DeserializationError error = deserializeJson(json, req);
  if (error) 
  {
    res.sendStatus(500);
    return;
  }

  _setCommand(json.as<JsonVariant>());
  res.sendStatus(204);
}

void _postApiRestart(Request &req, Response &res)
{
  restart = true;
  res.sendStatus(204);
}

void _postApiOta(Request &req, Response &res)
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

void _postApiResetWifi(Request &req, Response &res)
{
  if (!_eraseWifiCreds())
  {
    res.sendStatus(500);
    return;
  }
  
  restart = true;
  res.sendStatus(204);
}

void _postApiResetConfig(Request &req, Response &res)
{
  if (!_formatFS())
  {
    res.sendStatus(500);
    return;
  }

  restart = true;
  res.sendStatus(204);
}

void _postApiFactoryReset(Request &req, Response &res)
{
  if (!_eraseWifiCreds())
  {
    res.sendStatus(500);
    return;
  }

  if (!_formatFS())
  {
    res.sendStatus(500);
    return;
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

  DynamicJsonDocument mqtt(JSON_MQTT_MAX_SIZE);

  if (_readJson(&mqtt, MQTT_FILENAME))
  {
    _setMqtt(mqtt.as<JsonVariant>());
  }
  
  DynamicJsonDocument config(JSON_CONFIG_MAX_SIZE);

  if (_readJson(&config, CONFIG_FILENAME))
  {
    _setConfig(config.as<JsonVariant>());
  }
  
  _initialiseRestApi();
}

void OXRS_API::loop(Client * client)
{
  _checkRestart();
  
  if (client->connected())
  {
    _app.process(client);
    client->stop();
  }    
}

void OXRS_API::get(const char * path, Router::Middleware * middleware)
{
  _api.get(path, middleware);
}

void OXRS_API::post(const char * path, Router::Middleware * middleware)
{
  _api.post(path, middleware);
}

void OXRS_API::onAdopt(jsonCallback callback)
{
  _apiAdopt = callback;
}

JsonVariant OXRS_API::getAdopt(JsonVariant json)
{
  if (_apiAdopt)
  { 
    _apiAdopt(json);
  }
  
  return json;
}

void OXRS_API::_initialiseRestApi(void)
{
  // /api router
  _app.use("/api", &_api);
  
  // enable cors for all api requests
  _api.options(&_apiCorsOptions);
  _api.get(&_apiCors);
  _api.post(&_apiCors);
  _api.put(&_apiCors);

  // api endpoints
  _api.get("/adopt", &_getApiAdopt);

  _api.get("/mqtt", &_getApiMqtt);
  _api.post("/mqtt", &_postApiMqtt);

  _api.get("/config", &_getApiConfig);
  _api.post("/config", &_postApiConfig);

  _api.post("/command", &_postApiCommand);

  _api.post("/restart", &_postApiRestart);
  _api.post("/ota", &_postApiOta);

  _api.post("/resetWifi", &_postApiResetWifi);
  _api.post("/resetConfig", &_postApiResetConfig);
  _api.post("/factoryReset", &_postApiFactoryReset);
}

void OXRS_API::_checkRestart(void)
{
  if (restart) { ESP.restart(); }  
}
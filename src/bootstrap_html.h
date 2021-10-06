#ifndef bootstrap_html_h
#define bootstrap_html_h

const char BOOTSTRAP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>OXRS Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="expires" content="0">

  <style>

    body { background: #fff; color: #000; padding: 0; margin: 0; }
    html { font-family: Arial; }
    * { box-sizing: border-box; border-collapse: collapse; }

    @media (prefers-color-scheme: dark) { body { background: #000; color: #fff; } }

    label { display: block; width: 100%; margin-bottom: 1em; }
    input { display: block; width: 100%; margin-bottom: 1em; padding: 0.5em; outline: none; border-radius: .4em; }
    input:active, input:focus, button:hover, button:focus, button:active { box-shadow: 0 0 5px 5px #0a97d8; }

    form { padding: 1em; }

    @media only screen and (min-width: 768px) {
      form { padding: 0; margin: 0 auto; max-width: 40em; }
      label { display: inline-block; width: 49%; }
      input { display: inline-block; width: 49%; }
    }

    button { width: 100%; padding: 0.5em; cursor: pointer; }

    #state-text { background-color: grey; margin: 1em 0; padding: .25em .6em; border-radius: .4em; font-weight: bold; text-align: center; }
  </style>
</head>

<body onload="handleBodyLoad()">

<form id="mqtt-form">

  <div id="state-text">LOADING...</div>

  <label for="mqtt-broker">Broker:</label>
  <input type="text" name="broker" id="mqtt-broker">

  <label for="mqtt-port">Port:</label>
  <input type="text" name="port" id="mqtt-port" value="1883">

  <label for="mqtt-clientid">Client ID:</label>
  <input type="text" name="clientId" id="mqtt-clientid">

  <label for="mqtt-username">Username:</label>
  <input type="text" name="username" id="mqtt-username">

  <label for="mqtt-password">Password:</label>
  <input type="password" name="password" id="mqtt-password" disabled>

  <label for="mqtt-topic-prefix">Topic Prefix:</label>
  <input type="text" name="topicPrefix" id="mqtt-topic-prefix">

  <label for="mqtt-topic-suffix">Topic Suffix:</label>
  <input type="text" name="topicSuffix" id="mqtt-topic-suffix">

  <button type="submit">Submit</button>

</form>

<script>

var updateMqttConnectionStatusTimer;

function handleError(error)
{
  // Display the error message in RED
  document.getElementById("state-text").style.background = "red";
  document.getElementById("state-text").innerHTML = 'ERR: ' + error.message;
  
  // Stop the connection status timer so our error isn't cleared away
  clearTimeout(updateMqttConnectionStatusTimer);
}

function scheduleUpdateMqttConnectionStatus(time)
{
  // Stop any running timer
  clearTimeout(updateMqttConnectionStatusTimer);

  // Reschedule the timer
  updateMqttConnectionStatusTimer = setTimeout(function ()
  { 
    updateMqttConnectionStatus();
  }, time);
}

function handleBodyLoad()
{
  fetch('/mqtt')
    .then(response =>
    {
      if (!response.ok) 
        throw new Error("Device response was not ok");

      return response.json();
    })
    .then(data => 
    {
      document.getElementById('mqtt-broker').value = data.broker;
      document.getElementById('mqtt-port').value = data.port;
      document.getElementById('mqtt-clientid').value = data.clientId;
      document.getElementById('mqtt-username').value = data.username;
      document.getElementById('mqtt-topic-prefix').value = data.topicPrefix;
      document.getElementById('mqtt-topic-suffix').value = data.topicSuffix;

      // Trigger the username keyup event to ensure the password field
      // is enabled/disabled based on what we have loaded above
      document.getElementById('mqtt-username').dispatchEvent(new Event('keyup'));
    })
    .catch(error =>
    {
      handleError(error);
      return false;
    })

  // Initial connection status check
  scheduleUpdateMqttConnectionStatus(0);
}

function handleFormSubmit(event)
{
  event.preventDefault();

  // Validate input fields
  if (document.getElementById('mqtt-broker').value == "")
  {
    alert('Broker hostname or IP address is mandatory');
    return false;
  }
  
  if (document.getElementById('mqtt-clientid').value == "")
  {
    alert('Client ID is mandatory');
    return false;
  }
  
  if (document.getElementById('mqtt-username').value != "" && document.getElementById('mqtt-password').value == "")
  {
    alert('Password is mandatory if username specified');
    return false;
  }

  const form = event.currentTarget;
  var formData = new FormData(form);
  var formEntries = Object.fromEntries(formData.entries());

  fetch('/mqtt',
  {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(formEntries)
  })
    .then(response =>
    {
      if (!response.ok) 
        throw new Error("Device response was not ok");
    })
    .catch(error =>
    {
      handleError(error);
      return false;
    });

  // Schedule a connection status check
  scheduleUpdateMqttConnectionStatus(500);
}

function updateMqttConnectionStatus()
{
  fetch('/mqtt')
    .then(response =>
    {
      if (!response.ok) 
        throw new Error("Device response was not ok");

      return response.json();
    })
    .then(data =>
    {
      if (data.connected === true)
      {
        document.getElementById("state-text").style.background = "green";
        document.getElementById("state-text").innerHTML = "MQTT CONNECTED";
      }
      else
      {
        document.getElementById("state-text").style.background = "orange";
        document.getElementById("state-text").innerHTML = "MQTT DISCONNECTED";
      }
    })
    .catch(error =>
    {
      document.getElementById("state-text").style.background = "red";
      document.getElementById("state-text").innerHTML = "OFFLINE";
    });

  // Reschedule a connection status check every 5 secs
  scheduleUpdateMqttConnectionStatus(5000);
}

const mqttForm = document.getElementById("mqtt-form");
mqttForm.addEventListener("submit", handleFormSubmit);

const mqttUsername = document.getElementById('mqtt-username');
mqttUsername.addEventListener('keyup', (event) =>
{
  const mqttPassword = document.getElementById("mqtt-password");
  mqttPassword.removeAttribute("disabled");
  
  if (event.target.value == "")
  {
    mqttPassword.setAttribute("disabled", true);
    mqttPassword.value = "";
  }
});

</script>

</body>
</html>
)rawliteral";

#endif

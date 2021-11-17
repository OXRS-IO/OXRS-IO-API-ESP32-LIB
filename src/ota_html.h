#ifndef ota_html_h
#define ota_html_h

const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>OXRS OTA Update</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="expires" content="0">

  <style>

    body { padding: 0; margin: 0; font-family: Arial; }
    * { box-sizing: border-box; border-collapse: collapse; }

    label { display: block; width: 100%; margin-bottom: 0.5rem; }
    input { display: block; width: 100%; margin-bottom: 1rem; padding: 0.5rem; outline: none; border-radius: .4rem; border: 2px solid rgba(0,0,0,0.5); background-color: rgba(0,0,0,0.1); color: #000; }
    input:active, input:focus, button:hover, button:focus, button:active { box-shadow: 0 0 5px 5px #0a97d8; }

    form { padding: 1rem; }

    button { display: block; width: 100%; margin-bottom: 1rem; padding: 1rem; border-radius: .4rem; cursor: pointer; font-weight: bold; border: none; }
    input:disabled, button:disabled { cursor: not-allowed; }

    #submit { background-color: olivedrab; }

    @media only screen and (min-width: 768px) {
      form { padding: 0; margin: 0 auto; max-width: 40rem; }
      label { display: inline-block; width: 24%; }
      input { display: inline-block; width: 74%; }
      button { flex-grow: 1; }
      .btns { display: flex; flex-direction: row; gap: 1rem; }
    }

    @media (prefers-color-scheme: dark) {
      body { background: #1c1c1c; color: #fff; }
      input { color: #fff; border: 2px solid rgba(255,255,255,0.1); }
    }
  </style>
</head>

<body>

<form id="ota-form">

  <label for="ota-file">Firmware Binary:</label>
  <input type="file" name="file" id="ota-file" required>

  <div class="btns">
    <button id="submit" type="submit">Submit</button>
  </div>

</form>

<script>

function handleFormSubmit(event)
{
  event.preventDefault();
  
  const body = document.getElementById("ota-file").files[0];
  
  fetch('/ota', 
  { 
    method: "POST", 
    body 
  })
    .then((response) =>
    {
      if (!response.ok)
        return alert("File upload failed");
    
      alert("File upload succeeded, device will reboot");
    });
}

const otaForm = document.getElementById("ota-form");
otaForm.addEventListener("submit", handleFormSubmit);

</script>

</body>
</html>
)rawliteral";

#endif

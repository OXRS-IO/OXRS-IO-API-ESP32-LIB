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

    input[type="file"]:hover { cursor: pointer; }
    .btns { position: relative; }

    #submit { background-color: olivedrab; }

    #state-text { background-color: grey; margin: 1rem 0; padding: .25rem .6rem; border-radius: .4rem; font-weight: bold; text-align: center; }

    @media only screen and (min-width: 768px) {
      form { padding: 0; margin: 0 auto; max-width: 40rem; }
      label { display: inline-block; width: 24%; }
      input { display: inline-block; width: 74%; }
    }

    @media (prefers-color-scheme: dark) {
      body { background: #1c1c1c; color: #fff; }
      input { color: #fff; border: 2px solid rgba(255,255,255,0.1); }
    }
  </style>
</head>
<body>
<form id="ota-form" enctype="multipart/form-data" action="/ota">
  <h1>OXRS OTA Update</h1>

  <div id="state-text">READY</div>

  <label for="ota-file">Firmware binary:</label>
  <input type="file" name="file" id="ota-file" required>

  <div class="btns">
    <button id="submit" type="submit">Upload</button>
  </div>
</form>
<script>
function handleFormSubmit(event)
{
  event.preventDefault();

  let fileInput = document.getElementById('ota-file');
  let file = fileInput.files[0];
  if (!file) return false;

  let state = document.getElementById("state-text");  
  let btn = document.getElementById('submit');

  state.style.background = "orange";
  state.innerHTML = "UPLOADING...";
  
  btn.setAttribute("disabled", true);

  let request = new XMLHttpRequest();
  request.addEventListener("load", requestComplete);
  request.addEventListener('error', requestComplete);
  request.addEventListener('abort', requestComplete);

  function requestComplete(event)
  {    
    if (request.status !== 200 && request.status !== 204) 
    {
      state.style.background = "red";
      state.innerHTML = "UPLOAD FAILED";
      
      btn.removeAttribute("disabled");
    }
    else
    {
      state.style.background = "green";
      state.innerHTML = "UPLOAD COMPLETE";
    }
  }

  request.open('POST', otaForm.getAttribute('action'), true);
  request.send(fileInput.files[0]);
}

let otaForm = document.getElementById("ota-form");
otaForm.addEventListener("submit", handleFormSubmit);
</script>
</body>
</html>
)rawliteral";

#endif

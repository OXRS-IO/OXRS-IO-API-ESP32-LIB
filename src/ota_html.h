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
    body { padding: 2rem 0 0 0; margin: 0; font-family: Arial; }
    * { box-sizing: border-box; border-collapse: collapse; }
    label { display: block; width: 100%; margin-bottom: 0.5rem; }
    input { display: block; width: 100%; margin-bottom: 2rem; padding: 0.5rem; outline: none; border-radius: .4rem; border: 2px solid rgba(0,0,0,0.5); background-color: rgba(0,0,0,0.1); color: #000; }
    input:active, input:focus, button:hover, button:focus, button:active { box-shadow: 0 0 5px 5px #0a97d8; }
    input[type="file"]:hover { cursor: pointer; }
    form { padding: 1rem; }
    button { display: block; width: 100%; margin-bottom: 1rem; padding: 1rem; border-radius: .4rem; cursor: pointer; font-weight: bold; border: none; }
    input:disabled, button:disabled { cursor: not-allowed; }
    .btns { position: relative; }
    #status { background-color: rgba(0,0,0,0.5); content: ""; width: 0; height: 100%; position: absolute; left: 0; top: 0; }
    #submit { background-color: #6B8E23; color: #fff; }
    .uploading {-webkit-animation: pulse 2s ease-in-out infinite alternate; animation: pulse 2s ease-in-out infinite alternate; }
    @-webkit-keyframes pulse { 0% { background-color: #6B8E23; } 50% { background-color: #C3D1A5; } 100% { background-color: #6B8E23; }}
    @keyframes pulse { 0% { background-color: #6B8E23; } 50% { background-color: #C3D1A5; } 100% { background-color: #6B8E23; }}
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
  <label for="ota-file">Firmware binary:</label>
  <input type="file" name="file" id="ota-file" required>
  <div class="btns">
    <button id="submit" type="submit">Upload</button>
    <div id="status"></div>
  </div>
</form>
<script>
function handleFormSubmit(event)
{
  event.preventDefault();

  let fileInput = document.getElementById('ota-file');
  let file = fileInput.files[0];
  let request = new XMLHttpRequest();
  let btn = document.getElementById('submit');

  if (!file) return false;

  btn.setAttribute('class', 'uploading');
  btn.textContent = 'Uploading...';

  request.addEventListener("progress", requestUpdate);
  request.addEventListener("load", requestComplete);
  request.addEventListener('error', requestComplete);
  request.addEventListener('abort', requestComplete);

  function requestUpdate(event)
  {
    if (!event.lengthComputable) return;
    let percentComplete = event.loaded / event.total * 100;
    let status = document.getElementById('status');
    status.style.width = percentComplete + "%";
  }

  function requestComplete(event)
  {
    if (request.status !== 204) return alert('File upload failed!');
    alert('File upload succeeded! Device will reboot');
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

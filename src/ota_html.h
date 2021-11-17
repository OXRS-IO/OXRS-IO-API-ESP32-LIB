#ifndef ota_html_h
#define ota_html_h

const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body>
  <h1>
    Compiled:
  </h1>
  <form id='form'>
    <input id='file' type='file'>
    <input type='submit' value='Send' />
  </form>
</body>
<script>
  const form = document.getElementById('form');
  form.onsubmit = function(e) {
    e.preventDefault();
    const body = document.getElementById('file').files[0];
    fetch('/ota', { method: 'POST', body }).then((response) => {
      if (!response.ok) {
        return alert('File upload failed');
      }
      alert('File upload succeeded');
    });
  }
</script>
</html>
)rawliteral";

#endif

#pragma once

static const char *setupPortalHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>P61 Setup</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, sans-serif;
      background: linear-gradient(135deg, #6B4226 0%, #3E2723 100%);
      min-height: 100vh;
      padding: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .container {
      background: white;
      border-radius: 16px;
      padding: 32px;
      max-width: 480px;
      width: 100%;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    }
    h1 { color: #3E2723; margin-bottom: 8px; font-size: 24px; }
    .subtitle { color: #6B4226; margin-bottom: 24px; font-size: 14px; }
    .status {
      background: #FFF3E0;
      border: 1px solid #FF6F00;
      border-radius: 8px;
      padding: 12px;
      margin-bottom: 24px;
      color: #E65100;
      font-size: 14px;
    }
    label { display: block; margin-bottom: 8px; color: #374151; font-weight: 500; font-size: 14px; }
    input, select {
      width: 100%;
      padding: 12px;
      border: 2px solid #e5e7eb;
      border-radius: 8px;
      font-size: 16px;
      margin-bottom: 16px;
    }
    input:focus, select:focus { outline: none; border-color: #6B4226; }
    button {
      width: 100%;
      padding: 14px;
      background: linear-gradient(135deg, #6B4226 0%, #3E2723 100%);
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
    }
    button:hover { opacity: 0.9; }
    .scan-btn { background: #6b7280; margin-bottom: 16px; }
    .loading { display: none; text-align: center; padding: 20px; }
    .spinner {
      border: 3px solid #f3f4f6;
      border-top: 3px solid #6B4226;
      border-radius: 50%;
      width: 40px;
      height: 40px;
      animation: spin 1s linear infinite;
      margin: 0 auto 12px;
    }
    @keyframes spin { 100% { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <div class=\"container\">
    <h1>☕ P61 Setup</h1>
    <p class=\"subtitle\">%SERIAL_NUMBER%</p>
    <div class=\"status\">📡 Setup mode active</div>
    <button class=\"scan-btn\" onclick=\"scanNetworks()\">🔍 Scan Networks</button>
    <form id=\"wifiForm\" onsubmit=\"return connectWiFi(event)\">
      <label>WiFi Network</label>
      <select id=\"ssid\" name=\"ssid\" required>
        <option value=\"\">Select network...</option>
      </select>
      <label>Password</label>
      <input type=\"password\" id=\"password\" name=\"password\">
      <button type=\"submit\">Connect</button>
    </form>
    <div class=\"loading\" id=\"loading\">
      <div class=\"spinner\"></div>
      <p>Connecting...</p>
    </div>
  </div>
  <script>
    async function scanNetworks() {
      const select = document.getElementById('ssid');
      select.innerHTML = '<option>Scanning...</option>';
      const response = await fetch('/scan');
      const networks = await response.json();
      select.innerHTML = '<option value=\"\">Select network...</option>';
      networks.forEach(n => {
        const option = document.createElement('option');
        option.value = n.ssid;
        option.textContent = `${n.rssi > -50 ? '📶' : '📶'} ${n.ssid} ${n.secure ? '🔒' : ''}`;
        select.appendChild(option);
      });
    }
    async function connectWiFi(event) {
      event.preventDefault();
      document.getElementById('wifiForm').style.display = 'none';
      document.getElementById('loading').style.display = 'block';
      const formData = new FormData(event.target);
      const response = await fetch('/connect', { method: 'POST', body: formData });
      const result = await response.json();
      if (result.success) {
        document.getElementById('loading').innerHTML = '<div class=\"status\">✅ Connected!</div>';
      } else {
        alert('Connection failed');
        location.reload();
      }
    }
    window.onload = () => scanNetworks();
  </script>
</body>
</html>
)rawliteral";

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

const char* AP_SSID = "ServoControl";
const char* AP_PASS = "servo1234";

#define RIGHT_SERVO 0
#define LEFT_SERVO  1
#define SERVO_MIN   65
#define SERVO_MAX   105
#define SERVO_MID   90
#define MAX_DEG     20

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int angleToPwm(int angle) {
  int pulse_us = 500 + (int)((angle / 180.0) * 2000);
  return (int)((pulse_us / 20000.0) * 4096);
}

void setFromJoystick(float joyX, float joyY) {
  float pitch = joyY * MAX_DEG;
  float roll  = -joyX * MAX_DEG;  // flipped to match head orientation

  int right_angle = constrain((int)(SERVO_MID + pitch + roll), SERVO_MIN, SERVO_MAX);
  int left_angle  = constrain((int)(SERVO_MID - pitch + roll), SERVO_MIN, SERVO_MAX);

  pwm.setPWM(RIGHT_SERVO, 0, angleToPwm(right_angle));
  pwm.setPWM(LEFT_SERVO,  0, angleToPwm(left_angle));
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_TEXT) {
      data[len] = '\0';
      // Message format: "x,y"  e.g. "0.45,-0.30"
      float x = 0, y = 0;
      sscanf((char*)data, "%f,%f", &x, &y);
      x = constrain(x, -1.0f, 1.0f);
      y = constrain(y, -1.0f, 1.0f);
      setFromJoystick(x, y);
    }
  }
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>Tilt Control</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: sans-serif;
      background: #1a1a2e;
      color: #eee;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      user-select: none;
      touch-action: none;
    }
    h2 { margin-bottom: 36px; font-size: 1.4rem; letter-spacing: 1px; }
    #zone {
      position: relative;
      width: 280px;
      height: 280px;
      border-radius: 50%;
      background: #16213e;
      border: 2px solid #e94560;
      touch-action: none;
    }
    #zone::before, #zone::after {
      content: '';
      position: absolute;
      background: #ffffff18;
    }
    #zone::before { width: 1px; height: 100%; left: 50%; }
    #zone::after  { width: 100%; height: 1px; top: 50%; }
    #knob {
      position: absolute;
      width: 70px;
      height: 70px;
      border-radius: 50%;
      background: radial-gradient(circle at 35% 35%, #ff6b81, #e94560);
      box-shadow: 0 0 20px #e9456066;
      transform: translate(-50%, -50%);
      left: 50%; top: 50%;
      pointer-events: none;
    }
    #status {
      margin-top: 16px;
      font-size: 0.8rem;
      color: #aaa;
    }
    #status.connected { color: #4caf50; }
    #info {
      margin-top: 12px;
      font-size: 0.95rem;
      color: #aaa;
      text-align: center;
      line-height: 1.8;
    }
    #vals { color: #e94560; font-weight: bold; }
  </style>
</head>
<body>
  <h2>Tilt Control</h2>
  <div id="zone"><div id="knob"></div></div>
  <div id="status">Connecting...</div>
  <div id="info">
    <div id="vals">X: 0.00 &nbsp; Y: 0.00</div>
    <span id="angles">Right: 90° &nbsp; Left: 90°</span>
  </div>

  <script>
    const zone   = document.getElementById('zone');
    const knob   = document.getElementById('knob');
    const vals   = document.getElementById('vals');
    const angles = document.getElementById('angles');
    const status = document.getElementById('status');

    const R     = zone.offsetWidth / 2;
    const KR    = 35;
    const MAX_R = R - KR;
    const MAX_DEG = 20;

    // WebSocket
    let ws;
    function connectWS() {
      ws = new WebSocket('ws://' + location.hostname + '/ws');
      ws.onopen  = () => { status.textContent = 'Connected'; status.className = 'connected'; };
      ws.onclose = () => { status.textContent = 'Disconnected — retrying...'; status.className = ''; setTimeout(connectWS, 1000); };
    }
    connectWS();

    let active = false;
    let cx = 0, cy = 0;
    let lastX = 0, lastY = 0;
    let rafId = null;

    function getCenter() {
      const rect = zone.getBoundingClientRect();
      cx = rect.left + rect.width  / 2;
      cy = rect.top  + rect.height / 2;
    }

    function sendLoop() {
      if (!active) return;
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(lastX.toFixed(3) + ',' + lastY.toFixed(3));
      }
      rafId = requestAnimationFrame(sendLoop);
    }

    function update(px, py) {
      let dx = px - cx;
      let dy = py - cy;
      const dist = Math.sqrt(dx*dx + dy*dy);
      if (dist > MAX_R) { dx = dx/dist*MAX_R; dy = dy/dist*MAX_R; }

      knob.style.left = (R + dx) + 'px';
      knob.style.top  = (R + dy) + 'px';

      lastX =  dx / MAX_R;
      lastY = -dy / MAX_R;

      const right = Math.round(90 + lastY * MAX_DEG - lastX * MAX_DEG);
      const left  = Math.round(90 - lastY * MAX_DEG - lastX * MAX_DEG);
      vals.innerHTML = `X: ${lastX.toFixed(2)} &nbsp; Y: ${lastY.toFixed(2)}`;
      angles.textContent = `Right: ${right}°   Left: ${left}°`;
    }

    function release() {
      if (!active) return;
      active = false;
      cancelAnimationFrame(rafId);
      knob.style.transition = 'left 0.2s, top 0.2s';
      knob.style.left = '50%';
      knob.style.top  = '50%';
      if (ws && ws.readyState === WebSocket.OPEN) ws.send('0,0');
      vals.innerHTML = 'X: 0.00 &nbsp; Y: 0.00';
      angles.textContent = 'Right: 90°   Left: 90°';
    }

    zone.addEventListener('mousedown', e => {
      active = true;
      knob.style.transition = 'none';
      getCenter(); update(e.clientX, e.clientY);
      rafId = requestAnimationFrame(sendLoop);
    });
    window.addEventListener('mousemove', e => { if (active) update(e.clientX, e.clientY); });
    window.addEventListener('mouseup', release);

    zone.addEventListener('touchstart', e => {
      e.preventDefault(); active = true;
      knob.style.transition = 'none';
      getCenter(); update(e.touches[0].clientX, e.touches[0].clientY);
      rafId = requestAnimationFrame(sendLoop);
    }, { passive: false });
    window.addEventListener('touchmove', e => {
      e.preventDefault();
      if (active) update(e.touches[0].clientX, e.touches[0].clientY);
    }, { passive: false });
    window.addEventListener('touchend', release);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  pwm.setPWM(RIGHT_SERVO, 0, angleToPwm(SERVO_MID));
  pwm.setPWM(LEFT_SERVO,  0, angleToPwm(SERVO_MID));
  Serial.println("Servos at 90.");

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html", INDEX_HTML);
  });
  server.begin();
  Serial.println("WebSocket server started.");
}

void loop() {
  ws.cleanupClients();
}

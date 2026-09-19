#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

struct MOTOR_PINS
{
  int pinEn;  
  int pinIN1;
  int pinIN2;    
};

std::vector<MOTOR_PINS> motorPins = 
{
  {12, 13, 15},  //RIGHT_MOTOR Pins (EnA, IN1, IN2)
  {12, 14, 2},   //LEFT_MOTOR  Pins (EnB, IN3, IN4)
};
#define LIGHT_PIN 4

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1

#define FORWARD 1
#define BACKWARD -1

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;
const int PWMLightChannel = 3;

//Camera related constants
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid     = "Cam_Car";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <title>RoboBillu Rover</title>
    <style>
      body {
        background-color: #121212;
        color: #e0e0e0;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        text-align: center;
        margin: 0;
        padding: 20px 10px;
        overflow-x: hidden;
      }
      .noselect {
        -webkit-touch-callout: none; -webkit-user-select: none;
        -khtml-user-select: none; -moz-user-select: none;
        -ms-user-select: none; user-select: none;
      }
      .container {
        max-width: 450px;
        margin: auto;
      }
      .brand-title {
        font-size: 24px;
        font-weight: 700;
        color: #4caf50;
        margin-bottom: 15px;
        letter-spacing: 1px;
      }
      #cameraImage {
        width: 100%;
        max-width: 400px;
        height: auto;
        aspect-ratio: 4/3;
        background-color: #222;
        border-radius: 12px;
        box-shadow: 0 8px 20px rgba(0,0,0,0.6);
        margin-bottom: 25px;
        object-fit: cover;
      }
      
      /* Modern D-Pad */
      .d-pad {
        display: grid;
        grid-template-columns: repeat(3, 75px);
        grid-gap: 12px;
        justify-content: center;
        margin-bottom: 35px;
      }
      .btn {
        background: #2a2a35;
        border: 2px solid #3e3e50;
        border-radius: 16px;
        height: 75px;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 32px;
        color: #fff;
        cursor: pointer;
        transition: all 0.1s ease;
        box-shadow: 0 6px 10px rgba(0,0,0,0.4);
      }
      .btn:active {
        background: #4caf50;
        border-color: #4caf50;
        transform: translateY(4px);
        box-shadow: 0 2px 4px rgba(0,0,0,0.4);
      }
      .empty { background: transparent; border: none; box-shadow: none; pointer-events: none; }
      
      /* Sliders */
      .control-group {
        background: #1e1e26;
        padding: 15px 20px;
        border-radius: 12px;
        margin-bottom: 15px;
        text-align: left;
        box-shadow: 0 4px 8px rgba(0,0,0,0.3);
      }
      .control-header {
        display: flex;
        justify-content: space-between;
        font-weight: 600;
        margin-bottom: 12px;
        font-size: 15px;
        color: #a0a0b0;
      }
      .val-display { color: #4caf50; font-weight: bold; }
      
      input[type=range] {
        -webkit-appearance: none;
        width: 100%;
        background: transparent;
      }
      input[type=range]::-webkit-slider-thumb {
        -webkit-appearance: none;
        height: 26px;
        width: 26px;
        border-radius: 50%;
        background: #4caf50;
        cursor: pointer;
        margin-top: -9px;
        box-shadow: 0 0 10px rgba(76, 175, 80, 0.6);
      }
      input[type=range]::-webkit-slider-runnable-track {
        width: 100%;
        height: 8px;
        cursor: pointer;
        background: #444;
        border-radius: 4px;
      }
      input[type=range]:focus { outline: none; }
    </style>
  </head>
  <body class="noselect">
    <div class="container">
      <div class="brand-title">RoboBillu Rover Dashboard</div>
      
      <img id="cameraImage" src="">
      
      <div class="d-pad">
        <div class="empty"></div>
        <div class="btn" ontouchstart='sendMove("1")' onmousedown='sendMove("1")' ontouchend='sendMove("0")' onmouseup='sendMove("0")' onmouseleave='sendMove("0")'>&#8679;</div>
        <div class="empty"></div>
        <div class="btn" ontouchstart='sendMove("3")' onmousedown='sendMove("3")' ontouchend='sendMove("0")' onmouseup='sendMove("0")' onmouseleave='sendMove("0")'>&#8678;</div>
        <div class="empty"></div>
        <div class="btn" ontouchstart='sendMove("4")' onmousedown='sendMove("4")' ontouchend='sendMove("0")' onmouseup='sendMove("0")' onmouseleave='sendMove("0")'>&#8680;</div>
        <div class="empty"></div>
        <div class="btn" ontouchstart='sendMove("2")' onmousedown='sendMove("2")' ontouchend='sendMove("0")' onmouseup='sendMove("0")' onmouseleave='sendMove("0")'>&#8681;</div>
        <div class="empty"></div>
      </div>

      <div class="control-group">
        <div class="control-header"><span>SPEED</span> <span id="speedVal" class="val-display">150</span></div>
        <input type="range" min="0" max="255" value="150" id="Speed" oninput='throttledSliderUpdate("Speed", this.value)'>
      </div>

      <div class="control-group">
        <div class="control-header"><span>HEADLIGHT</span> <span id="lightVal" class="val-display">0</span></div>
        <input type="range" min="0" max="255" value="0" id="Light" oninput='throttledSliderUpdate("Light", this.value)'>
      </div>
    </div>
  
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCamera;
      var websocketCarInput;
      var currentBlobUrl = null;
      
      var lastSendTime = 0;
      var sendTimeout = null;
      
      function initCameraWebSocket() {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onclose = function(){ setTimeout(initCameraWebSocket, 2000); };
        websocketCamera.onmessage = function(event) {
          var imageId = document.getElementById("cameraImage");
          if (currentBlobUrl) URL.revokeObjectURL(currentBlobUrl); 
          currentBlobUrl = URL.createObjectURL(event.data);
          imageId.src = currentBlobUrl;
        };
      }
      
      function initCarInputWebSocket() {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen = function() {
          sendButtonInput("Speed", document.getElementById("Speed").value);
          sendButtonInput("Light", document.getElementById("Light").value);
        };
        websocketCarInput.onclose = function(){ setTimeout(initCarInputWebSocket, 2000); };
      }
      
      function initWebSocket() {
        initCameraWebSocket();
        initCarInputWebSocket();
      }

      function sendButtonInput(key, value) {
        if (websocketCarInput && websocketCarInput.readyState === WebSocket.OPEN) {
          websocketCarInput.send(key + "," + value);
        }
      }

      function sendMove(dir) {
        sendButtonInput("MoveCar", dir);
      }

      function throttledSliderUpdate(key, value) {
        document.getElementById(key === 'Speed' ? 'speedVal' : 'lightVal').innerText = value;
        var now = Date.now();
        if (now - lastSendTime >= 50) {
          sendButtonInput(key, value);
          lastSendTime = now;
        } else {
          clearTimeout(sendTimeout);
          sendTimeout = setTimeout(() => {
            sendButtonInput(key, value);
            lastSendTime = Date.now();
          }, 50);
        }
      }
    
      window.onload = initWebSocket;
      document.querySelectorAll('.btn').forEach(b => {
        b.addEventListener('touchstart', e => e.preventDefault());
      });
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);    
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);     
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);       
  }
}

void moveCar(int inputValue)
{
  switch(inputValue)
  {
    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);                  
      break;
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
    case LEFT:
      // SWAPPED: Right motor backwards, Left motor forwards
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);  
      break;
    case RIGHT:
      // SWAPPED: Right motor forwards, Left motor backwards
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD); 
      break;
    case STOP:
    default:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
  }
}

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server, 
                              AsyncWebSocketClient *client, 
                              AwsEventType type,
                              void *arg, 
                              uint8_t *data, 
                              size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      moveCar(STOP);
      ledcWrite(PWMLightChannel, 0);  
      break;
    case WS_EVT_DATA:
    { 
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        // OPTIMIZED: Replaced heavy std::string with lightweight C-style string parsing
        char buffer[32]; // Small stack buffer
        if (len < sizeof(buffer)) {
          memcpy(buffer, data, len);
          buffer[len] = '\0'; // Null-terminate
          
          char* key = strtok(buffer, ",");
          char* valueStr = strtok(NULL, ",");
          
          if (key != NULL && valueStr != NULL) {
            int valueInt = atoi(valueStr);
            
            if (strcmp(key, "MoveCar") == 0) {
              moveCar(valueInt);        
            }
            else if (strcmp(key, "Speed") == 0) {
              ledcWrite(PWMSpeedChannel, valueInt);
            }
            else if (strcmp(key, "Light") == 0) {
              ledcWrite(PWMLightChannel, valueInt);         
            } 
          }
        }
      }
      break;
    } 
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, 
                            AsyncWebSocketClient *client, 
                            AwsEventType type,
                            void *arg, 
                            uint8_t *data, 
                            size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      cameraClientId = 0;
      break;
    default:
      break;  
  }
}

void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  config.frame_size = FRAMESIZE_QVGA; 
  config.jpeg_quality = 12;

  if (psramFound()) {
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  

  sensor_t * s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);   
    s->set_hmirror(s, 1); 
  }
}

void sendCameraPicture()
{
  if (cameraClientId == 0) return;

  static unsigned long lastFrameTime = 0;
  if (millis() - lastFrameTime < 50) return; 

  AsyncWebSocketClient *client = wsCamera.client(cameraClientId);
  if (!client || client->queueIsFull() || client->queueLen() > 0) return; 

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
  
  lastFrameTime = millis();
}

void setUpPinModes()
{
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);
      
  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinEn, OUTPUT);    
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);  
    ledcAttachPin(motorPins[i].pinEn, PWMSpeedChannel);
  }
  
  ledcWrite(PWMSpeedChannel, 150);
  
  moveCar(STOP);

  pinMode(LIGHT_PIN, OUTPUT);    
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
  ledcWrite(PWMLightChannel, 0); 
}

void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);
 
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  setupCamera();
}

void loop() 
{
  wsCamera.cleanupClients(); 
  wsCarInput.cleanupClients(); 
  sendCameraPicture(); 
}
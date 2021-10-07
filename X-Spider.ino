#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define BLUE_LED 2

Preferences preferences;
StaticJsonDocument<100> docReceive;

// Replace with your network credentials
const char* ssid = "MW325R";
const char* password = "xushuai821";

#include <Adafruit_NeoPixel.h>

// Which pin on the ESP32 is connected to the NeoPixels?
#define PIN         23

// How many NeoPixels LEDs are attached to the ESP32?
#define NUMPIXELS   5

// We define birghtness of NeoPixel LEDs
#define BRIGHTNESS  255

Adafruit_NeoPixel matrix = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const char* PARAM_INPUT_0 = "typeCmd"; //1:moveCtrl,0:debug
const char* PARAM_INPUT_1 = "cmd01";
const char* PARAM_INPUT_2 = "cmd02";
const char* PARAM_INPUT_3 = "cmd03";

const int pwm_0 = 32;
const int pwm_1 = 33;
const int pwm_2 = 25;
const int pwm_3 = 26;

const int pwm_4 = 27;
const int pwm_5 = 14;
const int pwm_6 = 13;
const int pwm_7 = 19;

const int pwm_8 = 18;
const int pwm_9 = 17;
const int pwm_10 = 16;
const int pwm_11 = 4;

int loopMode = 0; // 0:debug, 1:ctrlLoop

int freq = 50;      // 频率(20ms周期)
int resolution = 12; // 分辨率
const int led = 32;

int InitAngle[12]={90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90};
float GoalAngle[12]={90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90};
float NowAngle[12] ={90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90};
int fbRang = 15;
int hRang  = 35;
int xsteps = 0;
bool stopFlag = false;
int loopDelay = 1;
float deStep = 0.003;

int globalLeft = 0;
int globalRight = 0;

int MaxOffset = 20;

void servosSetup(){
  for(int i=0;i<12;i++){
    ledcSetup(i, freq, resolution);
  }
//  ledcSetup(12, freq, resolution);
  
  ledcAttachPin(pwm_0, 0);
  ledcAttachPin(pwm_1, 1);
  ledcAttachPin(pwm_2, 2);
  ledcAttachPin(pwm_3, 3);
  
  ledcAttachPin(pwm_4, 4);
  ledcAttachPin(pwm_5, 5);
  ledcAttachPin(pwm_6, 6);
  ledcAttachPin(pwm_7, 7);

  ledcAttachPin(pwm_8, 8);
  ledcAttachPin(pwm_9, 9);
  ledcAttachPin(pwm_10, 10);
  ledcAttachPin(pwm_11, 11);
}


void servosCtrl(int commandInput){
  for(int i = 0; i<12; i++){
    ledcWrite(i, calculatePWM(commandInput));
  }
//  ledcWrite(12, calculatePWM(commandInput));
}


int calculatePWM(float degree){ //0-180度
 //20ms周期，高电平0.5-2.5ms，对应0-180度角度
  const float deadZone = 6.4;//对应0.5ms（0.5ms/(20ms/256）)
  const float maxR = 32;//对应2.5ms
  if (degree < 0)
    degree = 0;
  if (degree > 180)
    degree = 180;
//  Serial.println((int)(((maxR - deadZone) / 180) * degree + deadZone)*4);
  return (int)((((maxR - deadZone) / 180) * degree + deadZone)*16);
}


int moveInit(){
  for(int i = 0; i<12; i++){
    ledcWrite(i, calculatePWM(InitAngle[i]));
  }
//  ledcWrite(12, calculatePWM(InitAngle[1]));
}


int moveGoal(){
  for(int i = 0; i<12; i++){
    ledcWrite(i, calculatePWM(GoalAngle[i]));
  }
//  ledcWrite(12, calculatePWM(GoalAngle[1]));
}


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    table { margin-left: auto; margin-right: auto; }
    td { padding: 8 px; }
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .button {
      background-color: #2f4468;
      border: none;
      color: white;
      padding: 10px 20px;
      text-align: center;
      text-decoration: none;
      display: inline-block;
      font-size: 18px;
      margin: 6px 3px;
      cursor: pointer;
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      -khtml-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
      -webkit-tap-highlight-color: rgba(0,0,0,0);
    }
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <table>
    <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox(1, 1, 1, 0);" ontouchstart="toggleCheckbox(1, 1, 1, 0);" onmouseup="toggleCheckbox(1, 0, 0, 0);" ontouchend="toggleCheckbox(1, 0, 0, 0);">Forward</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(1, -1, 1, 0);" ontouchstart="toggleCheckbox(1, -1, 1, 0);" onmouseup="toggleCheckbox(1, 0, 0, 0);" ontouchend="toggleCheckbox(1, 0, 0, 0);">Left</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(1, 0, 0, 0);" ontouchstart="toggleCheckbox(1, 0, 0, 0);">Stop</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(1, 1, -1, 0);" ontouchstart="toggleCheckbox(1, 1, -1, 0);" onmouseup="toggleCheckbox(1, 0, 0, 0);" ontouchend="toggleCheckbox(1, 0, 0, 0);">Right</button></td></tr>
    <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox(1, -1, -1, 0);" ontouchstart="toggleCheckbox(1, -1, -1, 0);" onmouseup="toggleCheckbox(1, 0, 0, 0);" ontouchend="toggleCheckbox(1, 0, 0, 0);">Backward</button></td></tr>                   
  </table>
  <table>
    <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox(4, 0, 0, 0);" ontouchstart="toggleCheckbox(4, 0, 0, 0);">Steady</button></td></tr>
    <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox(2, 0, 0, 0);" ontouchstart="toggleCheckbox(2, 0, 0, 0);">MoveInit</button></td></tr>
    <tr><td colspan="3" align="center"><button class="button" onmousedown="toggleCheckbox(3, 0, 0, 0);" ontouchstart="toggleCheckbox(3, 0, 0, 0);">Move90</button></td></tr>
  </table>
  <table>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 0, 0);" ontouchstart="toggleCheckbox(0, -1, 0, 0);">PWM0-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 0, 0);" ontouchstart="toggleCheckbox(0, 1, 0, 0);">PWM0+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 0, 1);" ontouchstart="toggleCheckbox(0, 0, 0, 1);">PWM0_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 1, 0);" ontouchstart="toggleCheckbox(0, -1, 1, 0);">PWM1-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 1, 0);" ontouchstart="toggleCheckbox(0, 1, 1, 0);">PWM1+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 1, 1);" ontouchstart="toggleCheckbox(0, 0, 1, 1);">PWM1_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 2, 0);" ontouchstart="toggleCheckbox(0, -1, 2, 0);">PWM2-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 2, 0);" ontouchstart="toggleCheckbox(0, 1, 2, 0);">PWM2+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 2, 1);" ontouchstart="toggleCheckbox(0, 0, 2, 1);">PWM2_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 3, 0);" ontouchstart="toggleCheckbox(0, -1, 3, 0);">PWM3-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 3, 0);" ontouchstart="toggleCheckbox(0, 1, 3, 0);">PWM3+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 3, 1);" ontouchstart="toggleCheckbox(0, 0, 3, 1);">PWM3_SET</button></td></tr>

    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 4, 0);" ontouchstart="toggleCheckbox(0, -1, 4, 0);">PWM4-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 4, 0);" ontouchstart="toggleCheckbox(0, 1, 4, 0);">PWM4+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 4, 1);" ontouchstart="toggleCheckbox(0, 0, 4, 1);">PWM4_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 5, 0);" ontouchstart="toggleCheckbox(0, -1, 5, 0);">PWM5-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 5, 0);" ontouchstart="toggleCheckbox(0, 1, 5, 0);">PWM5+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 5, 1);" ontouchstart="toggleCheckbox(0, 0, 5, 1);">PWM5_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 6, 0);" ontouchstart="toggleCheckbox(0, -1, 6, 0);">PWM6-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 6, 0);" ontouchstart="toggleCheckbox(0, 1, 6, 0);">PWM6+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 6, 1);" ontouchstart="toggleCheckbox(0, 0, 6, 1);">PWM6_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 7, 0);" ontouchstart="toggleCheckbox(0, -1, 7, 0);">PWM7-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1, 7, 0);" ontouchstart="toggleCheckbox(0, 1, 7, 0);">PWM7+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 7, 1);" ontouchstart="toggleCheckbox(0, 0, 7, 1);">PWM7_SET</button></td></tr>

    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 8, 0);" ontouchstart="toggleCheckbox(0, -1, 8, 0);">PWM8-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1,  8, 0);" ontouchstart="toggleCheckbox(0, 1, 8, 0);">PWM8+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 8, 1);" ontouchstart="toggleCheckbox(0, 0, 8, 1);">PWM8_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1, 9, 0);" ontouchstart="toggleCheckbox(0, -1, 9, 0);">PWM9-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1,  9, 0);" ontouchstart="toggleCheckbox(0, 1, 9, 0);">PWM9+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0, 9, 1);" ontouchstart="toggleCheckbox(0, 0, 9, 1);">PWM9_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1,10, 0);" ontouchstart="toggleCheckbox(0, -1,10, 0);">PWM10-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1,10, 0);" ontouchstart="toggleCheckbox(0, 1,10, 0);">PWM10+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0,10, 1);" ontouchstart="toggleCheckbox(0, 0,10, 1);">PWM10_SET</button></td></tr>
    <tr><td align="center"><button class="button" onmousedown="toggleCheckbox(0, -1,11, 0);" ontouchstart="toggleCheckbox(0, -1,11, 0);">PWM11-</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 1,11, 0);" ontouchstart="toggleCheckbox(0, 1,11, 0);">PWM11+</button></td><td align="center"><button class="button" onmousedown="toggleCheckbox(0, 0,11, 1);" ontouchstart="toggleCheckbox(0, 0,11, 1);">PWM11_SET</button></td></tr>
  </table>
<script>function toggleCheckbox(typeInput, cmdIn1, cmdIn2, cmdIn3) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?typeCmd="+typeInput+"&cmd01="+cmdIn1+"&cmd02="+cmdIn2+"&cmd03="+cmdIn3, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";


// Replaces placeholder with button section in your web page %BUTTONPLACEHOLDER%
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}


String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}


void wifiCheck(){
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");

  for(int i = 0; i<12; i++){
    ledcWrite(i, calculatePWM(InitAngle[i]));
    Serial.print("Servo ID: ");
    Serial.print(i);
    Serial.print(" Moving to InitPosition: ");
    Serial.println(InitAngle[i]);
    delay(400);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
}


int rangCtrl(int rawData){
  if (rawData > 90 + MaxOffset){return 90 + MaxOffset;}
  else if (rawData < 90 - MaxOffset){return 90 - MaxOffset;}
  else{return rawData;}
}


void webCtrlServer(){
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage0;
    String inputMessage1;
    String inputMessage2;
    String inputMessage3;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage0 = request->getParam(PARAM_INPUT_0)->value();
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputMessage3 = request->getParam(PARAM_INPUT_3)->value();

      if(inputMessage0.toInt()==1){
        loopMode = 1;
        globalLeft  = inputMessage1.toInt();
        globalRight = inputMessage2.toInt();
      }
      else if(inputMessage0.toInt()==0){
        loopMode = 0;
        delay(100);
        int angleCmd;
        int activeServo;
        int angleSet;
        
        angleCmd = inputMessage1.toInt();
        activeServo = inputMessage2.toInt();
        angleSet = inputMessage3.toInt();
        
        if(angleSet == 0){
          InitAngle[activeServo] += angleCmd;
          ledcWrite(activeServo, calculatePWM(InitAngle[activeServo]));

          Serial.print("ServoID: ");
          Serial.print(activeServo);
          Serial.print("   D-PWM: ");
          Serial.print(angleCmd);
          Serial.print("   Now-PWM: ");
          Serial.print(InitAngle[activeServo]);
          Serial.println("");
        }
        else if(angleSet == 1){
          Serial.print("ServoID: ");
          Serial.print(activeServo);
          Serial.print("   SET: ");
          Serial.print(InitAngle[activeServo]);
          Serial.println("");

          if(activeServo == 0){preferences.putInt("PWM0", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 1){preferences.putInt("PWM1", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 2){preferences.putInt("PWM2", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 3){preferences.putInt("PWM3", rangCtrl(InitAngle[activeServo]));}

          else if(activeServo == 4){preferences.putInt("PWM4", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 5){preferences.putInt("PWM5", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 6){preferences.putInt("PWM6", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 7){preferences.putInt("PWM7", rangCtrl(InitAngle[activeServo]));}

          else if(activeServo == 8){preferences.putInt("PWM8", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 9){preferences.putInt("PWM9", rangCtrl(InitAngle[activeServo]));} 
          else if(activeServo == 10){preferences.putInt("PWM10", rangCtrl(InitAngle[activeServo]));}
          else if(activeServo == 11){preferences.putInt("PWM11", rangCtrl(InitAngle[activeServo]));}

          InitAngle[activeServo] = rangCtrl(InitAngle[activeServo]);
          Serial.print("Print: ");
          Serial.println(InitAngle[activeServo]);
        }
      }
      else if(inputMessage0.toInt()==2){
        Serial.println("Move to InitPOS.");
        moveInit();
      }
      else if(inputMessage0.toInt()==3){
        Serial.println("Move to 90.");
        servosCtrl(90);
      }
    }
    request->send(200, "text/plain", "OK");
  });   

  // Start server
  server.begin();
}
 

void initAngleUpdate(){
  InitAngle[0] = preferences.getInt("PWM0", 90);
  InitAngle[1] = preferences.getInt("PWM1", 90);
  InitAngle[2] = preferences.getInt("PWM2", 90);
  InitAngle[3] = preferences.getInt("PWM3", 90);

  InitAngle[4] = preferences.getInt("PWM4", 90);
  InitAngle[5] = preferences.getInt("PWM5", 90);
  InitAngle[6] = preferences.getInt("PWM6", 90);
  InitAngle[7] = preferences.getInt("PWM7", 90);

  InitAngle[8] = preferences.getInt("PWM8", 90);
  InitAngle[9] = preferences.getInt("PWM9", 90);
  InitAngle[10] = preferences.getInt("PWM10", 90);
  InitAngle[11] = preferences.getInt("PWM11", 90);
}


void stepsInput(int leftDirection, int rightDirection, int stepInput, float smoothRate){
  if(stepInput == 0){
    legCtrl(1, 0, leftDirection, smoothRate);
    legCtrl(2, 2, leftDirection, smoothRate);
    legCtrl(3, 0, leftDirection, smoothRate);

    legCtrl(4, 2, rightDirection, smoothRate);
    legCtrl(5, 0, rightDirection, smoothRate);
    legCtrl(6, 2, rightDirection, smoothRate);
  }
  else if(stepInput == 1){
    legCtrl(1, 1, leftDirection, smoothRate);
    legCtrl(2, 3, leftDirection, smoothRate);
    legCtrl(3, 1, leftDirection, smoothRate);

    legCtrl(4, 3, rightDirection, smoothRate);
    legCtrl(5, 1, rightDirection, smoothRate);
    legCtrl(6, 3, rightDirection, smoothRate);
  }
  else if(stepInput == 2){
    legCtrl(1, 2, leftDirection, smoothRate);
    legCtrl(2, 0, leftDirection, smoothRate);
    legCtrl(3, 2, leftDirection, smoothRate);

    legCtrl(4, 0, rightDirection, smoothRate);
    legCtrl(5, 2, rightDirection, smoothRate);
    legCtrl(6, 0, rightDirection, smoothRate);
  }
  else if(stepInput == 3){
    legCtrl(1, 3, leftDirection, smoothRate);
    legCtrl(2, 1, leftDirection, smoothRate);
    legCtrl(3, 3, leftDirection, smoothRate);

    legCtrl(4, 1, rightDirection, smoothRate);
    legCtrl(5, 3, rightDirection, smoothRate);
    legCtrl(6, 1, rightDirection, smoothRate);
  }
  moveGoal();
}


void spiderMoveCtrl(int leftCommand, int rightCommand){
  if(leftCommand == 0 && rightCommand == 0){
    moveInit();
  }
  else{
    for(float i=0.001;i<=1;i+=deStep){
      stepsInput(leftCommand, rightCommand, xsteps, i);
      if(stopFlag){
        moveInit();
        break;
      }
      delay(loopDelay);
    }
    xsteps++;
    if(xsteps>3){xsteps=0;}
  }
}


void legCtrl(int LegID, int LegStep, int DirectionInput, float RateInput){
  if(LegID == 1){
    if(LegStep == 0){
      GoalAngle[0] = InitAngle[0]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[1] = InitAngle[1];
    }
    else if(LegStep == 1){
      GoalAngle[0] = InitAngle[0]+fbRang*DirectionInput*RateInput;
      GoalAngle[1] = InitAngle[1];
    }
    else if(LegStep == 2){
      GoalAngle[0] = InitAngle[0]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[1] = InitAngle[1]-hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[0] = InitAngle[0]-fbRang*DirectionInput*RateInput;
      GoalAngle[1] = InitAngle[1]-hRang+hRang*RateInput;
    }
  }
  else if(LegID == 2){
    if(LegStep == 0){
      GoalAngle[2] = InitAngle[2]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[3] = InitAngle[3];
    }
    else if(LegStep == 1){
      GoalAngle[2] = InitAngle[2]+fbRang*DirectionInput*RateInput;
      GoalAngle[3] = InitAngle[3];
    }
    else if(LegStep == 2){
      GoalAngle[2] = InitAngle[2]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[3] = InitAngle[3]-hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[2] = InitAngle[2]-fbRang*DirectionInput*RateInput;
      GoalAngle[3] = InitAngle[3]-hRang+hRang*RateInput;
    }
  }
  else if(LegID == 3){
    if(LegStep == 0){
      GoalAngle[4] = InitAngle[4]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[5] = InitAngle[5];
    }
    else if(LegStep == 1){
      GoalAngle[4] = InitAngle[4]+fbRang*DirectionInput*RateInput;
      GoalAngle[5] = InitAngle[5];
    }
    else if(LegStep == 2){
      GoalAngle[4] = InitAngle[4]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[5] = InitAngle[5]-hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[4] = InitAngle[4]-fbRang*DirectionInput*RateInput;
      GoalAngle[5] = InitAngle[5]-hRang+hRang*RateInput;
    }
  }
  else if(LegID == 4){
    if(LegStep == 0){
      GoalAngle[6] = InitAngle[6]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[7] = InitAngle[7];
    }
    else if(LegStep == 1){
      GoalAngle[6] = InitAngle[6]-fbRang*DirectionInput*RateInput;
      GoalAngle[7] = InitAngle[7];
    }
    else if(LegStep == 2){
      GoalAngle[6] = InitAngle[6]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[7] = InitAngle[7]+hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[6] = InitAngle[6]+fbRang*DirectionInput*RateInput;
      GoalAngle[7] = InitAngle[7]+hRang-hRang*RateInput;
    }
  }
  else if(LegID == 5){
    if(LegStep == 0){
      GoalAngle[8] = InitAngle[8]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[9] = InitAngle[9];
    }
    else if(LegStep == 1){
      GoalAngle[8] = InitAngle[8]-fbRang*DirectionInput*RateInput;
      GoalAngle[9] = InitAngle[9];
    }
    else if(LegStep == 2){
      GoalAngle[8] = InitAngle[8]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[9] = InitAngle[9]+hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[8] = InitAngle[8]+fbRang*DirectionInput*RateInput;
      GoalAngle[9] = InitAngle[9]+hRang-hRang*RateInput;
    }
  }
  else if(LegID == 6){
    if(LegStep == 0){
      GoalAngle[10] = InitAngle[10]+fbRang*DirectionInput-fbRang*DirectionInput*RateInput;
      GoalAngle[11] = InitAngle[11];
    }
    else if(LegStep == 1){
      GoalAngle[10] = InitAngle[10]-fbRang*DirectionInput*RateInput;
      GoalAngle[11] = InitAngle[11];
    }
    else if(LegStep == 2){
      GoalAngle[10] = InitAngle[10]-fbRang*DirectionInput+fbRang*DirectionInput*RateInput;
      GoalAngle[11] = InitAngle[11]+hRang*RateInput;
    }
    else if(LegStep == 3){
      GoalAngle[10] = InitAngle[10]+fbRang*DirectionInput*RateInput;
      GoalAngle[11] = InitAngle[11]+hRang-hRang*RateInput;
    }
  }
}


void setSingleLED(uint16_t LEDnum, uint32_t c)
{
  matrix.setPixelColor(LEDnum, c);
  matrix.show();
}


void MoveCMD() {
  if (Serial.available()) 
  {
    // Read the JSON document from the "link" serial port
    DeserializationError err = deserializeJson(docReceive, Serial);

    if (err == DeserializationError::Ok) 
    {
      loopMode = 1;
      docReceive["left"].as<int>();
      docReceive["right"].as<int>();

      globalLeft  = docReceive["left"];
      globalRight = docReceive["right"];

      Serial.print(globalLeft);
      Serial.print(" ");
      Serial.println(globalRight);
    } 
    else 
    {
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}


void setup(){
  // Initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial) continue;
  
  // Set NeoPixel configuration 
  matrix.setBrightness(BRIGHTNESS);

  // Start NeoPixel library with all LEDs off
  matrix.begin();

  // Show settings of LEDs in NeoPixel array
  matrix.show();

  setSingleLED(0,matrix.Color(255, 0, 0));
  
  preferences.begin("ServoConfig", false);
  initAngleUpdate();
  servosSetup();
  wifiCheck();
  webCtrlServer();
  moveInit();

  setSingleLED(0,matrix.Color(0, 0, 255));
  
  for(int i=0;i<12;i++){
    Serial.print(InitAngle[i]);
    Serial.print(" ");
  }
  Serial.println(" ");
}


void loop() {
  if (loopMode == 1){spiderMoveCtrl(globalLeft, globalRight);}
  MoveCMD();

//  for(float i=0.01;i<=1;i++){
//    spiderMoveCtrl(1,1);
//  }
//  GoalAngle[1] = 90;
//  ledcWrite(12, calculatePWM(InitAngle[1]));
//  delay(1500);
//  GoalAngle[1] = 20;
//  ledcWrite(12, calculatePWM(GoalAngle[1]));
//  delay(1500);
}

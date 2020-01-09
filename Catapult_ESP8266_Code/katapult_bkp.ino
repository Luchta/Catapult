// 4-channel RC receiver for controlling
// an RC car / boat / plane / quadcopter / etc.
// using an ESP8266 and an Android phone with RoboRemo app

// Disclaimer: Don't use RoboRemo for life support systems
// or any other situations where system failure may affect
// user or environmental safety.

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>

// config:

const char *ssid = "katapult";  // You will connect your phone to this Access Point
const char *pw = "qwerty123"; // and this is the password
IPAddress ip(192, 168, 0, 1); // From RoboRemo app, connect to this IP
IPAddress netmask(255, 255, 255, 0);
const int port = 9876; // and this port

const int chCount = 4; // 4 channels, you can add more if you have GPIOs :)
Servo servoCh[chCount]; // will generate 4 servo PWM signals
int chPin[] = {13, 14, 16, 12}; // ESP pins: GPIO 0, 2, 14, 12
int chVal[] = {1500, 1500, 1500, 1570}; // default value (middle)
int buttonPin = 5;
int out  = 4;
int load = 0;
int count = 0;
int loading = 1;

int usMin = 700; // min pulse width
int usMax = 2300; // max pulse width

bool STARTUP = 1;

WiFiServer server(port);
WiFiClient client;


char cmd[100]; // stores the command chars received from RoboRemo
int cmdIndex;
unsigned long lastCmdTime = 60000;
unsigned long aliveSentTime = 0;


boolean cmdStartsWith(const char *st) { // checks if cmd starts with st
  for(int i=0; ; i++) {
    if(st[i]==0) return true;
    if(cmd[i]==0) return false;
    if(cmd[i]!=st[i]) return false;;
  }
  return false;
}


void exeCmd() { // executes the command from cmd

  lastCmdTime = millis();

  // example: set RoboRemo slider id to "ch0", set min 1000 and set max 2000
  
  if( cmdStartsWith("ch") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (int)atof(cmd+4);
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      }   
      servoCh[ch].writeMicroseconds(chVal[ch]);
    }
  }
  
  // invert channel:
  // example: set RoboRemo slider id to "ci0", set min -2000 and set max -1000
  
  if( cmdStartsWith("ci") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = -(int)atof(cmd+4);
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      }   
      servoCh[ch].writeMicroseconds(chVal[ch]);
    }
  }
  
  // use accelerometer:
  // example: set RoboRemo acc y id to "ca1"
  /*
  if( cmdStartsWith("ca") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (usMax+usMin)/2 + (int)( atof(cmd+4)*51 ); // 9.8*51 = 500 => 1000 .. 2000
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      }   
      servoCh[ch].writeMicroseconds(chVal[ch]);
    }
  }
  
  // invert accelerometer:
  // example: set RoboRemo acc y id to "cb1"
  
  if( cmdStartsWith("cb") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (usMax+usMin)/2 - (int)( atof(cmd+4)*51 ); // 9.8*51 = 500 => 1000 .. 2000
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      }   
      servoCh[ch].writeMicroseconds(chVal[ch]);
    }
  }
  */
  // release button: set press action cf0=1 and set release action cf0=0
  
  if( cmdStartsWith("cf") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (int)atof(cmd+4);
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      } 
      if(chVal[ch] == 1){
        servoCh[ch].writeMicroseconds(2000);
      }
      else{
        servoCh[ch].writeMicroseconds(1200);
      }
    }
  }
  
    // Load katapult button to press action cl0 = 1
	//load button gpio05;
	
  
  if( cmdStartsWith("ca") ) {
    int ch = cmd[2] - '0';
    if(ch>=0 && ch<=9 && cmd[3]==' ') {
      chVal[ch] = (int)atof(cmd+4);
      if(!servoCh[ch].attached()) {
        servoCh[ch].attach(chPin[ch], usMin, usMax);
      }
      if(chVal[ch] == 1){
        load = 1;
      }  
      
    }
  }
  
}



void setup() {

  delay(1000);

  /*for(int i=0; i<chCount; i++) {
    // attach channels to pins
    servoCh[i].attach(chPin[i], usMin, usMax);
    // initial value = middle
    chVal[i] = (usMin + usMax)/2;
    // update
    servoCh[i].writeMicroseconds( chVal[i] );
  }*/
  
  cmdIndex = 0;
  
  

  
  
  Serial.begin(115200);

  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP 
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP

  server.begin(); // start TCP server

  Serial.println("ESP8266 RC receiver 1.1 powered by RoboRemo");
  Serial.println((String)"SSID: " + ssid + "  PASS: " + pw);
  Serial.println((String)"RoboRemo app must connect to " + ip.toString() + ":" + port);
  

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(out, OUTPUT);
  
}


void loop() {

  // if contact lost for more than half second
  if(STARTUP) {  
    for(int i=0; i<chCount; i++) {
      // set all values to middle
      servoCh[i].writeMicroseconds( (usMin + usMax)/2 );
      servoCh[i].detach(); // stop PWM signals
    }
    STARTUP = 0;
  }

  

  if(!client.connected()) {
    client = server.available();
    return;
  }

  // here we have a connected client

  if(client.available()) {
    char c = (char)client.read(); // read char from client (RoboRemo app)

    if(c=='\n') { // if it is command ending
      cmd[cmdIndex] = 0;
      exeCmd();  // execute the command
      cmdIndex = 0; // reset the cmdIndex
    } else {      
      cmd[cmdIndex] = c; // add to the cmd buffer
      if(cmdIndex<99) cmdIndex++;
    }
  } 

  //loading
  if(load){
       
  switch (loading) {
    case 1:
      //pull arm
      if(!servoCh[2].attached()) {
        servoCh[2].attach(chPin[2], usMin, usMax);
      }
      if(!servoCh[3].attached()) {
        servoCh[3].attach(chPin[3], usMin, usMax);
      }
      servoCh[2].writeMicroseconds(2000);   //pull arm
      servoCh[3].writeMicroseconds(2000);   //unlock arm
      loading = 2;
      break;
    case 2:
      //wait for button
      if(digitalRead(buttonPin) == LOW){
         loading = 3; 
      }
      break;
    case 3:
        //lock and unload
        servoCh[3].writeMicroseconds(1200); //lock arm
        loading = 4;  
        break;
    case 4:
       servoCh[2].writeMicroseconds(1000); //start servo unroll
       count = 0;
       loading = 5;
       break; 
    case 5: 
     if(count >= 110000 ){
       servoCh[2].writeMicroseconds(1500); //stop servo unroll
       load = 0;
       loading = 1;
       count = 0;
       servoCh[2].detach(); // stop PWM signals
       }
     break;
    default:
      // if nothing else matches, do the default
      // default is optional
    break;
    }
    
  }
  
  count = (count + 1);
  if(count >= 900000){
    count = 0;
    }
  
  if(digitalRead(buttonPin) == LOW){
    digitalWrite(out,HIGH);
  }else{
    digitalWrite(out,LOW);
  }
  

  if(millis() - aliveSentTime > 500) { // every 500ms
    client.write("alive 1\n");
    // send the alibe signal, so the "connected" LED in RoboRemo will stay ON
    // (the LED must have the id set to "alive")
    
    aliveSentTime = millis();
    // if the connection is lost, the RoboRemo will not receive the alive signal anymore,
    // and the LED will turn off (because it has the "on timeout" set to 700 (ms) )
  }

}

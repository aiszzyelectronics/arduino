/*
NODEMCU ESP32 PMS5003T
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-1-13 09:00
https://www.facebook.com/francefu

Set WIFI ssid and pwd
http://192.168.4.1?admin

Set ThingSpeak api_key
http://192.168.4.1?admin_key

Set Line Notify token
http://192.168.4.1?admin_token
  

LCD Library
https://github.com/nhatuan84/esp32-lcd

LCD
5V, GND, RX:12, TX:14

PMS5003T
3V3, GND, RX:16, TX:17
*/

const char* ssid     = "*****";  //WIFI ssid
const char* password = "*****";  //WIFI pwd

const char* apssid = "ESP32_PMS5003T";
const char* appassword = "12345678";

String thingspeak_api_key = "";   //ThingSpeak
String line_token = "";           //Line Notify

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <HardwareSerial.h>
HardwareSerial mySerial(1);  // RX:gpio16   TX:gpio17

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(39, 16, 2);  //0x27=39, 0x3F=63 

int lcdRX = 12;
int lcdTX = 14;
  
long pmat10 = 0;
long pmat25 = 0;
long pmat100 = 0;
long Temp = 0;
long Humid = 0;
char buf[50];

WiFiServer server(80);

String Feedback="", Command="",cmd="",P1="",P2="",P3="",P4="",P5="",P6="",P7="",P8="",P9="";
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;

void ExecuteCommand()
{
  Serial.println("");
  //Serial.println("Command: "+Command);
  Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
  Serial.println("");

  if (cmd=="admin") { 
    Feedback="PMS5003T (Fongshan)<br>SSID: <input type=\"text\" id=\"ssid\"><br>PWD: <input type=\"text\" id=\"pwd\"><br><input type=\"button\" value=\"submit\" onclick=\"location.href='?resetwifi='+document.getElementById('ssid').value+';'+document.getElementById('pwd').value;\">";  
  }
  else if (cmd=="admin_key") { 
    Feedback="PMS5003T (Fongshan)<br>ThingSpeak API_KEY: <input type=\"text\" id=\"key\"><input type=\"button\" value=\"submit\" onclick=\"location.href='?thingspeakapikey='+document.getElementById('key').value;\">";  
  }  
  else if (cmd=="admin_token") { 
    Feedback="PMS5003T (Fongshan)<br>Line Token: <input type=\"text\" id=\"token\"><input type=\"button\" value=\"submit\" onclick=\"location.href='?linetoken='+document.getElementById('token').value;\">";  
  }     
  else if (cmd=="thingspeakapikey") {
    thingspeak_api_key = P1;
    Feedback="Set api_key = "+P1+" OK";
  }  
  else if (cmd=="linetoken") {
    line_token = P1;
    Feedback="Set token = "+P1+" OK";    
  }   
  else if (cmd=="ip") {
    Feedback="AP IP: "+WiFi.softAPIP().toString();    
    Feedback+=", ";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  }  
  else if (cmd=="mac") {
    Feedback="STA MAC: "+WiFi.macAddress();
  }  
  else if (cmd=="restart") {
    ESP.restart();
  } 
  else if (cmd=="resetwifi") {
    WiFi.begin(P1.c_str(), P2.c_str());
    Serial.print("Connecting to ");
    Serial.println(P1);
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    } 
    Serial.println("");
    Serial.println("STAIP: "+WiFi.localIP().toString());
    Feedback="STAIP: "+WiFi.localIP().toString();
  }    
  else if (cmd=="inputpullup") {
    pinMode(P1.toInt(), INPUT_PULLUP);
  }  
  else if (cmd=="pinmode") {
    if (P2.toInt()==1)
      pinMode(P1.toInt(), OUTPUT);
    else
      pinMode(P1.toInt(), INPUT);
  }        
  else if (cmd=="digitalwrite") {
    ledcDetachPin(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  }   
  else if (cmd=="digitalread") {
    Feedback=String(digitalRead(P1.toInt()));
  }
  else if (cmd=="analogwrite") {
    ledcAttachPin(P1.toInt(), 1);
    ledcSetup(1, 5000, 8);
    ledcWrite(1,P2.toInt());
  }       
  else if (cmd=="analogread") {
    Feedback=String(analogRead(P1.toInt()));
  }
  else if (cmd=="touchread") {
    Feedback=String(touchRead(P1.toInt()));
  }   
  else if (cmd=="thingspeakupdate") {
    String domain="api.thingspeak.com";
    String request = "/update?api_key=" + P1;
    request += "&field1="+P2+"&field2="+P3+"&field3="+P4+"&field4="+P5+"&field5="+P6+"&field6="+P7+"&field7="+P8+"&field8="+P9;
    Feedback=tcp_https(domain,request,443,0);
  }    
  else if (cmd=="linenotify") {    //message=xxx&stickerPackageId=xxx&stickerId=xxx
    String token = P1;
    String request = P2;
    Feedback=LineNotify(token,request,1);
  } 
  else {
    Feedback="Command is not defined.";
  }
  if (Feedback=="") Feedback=Command;  
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.mode(WIFI_AP_STA);

  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));

  WiFi.begin(ssid, password);

  delay(1000);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      if ((StartTime+10000) < millis()) break;
  } 

  if (WiFi.status() == WL_CONNECTED)
  {
    pinMode(2, OUTPUT);
    for (int i=0;i<5;i++)
    {
      digitalWrite(2,HIGH);
      delay(100);
      digitalWrite(2,LOW);
      delay(100);
    }
  }  

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());

  if (WiFi.status() == WL_CONNECTED)
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);
  else
    WiFi.softAP(apssid, appassword);
    
  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  Serial.println("");
  Serial.println("APIP address: ");
  Serial.println(WiFi.softAPIP());    
  server.begin();  
  
  lcd.begin(lcdRX, lcdTX);
  lcd.backlight();
  
  delay(3000); 
}

void loop() {

  getRequest();

  if (WiFi.status() == WL_CONNECTED) {
    retrievepm25();
  
    Serial.println("");
    Serial.println("PM2.5 : " + String(pmat25) + " ug/m3");
    Serial.println("PM10 : " + String(pmat100) + " ug/m3");
    Serial.println("Temp : " + String(Temp) + " *C");
    Serial.println("Humid : " + String(Humid) + " %RH");
    Serial.println("");    
  
    String myPm25 = String(pmat25) ;
    if (myPm25.length()==1) myPm25 = " " + myPm25;
    String myPm100 = String(pmat100) ;
    if (myPm100.length()==1) myPm100 = " " + myPm100;
    String myTemp = String(Temp) ;
    if (myTemp.length()==1) myTemp = " " + myTemp;
    String myHumid = String(Humid) ;
    if (myHumid.length()==1) myHumid = " " + myHumid; 
         
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("PM2.5=" + myPm25 + "  " + myTemp + "*C");
    lcd.setCursor(0,1);
    lcd.print("PM10 =" + myPm100 + "  " + myHumid + "%RH");

    String domain="api.thingspeak.com";
    String request = "/update?api_key=" + thingspeak_api_key;
    request += "&field1="+String(pmat25)+"&field2="+String(pmat100)+"&field3="+String(Temp)+"&field4="+String(Humid);
    if (thingspeak_api_key!="") 
      Serial.println(tcp_https(domain,request,443,0));
    
    String message = "\nPM2.5:    "+String(pmat25)+" ug/m3\nPM100:    "+String(pmat100)+" ug/m3\nTemperature:    "+String(Temp)+" *C\nHumidity:    "+String(Humid)+" RH";
    if (line_token!="")
      Serial.println(LineNotify(line_token, "message="+message, 1));

    delay(30000);
  }
}

void retrievepm25(){
  int count = 0;
  unsigned char c;
  unsigned char high;
  while (mySerial.available()) { 
    c = mySerial.read();
    if((count==0 && c!=0x42) || (count==1 && c!=0x4d)){
      break;
    }
    if(count > 27){ 
      break;
    }
     else if(count == 10 || count == 12 || count == 14 || count == 24 || count == 26) {
      high = c;
    }
    else if(count == 11){
      pmat10 = 256*high + c;
    }
    else if(count == 13){
      pmat25 = 256*high + c;
    }
    else if(count == 15){
      pmat100 = 256*high + c;
    }
     else if(count == 25){        
      Temp = (256*high + c)/10;
    }
    else if(count == 27){            
      Humid = (256*high + c)/10;
    }   
    count++;
  }
  while(mySerial.available()) mySerial.read();
}

void getRequest() {
  Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
   WiFiClient client = server.available();

  if (client) 
  { 
    String currentLine = "";

    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();             
        
        getCommand(c);
                
        if (c == '\n') {
          if (currentLine.length() == 0) {       
            client.println("HTTP/1.1 200 OK");
            client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
            client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html><head>");
            client.println("<meta charset=\"UTF-8\">");
            client.println("<meta http-equiv=\"Access-Control-Allow-Origin\" content=\"*\">");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
            client.println("</head><body>");
            client.println(Feedback);
            client.println("</body></html>");
            client.println();
                        
            Feedback="";
            break;
          } else {
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1))
        {
          if (Command.indexOf("stop")!=-1) {
            client.println();
            client.println();
            client.stop();
          }
          currentLine="";
          Feedback="";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void getCommand(char c)
{
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) P1=P1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) P2=P2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) P3=P3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) P4=P4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) P5=P5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) P6=P6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) P7=P7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) P8=P8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) P9=P9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}

String tcp_https(String domain,String request,int port,byte wait)
{
    WiFiClientSecure client_tcp;

    if (client_tcp.connect(domain.c_str(), port)) 
    {
      Serial.println("GET " + request);
      client_tcp.println("GET " + request + " HTTP/1.1");
      client_tcp.println("Host: " + domain);
      client_tcp.println("Connection: close");
      client_tcp.println();

      String getResponse="",Feedback="";
      boolean state = false;
      int waitTime = 3000;   // timeout 3 seconds
      long startTime = millis();
      while ((startTime + waitTime) > millis())
      {
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (c == '\n') 
            {
              if (getResponse.length()==0) state=true; 
              getResponse = "";
            } 
            else if (c != '\r')
              getResponse += String(c);
            if (state==true) Feedback += String(c);
            if (wait==1)
              startTime = millis();
         }
         if (wait==0)
          if ((state==true)&&(Feedback.length()!= 0)) break;
      }
      client_tcp.stop();
      return Feedback;
    }
    else
      return "Connection failed";  
}

String LineNotify(String token, String request, byte wait)
{
  request.replace(" ","%20");
  request.replace("&","%20");
  request.replace("#","%20");
  //request.replace("\'","%27");
  request.replace("\"","%22");
  request.replace("\n","%0D%0A");
  request.replace("%3Cbr%3E","%0D%0A");
  request.replace("%3Cbr/%3E","%0D%0A");
  request.replace("%3Cbr%20/%3E","%0D%0A");
  request.replace("%3CBR%3E","%0D%0A");
  request.replace("%3CBR/%3E","%0D%0A");
  request.replace("%3CBR%20/%3E","%0D%0A"); 
  request.replace("%20stickerPackageId","&stickerPackageId");
  request.replace("%20stickerId","&stickerId");    
  Serial.println(request);
  WiFiClientSecure client_tcp;
  
  if (client_tcp.connect("notify-api.line.me", 443)) 
  {
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close"); 
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("User-Agent: ESP8266/1.0");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Content-Length: " + String(request.length()));
    client_tcp.println();
    client_tcp.println(request);
    client_tcp.println();
    
    String getResponse="",Feedback="";
    boolean state = false;
    int waitTime = 3000;   // timeout 3 seconds
    long startTime = millis();
    while ((startTime + waitTime) > millis())
    {
      while (client_tcp.available()) 
      {
          char c = client_tcp.read();
          if (c == '\n') 
          {
            if (getResponse.length()==0) state=true; 
            getResponse = "";
          } 
          else if (c != '\r')
            getResponse += String(c);
          if (state==true) Feedback += String(c);
          if (wait==1)
            startTime = millis();
       }
       if (wait==0)
        if ((state==true)&&(Feedback.length()!= 0)) break;
    }
    client_tcp.stop();
    return Feedback;
  }
  else
    return "Connection failed";  
}
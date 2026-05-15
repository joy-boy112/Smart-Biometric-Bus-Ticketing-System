#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <HTTPClient.h>

#define SS_PIN 5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

const char* ssid="Sollamatapoda";
const char* password="sollanuma@123";

WebServer server(80);

String displayText="";

unsigned long ticketTimer=0;
bool ticketShown=false;

int ticketNumber=1;


// BUS STOPS
String stops[5]={
"MELMARUVATHUR",
"ACHARAPAKKAM",
"MADURANTAKAM",
"KARUNGUZHI",
"CHENGALPATTU"
};

int km[5]={0,6,14,18,28};

int fromIndex=-1;
int toIndex=-1;

int distanceKM;
int fare;

int state=1;


// KEYPAD
const byte ROWS=4;
const byte COLS=4;

char keys[ROWS][COLS]={
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

byte rowPins[ROWS]={13,12,14,27};
byte colPins[COLS]={26,25,33,32};

Keypad keypad=Keypad(makeKeymap(keys),rowPins,colPins,ROWS,COLS);


// GET DATE TIME
String getDateTime(){

struct tm timeinfo;

if(!getLocalTime(&timeinfo)){
return "TIME ERROR";
}

char buffer[30];
strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",&timeinfo);

return String(buffer);

}


// DATABASE SAVE FUNCTION
void saveTicketDB(String ticket,String from,String to,String distance,String fare,String date){

WiFiClient client;
HTTPClient http;

date.replace(" ","%20");

String url="http://172.31.45.63:3000/saveTicket?ticket="+ticket+
"&from="+from+
"&to="+to+
"&distance="+distance+
"&fare="+fare+
"&date="+date;

Serial.println("Sending to DB:");
Serial.println(url);

http.begin(client, url);
http.setTimeout(5000);

int httpCode=http.GET();

Serial.print("HTTP Response: ");
Serial.println(httpCode);

http.end();

}


// MOBILE TICKET UI
void handleRoot(){

String page="<!DOCTYPE html><html><head>";

page+="<meta charset='UTF-8'>";
page+="<meta name='viewport' content='width=device-width, initial-scale=1'>";
page+="<meta http-equiv='refresh' content='0.2'>";

page+="<style>";

page+="body{background:#f1f3f6;font-family:monospace;text-align:center;padding:20px;}";

page+=".ticket{background:white;width:300px;margin:auto;padding:16px;";
page+="box-shadow:0 3px 10px rgba(0,0,0,0.2);border-radius:6px;}";

page+=".header{font-weight:bold;font-size:16px;}";

page+=".depot{font-size:13px;margin-bottom:8px;}";

page+=".line{border-top:1px dashed #000;margin:8px 0;}";

page+="pre{white-space:pre-wrap;text-align:left;font-size:13px;}";

page+=".footer{font-size:12px;margin-top:6px;}";

page+="</style>";

page+="</head><body>";

page+="<div class='ticket'>";

page+="<div class='header'>TNSTC BUS TICKET</div>";
page+="<div class='depot'>Chengalpattu Depot</div>";

page+="<div class='line'></div>";

page+="<pre>";
page+=displayText;
page+="</pre>";

page+="<div class='line'></div>";

page+="<div class='footer'>Safe Journey 🚍</div>";

page+="</div>";

page+="</body></html>";

server.send(200,"text/html",page);

}


// MENU
void showMenu(){

displayText="";
displayText+="Select FROM Stop\n";
displayText+="8 sothupakam\n";
displayText+="2 Acharapakkam\n";
displayText+="3 Madurantakam\n";
displayText+="5 Karunguzhi\n";
displayText+="6 Chengalpattu\n";

Serial.println(displayText);

}


// SETUP
void setup(){

Serial.begin(115200);

SPI.begin();
rfid.PCD_Init();

WiFi.begin(ssid,password);

Serial.print("Connecting WiFi");

while(WiFi.status()!=WL_CONNECTED){
delay(500);
Serial.print(".");
}

Serial.println("");
Serial.println("WiFi Connected");

Serial.print("ESP32 IP: ");
Serial.println(WiFi.localIP());

configTime(19800,0,"pool.ntp.org","time.nist.gov");

delay(2000);

struct tm timeinfo;

while(!getLocalTime(&timeinfo)){
delay(1000);
}

server.on("/",handleRoot);
server.begin();

showMenu();

}


// LOOP
void loop(){

server.handleClient();

char key=keypad.getKey();

if(key && state<3){

while(keypad.getKey());

int index=-1;

if(key=='8') index=0;
if(key=='2') index=1;
if(key=='3') index=2;
if(key=='5') index=3;
if(key=='6') index=4;

if(index==-1) return;


// FROM
if(state==1){

fromIndex=index;

displayText="";
displayText+="From : "+stops[fromIndex]+"\n\n";

displayText+="Select Destination\n";
displayText+="8 Sothupakkam\n";
displayText+="2 Acharapakkam\n";
displayText+="3 Madurantakam\n";
displayText+="5 Karunguzhi\n";
displayText+="6 Chengalpattu\n";

state=2;

}


// DESTINATION
else if(state==2){

toIndex=index;

distanceKM=abs(km[toIndex]-km[fromIndex]);
fare=distanceKM*2;

displayText="";
displayText+="From : "+stops[fromIndex]+"\n";
displayText+="To : "+stops[toIndex]+"\n";
displayText+="Distance : "+String(distanceKM)+" KM\n";
displayText+="Fare : ₹"+String(fare)+"\n\n";
displayText+="Tap Card To Pay";

state=3;

}

}


// RFID TAP
if(state==3){

if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){

char ticketNo[10];
sprintf(ticketNo,"%06d",ticketNumber);

ticketNumber++;

displayText="";

displayText+="த அ போ க - (விழுப்புரம்) லிட்\n";
displayText+="CHENGALPATTU DEPOT\n\n";

String dateTime=getDateTime();

displayText+=dateTime+"\n";
displayText+="VPM/AB/M/522\n\n";

displayText+="T No : ";
displayText+=ticketNo;
displayText+="\n";

displayText+="CASH  GENERAL  MOFUSSI - EXPRESS\n\n";

displayText+=String(fromIndex+1)+"-"+stops[fromIndex];
displayText+="  ";
displayText+=String(toIndex+1)+"-"+stops[toIndex];
displayText+="\n\n";

displayText+="ADULT(S): 1 x ";
displayText+=String(fare);

displayText+="\n\nRs : ";
displayText+=String(fare);
displayText+=".00\n\n";

displayText+="** பாதுகாப்பான பயணம் செய்யவும் **\n";


// SAVE DATABASE
saveTicketDB(
String(ticketNo),
stops[fromIndex],
stops[toIndex],
String(distanceKM),
String(fare),
dateTime
);

ticketTimer=millis();
ticketShown=true;
state=4;

rfid.PICC_HaltA();
rfid.PCD_StopCrypto1();

}

}


// SHOW TICKET
if(ticketShown && millis()-ticketTimer>5000){

showMenu();

state=1;
ticketShown=false;

}

}
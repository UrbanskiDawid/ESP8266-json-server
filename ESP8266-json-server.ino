
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>


#include <OneWire.h>
#include <DallasTemperature.h>

#define HELLO_MESSAGE "ESP8266 json server"

//Real Access Point configuration
const char
  *ssid = "AAAAA", 
  *password = "BBBB";

//Software Access Point configuration
const char
*APssid = "ESP6288",
*APpassword = "dupadupa"; //MINIMUM 8 chars or softAP will use default name and pass

//--web server
#define webServerPort 80
ESP8266WebServer server(webServerPort);

//--udp ticker
WiFiUDP g_udp;
#define g_port 6666
const unsigned long int t_waitTimeMS = 10000;//10sec (not exact)
unsigned long int t_memory=0;

//--DS18B20
#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

//the value to publish
String VALUE="";

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!\n\n go to /json or /get to request value.");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleGetValueJSON() {
  String message = "{ ";
  message += " value=";
  message += VALUE;
  message += " }";
  server.send(200, "application/json", message);
}

void handleGetValueRAW() {
  server.send(200, "text/plain", VALUE);
}

boolean connectedToAP=false;

void setup(void) {

  //Serial
  Serial.begin(115200);
  Serial.println(HELLO_MESSAGE);

  //Ticker
  Serial.print("ticker: ");
  Serial.print(t_waitTimeMS/1000.0);
  Serial.println("sec");

  //DS18B20
  DS18B20.begin();//->find all devices
  Serial.print("DS18B20 Found: "); 
  Serial.print(String(DS18B20.getDeviceCount()));
  Serial.println(" devices");

  //soft AP
  Serial.print("Softrawre AP ");
  if(!WiFi.softAP(APssid, APpassword)){
     Serial.println("failed");  
  }else{
     Serial.println("started");
  }

  //connect to AP
  WiFi.begin(ssid, password);

  //Multicast DNS
  if (MDNS.begin("esp8266")) {
    MDNS.addService("http", "tcp", webServerPort);
    MDNS.addService("esp", "udp", g_port);
    Serial.println("MDNS responder started");
  }

  //UDP server
  g_udp.begin(g_port);

  // web server
  server.on("/", handleRoot);
  server.on("/json", handleGetValueJSON);
  server.on("/get", handleGetValueRAW);
  server.onNotFound(handleNotFound);
  server.begin();  
  Serial.println("HTTP server started");
}

void udpBrodcast(){
  IPAddress ip = WiFi.localIP();
  ip[3] = 255;

  // transmit broadcast package
  g_udp.beginPacket(ip, g_port);
  g_udp.write(VALUE.c_str());
  g_udp.write("\n");
  g_udp.endPacket();
}

void SerialPrintWiFiStatus(){
  Serial.print("WIFI: ");
  if(WiFi.status() == WL_CONNECTED){ 
    Serial.print("Connected to: ");
    Serial.print(ssid);
    Serial.print(" IP address: ");
    Serial.println(WiFi.localIP());
  }else{
    Serial.print("Disconnected from: ");
    Serial.println(ssid);
  }
}

void tick() {
  Serial.println("tick!");

  SerialPrintWiFiStatus();//print WIFI status

  updateVALUE();//update the value
  
  udpBrodcast();//brodcast value using UDP
}

String DS18B20toString(DeviceAddress deviceAddress) {

  String ret="0x";
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) ret=ret+"0";
    ret=ret+String(deviceAddress[i], HEX);
  } 
  return ret;
}

void updateVALUE(){

  uint8_t DS18B20num = DS18B20.getDeviceCount();
  uint8_t adress;

  if(DS18B20num==0)
  {
    VALUE="error-no-devices";
  }else{
    DS18B20.requestTemperatures(); /// sends command for all devices on the bus to perform a temperature conversion

    VALUE="";//values will be concatenated
    for(uint8_t index=0; index<DS18B20num; index++)
    {
      VALUE=VALUE+String(index)+":";
      if(!DS18B20.getAddress(&adress,index))
      {
        VALUE=VALUE+"0x00=??*C;";
      }else{
        VALUE=VALUE+DS18B20toString(&adress)+"=";
        float temp= DS18B20.getTempCByIndex(index);
        if(temp == 85.0 || temp == (-127.0)) {  VALUE=VALUE+"error";
        }else{                                  VALUE=VALUE+String(temp)+"*C";  }
        VALUE=VALUE+";";
      }
    }
  }
  
  Serial.print("TEMP: ");
  Serial.println(VALUE);  
}

void loop(void) {

  //webserver
  server.handleClient();

  //ticker
  int d = (int)millis()-t_memory;
  if(d<0) { d=-d; }//millis can overflow
  if(d>t_waitTimeMS) {
     t_memory=millis();
     tick();
  }

  //check if connection status has changed
  boolean isConnectedToAP=(WiFi.status() == WL_CONNECTED);
  if(connectedToAP != isConnectedToAP) {
    connectedToAP = isConnectedToAP;
    Serial.print("INFO: ");
    if(connectedToAP) { Serial.println("WIFI connected"); }
    else              { Serial.println("WIFI disconnected"); }
  }
  
}


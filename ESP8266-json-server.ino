#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

//Access Point configuration
const char
  *ssid = "AAAAA", 
  *password = "BBBB";

//web server
#define webServerPort 80
ESP8266WebServer server(webServerPort);
//--

//udp ticker
WiFiUDP g_udp;
#define g_port 6666
const unsigned long int t_waitTimeMS = 20000;//20sec (not exact)
unsigned long int t_memory=0;
//-----------------

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

void setup(void) {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // start Multicast DNS
  if (MDNS.begin("esp8266")) {
    MDNS.addService("http", "tcp", webServerPort);
    MDNS.addService("esp", "udp", g_port);
    Serial.println("MDNS responder started");    
  }

  // start UDP server
  g_udp.begin(g_port);

  // start web server
  server.on("/", handleRoot);
  server.on("/json", handleGetValueJSON);
  server.on("/get", handleGetValueRAW);
  server.onNotFound(handleNotFound);
  server.begin();  
  Serial.println("HTTP server started");
}

void tick() {
  Serial.println("tick!");

  IPAddress ip = WiFi.localIP();
  ip[3] = 255;

  // transmit broadcast package
  g_udp.beginPacket(ip, g_port);
  g_udp.write(VALUE.c_str());
  g_udp.write("\n");
  g_udp.endPacket();
}

void loop(void) {
  server.handleClient();

  //fake value generator: replase this with some readout
  VALUE=String(millis());
  //----
  
  int d = (int)millis()-t_memory;
  if(d<0) { d=-d; }//millis can overflow
  if(d>t_waitTimeMS) {
     t_memory=millis();
     tick();
  }
}


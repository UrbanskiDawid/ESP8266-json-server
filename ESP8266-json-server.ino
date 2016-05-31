#include <ESP8266WiFi.h>

//Access Point configuration
const char
  *ssid = "AAAAA", 
  *password = "BBBB";

//http message header
String httpHeader =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "\r\n";

WiFiServer server(80);

void printWiFiStatus();

void setup(void) {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  server.begin();
}


unsigned long value = 0;

void getValue() {
  value=millis();//just a dummy value
}

void waitUntilConnectedToAP() {
  if (WiFi.status() != WL_CONNECTED) {
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("Connecting to ");
      Serial.print(ssid);
      Serial.println("...");
      delay(500);
    }
    
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop(void) {

  waitUntilConnectedToAP();
  
  WiFiClient client = server.available();
  if (client) {
    
    Serial.println("Client connected.");

    while(!client.available()){
        delay(1);
    }
    delay(250);//this must be present for browser to send request and start reading response

    getValue();
    
    client.print(httpHeader);
    String body = "{ value=\"";
    body+= value;
    body+="\"}";
    client.print(body);

    Serial.println("Client disconnected.");
    client.flush();
    client.stop();
  }
}


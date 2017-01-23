#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 5

DHT dht(DHTPIN, DHT11);

const char* ssid     = "CWRouter1-";
const char* password = "cyberwings1234";
const char* zabbix_server = "192.168.1.63";

Ticker ticker;
bool readyForTicker = false;

void  setReadyForTicker ()  { 

  readyForTicker  =  true ; 
}
; 
void  doBlockingIO ()  {
  
  uint64_t  payloadsize  ;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // create "zabbix sender format" json data for sending zabbix server
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["request"] = "sender data";

  JsonArray& data = root.createNestedArray("data");

  JsonObject& item = jsonBuffer.createObject();
  item["host"] = "Home Network";
  item["key"] = "iot-hum";
  item["value"] = h;
  data.add(item);



  JsonObject& item2 = jsonBuffer.createObject();
    item2["host"] = "Home Network";
    item2["key"] = "iot-temp";
    item2["value"] = t;
    data.add(item2);

  /*
    // zabbix sender can send more items at once
    JsonObject& item2 = jsonBuffer.createObject();
    item2["host"] = "Home Network";
    item2["key"] = "test2";
    item2["value"] = "hello";
    data.add(item2);
  */

  Serial.println();
  Serial.println("== request  ================");
  root.printTo(Serial);

  Serial.println("");
  Serial.println("============================");
  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  Serial.print("payload json size: ");
  Serial.println(strlen(buffer));
  Serial.println("============================");


 
  // connect to zabbix server
  Serial.print("connecting to ");
  Serial.println(zabbix_server);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int zabbixPort = 10051 ;
  if (!client.connect(zabbix_server, zabbixPort)) {
    Serial.println("connection failed");
    return;
  }


  // send the zabbix_sender's format data to server

  // send fixed header to zabbix server
  client.print(String("ZBXD") );
  client.write(0x01);

  // send json size to zabbix server
  payloadsize = strlen(buffer);
  for (int i = 0; i < 64; i += 8) {
    client.write(lowByte(payloadsize >> i));
  }

  // send json to zabbix server
  client.print(buffer);
  //////////////////////////////


  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println("== response ================");
    Serial.print(line);
    Serial.println("");
    Serial.println("============================");
  }

  Serial.println();
  Serial.println("closing connection");


  readyForTicker = false;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  // call setReadyForTicker() every 60 seconds
  ticker.attach(30, setReadyForTicker);
}

void loop() {
  if (readyForTicker) {
    doBlockingIO();
  }
}


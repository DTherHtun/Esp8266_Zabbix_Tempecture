#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define DHTPIN 5

DHT dht(DHTPIN, DHT11);

const char* ssid     = "SSID_NAME";
const char* password = "SSID_PASSWORD";
const char* zabbix_server = "192.168.1.2";

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
    
  char buffer[256];
  WiFiClient client;
  const int zabbixPort = 10051 ;
  if (!client.connect(zabbix_server, zabbixPort)) {
    return;
  }

  client.print(String("ZBXD") );
  client.write(0x01);
  
  payloadsize = strlen(buffer);
  for (int i = 0; i < 64; i += 8) {
    client.write(lowByte(payloadsize >> i));
  }

  client.print(buffer);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      return;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\r');
  }

  readyForTicker = false;
}

void setup() {
  delay(10);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ticker.attach(30, setReadyForTicker);
}

void loop() {
  if (readyForTicker) {
    doBlockingIO();
  }
}


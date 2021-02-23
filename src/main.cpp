#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESP8266Ping.h>
#include "index.h"
#include "secrets.h"

const String SSID = WIFI_SSID;
const String PWD = WIFI_PWD;
const String HOST_NAME = "PlantCare";

const char *mqtt_server = "homeassistant";
const char *mqtt_user = MQTT_USER;
const char *mqtt_pass = MQTT_PWD;
const char *mqtt_topic = "sensor/moisture";

const int dry = 740;
const int wet = 294;

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);
IPAddress ip;
int percentageMoisture = 0;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void handleRoot();
void handleHumidity();
void handleNotFound();
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

void setup()
{
  Serial.begin(9600);
  Serial.println("");
  Serial.println("I'm PlantCare, hello.");
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOST_NAME);
  WiFi.begin(SSID, PWD);
  Serial.println("Connect to the wifi");
  while (!WiFi.isConnected())
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(50);
  }
  Serial.println("Connected to the wifi");
  ip = WiFi.localIP();
  Serial.print("I got the ip ");
  Serial.println(ip);
  Serial.println("Start web server");
  server.on("/", handleRoot);
  server.on("/humidity", handleHumidity);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");

  bool ret = Ping.ping(mqtt_server);
  if (ret)
  {
    Serial.print("ping replay from mqtt broker ");
  }
  else
  {
    Serial.print("NO ping replay from mqtt broker ");
  }
  Serial.print(mqtt_server);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(LED_BUILTIN, true);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    ++value;
    int sensorValue = analogRead(A0);
    percentageMoisture = map(sensorValue, wet, dry, 100, 0);
    snprintf(msg, MSG_BUFFER_SIZE, "%d", percentageMoisture);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_topic, msg);
  }
}

void handleRoot()
{
  server.send(200, "text/html", index_html); // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleHumidity()
{
  server.send(200, "text/plain", String(percentageMoisture)); // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound()
{
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void callback(char *topic, byte *payload, unsigned int length)
{

}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass ))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_topic, "hello world");
      // ... and resubscribe
      //client.subscribe("test");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
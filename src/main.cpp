#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

// Wifi setup
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// HiveMQ Cloud credentials
const char *mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char *mqtt_user = MQTT_USER;
const char *mqtt_pass = MQTT_PASS;

// MQTT Topics
const char *topic_led = "joseph/device1/led";
const char *topic_status = "joseph/device1/status"; // Status topic for online/offline

WiFiClientSecure espClient;
PubSubClient client(espClient);

#define LED_PIN 2 // or 4 or any working LED pin

unsigned long lastHeartbeat = 0;
const long heartbeatInterval = 30000; // Send heartbeat every 30 seconds

void callback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  for (int i = 0; i < length; i++)
    msg += (char)payload[i];

  if (String(topic) == topic_led)
  {
    if (msg == "ON")
      digitalWrite(LED_PIN, HIGH);
    if (msg == "OFF")
      digitalWrite(LED_PIN, LOW);
  }
}

void reconnect()
{
  while (!client.connected())
  {
    // Connect with Last Will and Testament (LWT)
    // When device disconnects unexpectedly, MQTT broker will publish "offline" to status topic
    if (client.connect("esp32Client", mqtt_user, mqtt_pass,
                       topic_status, 1, true, "offline"))
    {
      // Publish "online" status with retain flag
      client.publish(topic_status, "online", true);
      
      // Subscribe to LED control topic
      // The broker will send the last retained message immediately after subscription
      client.subscribe(topic_led);
    }
    else
    {
      delay(2000);
    }
  }
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  // HiveMQ TLS setup
  espClient.setInsecure(); // Simplest for testing

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  // Send periodic heartbeat to confirm device is still online
  unsigned long now = millis();
  if (now - lastHeartbeat > heartbeatInterval)
  {
    lastHeartbeat = now;
    client.publish(topic_status, "online", true);
  }
}
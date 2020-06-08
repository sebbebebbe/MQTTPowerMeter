#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *mqtt_server = "192.168.X.X";
const char *ssid = "****";
const char *password = "****";
const int wphDivider = 3600000; // 1000 blinks / kw
const unsigned long debounceDelay = 250; 
WiFiClient espClient;
PubSubClient client(mqtt_server,1883,espClient);

const byte interruptPin = D1;
volatile byte state = LOW;
long blinks = 0;
float lastWph = 0;
char buff[32];
unsigned long lastDebounceTime = 0; 
unsigned long lastBlinkTime = 0;

float wph = 0;

void blink()
{
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        blinks++;
        unsigned long elapsedTime = millis() - lastBlinkTime;
        wph = wphDivider / elapsedTime;
        lastBlinkTime = millis();
    }
    lastDebounceTime = millis();
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("connecting MQTT...");
        if (client.connect("powermeterv2"))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.println("failed, retrying in 5 seconds");
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());

    pinMode(interruptPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(interruptPin), blink, RISING);
    lastBlinkTime = millis();
}


void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    if (wph != lastWph) {
        lastWph = wph;
        int wint = wph;
        snprintf (buff, sizeof(buff), "%d", wint);
        Serial.println(buff);
        client.publish("/powermeter/energy", buff);
    }
}
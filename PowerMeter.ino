#include <pgmspace.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Timer.h>

// Update these with values suitable for your network.

const char* ssid = "Sensors";
const char* password = "password";
const char* mqtt_server = "192.168.88.99";

WiFiClient espClient;
PubSubClient client(espClient);

// Pulse counting settings 
int pulseCount = 0;                        // Number of pulses, used to measure energy.
int power[25] = {};                        // Array to store pulse power values
int txpower = 0;                           // powernumber to send
int txpulse = 0;                           // number of pulses to send
volatile unsigned long pulseTime,lastTime;  // Used to measure power.
int ppwh = 1;                              // Pulses per Watt hour 

// Timer used for timing callbacks
Timer callback_timer;

//----- Interupt filtering variables ---------
int minElapsed = 300;
volatile unsigned long previousTime;

///mysensors
char topic[] = "MySensorsOut/100100/";
char mySensorsData[] = "/1/0/18";
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname("PowerMeter");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {

}

void reconnect() {

  // reset the watchdog timer
  wdt_reset();
  callback_timer.update();
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Setup for report event timing
  int reportEvent = callback_timer.every(5000, send_data);

  // Attach interupt for capturing light pulses on powercentral
  pinMode(D1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D1), onPulse, FALLING);
  
  // enable the watchdog timer - 8s timeout
  wdt_enable(WDTO_8S);
  wdt_reset();

  Serial.println("Setup finished!");
}

void loop() {

  wdt_reset();
  callback_timer.update();

  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 
}

void send_data()
{
  // Calculate average over the last power meassurements before sending
  
  long _sum = 0;
  int _pulsecount = pulseCount;

  if(_pulsecount < 1)
  {
    return;
    }
  
  for(int i=1; i<=_pulsecount; i++) {
    _sum += power[i];
  }
  txpower = int(_sum / _pulsecount);
  txpulse = _pulsecount;
  
  pulseCount=0;
  power[25] = { };

  Serial.print("W: ");
  Serial.print(txpower);
  Serial.print(" - Pulse: ");
  Serial.println(txpulse); 
    
  publishData("power", txpower);
  //publishData("pulse", txpulse);
}

void publishData(const char * name, int value)
{  
  // publish to the MQTT broker 
  //client.publish(topic, msg);

   //snprintf (msg, 75, "Watt", value);

  char myConcatenation[80];
  sprintf(myConcatenation,"%s%s",topic, mySensorsData);

  // build the MQTT payload
  char payload[16];
  snprintf(payload, 16, "%i", value);

  Serial.print(myConcatenation);
  Serial.println(payload);
  client.publish(myConcatenation, payload);
}


// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse() 
{
  unsigned long elapsedTime = millis() - previousTime;

  if (elapsedTime >= minElapsed) {  //in range
  
    previousTime = millis();
    
    lastTime = pulseTime;           //used to measure time between pulses.
    pulseTime = micros();

    // Increase pulseCounter
    pulseCount++;
    
    // Size of array to avoid runtime error
    if (pulseCount < 25) {
      power[pulseCount] = int((3600000000.0 / (pulseTime - lastTime))/ppwh);  //Calculate power
      
    
      Serial.print("Power: ");
      Serial.print(power[pulseCount]);
      Serial.print(" W - Count: ");
      Serial.println(pulseCount);
    }
    else {
      Serial.println("Pulsecount over 25. Not logging....");
    }
  }
}



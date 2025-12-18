#include <WiFi.h>
#include <PubSubClient.h>

// ---- WIFI ----
const char* ssid     = "";
const char* password = "";

// ---- MQTT ----
const char* mqtt_server = "";
const int   mqtt_port   = 1883;
const char* mqtt_topic  = "train/speed";

// ---- MQTT AUTH ----
const char* mqtt_user     = "";
const char* mqtt_pass     = "";

WiFiClient espClient;
PubSubClient client(espClient);

// ---- H-BRIDGE MOTOR DRIVER ----
const int HBRIDGE_PIN1 = 32;  // H-bridge input A
const int HBRIDGE_PIN2 = 33;  // H-bridge input B

// ---- CONFIG ----
const char* status_topic = "train/controller";

// ---- CALLBACK ----
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT message on [");
  Serial.print(topic);
  Serial.print("]: ");

  // Convert payload to string
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  int speedVal = msg.toInt();
  if (speedVal >= -100 && speedVal <= 100) {
    if (speedVal > 0) {
      // Forward
      analogWrite(HBRIDGE_PIN1, map(speedVal, 0, 100, 255, 0));
      analogWrite(HBRIDGE_PIN2, 255);
    } else if (speedVal < 0) {
      // Reverse
      analogWrite(HBRIDGE_PIN1, 255);
      analogWrite(HBRIDGE_PIN2, map(speedVal, 0, -100, 255, 0));
    } else {
      // Coast
      analogWrite(HBRIDGE_PIN1, 255);
      analogWrite(HBRIDGE_PIN2, 255);
    }

    String s = String(speedVal);
    client.publish("train/controller/speed", s.c_str(), true);
  }
}

// ---- MQTT RECONNECT ----
void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT connecting... ");

    if (client.connect(
          "TrainESC",
          mqtt_user,
          mqtt_pass,
          status_topic,   // last will topic
          0,              // QoS
          true,           // retained
          "offline"       // last will payload
        )) {
      Serial.println("connected!");

      client.publish(status_topic, "online", true);

      client.subscribe(mqtt_topic);
      Serial.print("Subscribed to ");
      Serial.println(mqtt_topic);

    } else {
      Serial.print("failed (rc=");
      Serial.print(client.state());
      Serial.println("). Retrying in 3 seconds...");
      delay(3000);
    }
  }
}

void setup() {
  // Initialize H-bridge pins
  pinMode(HBRIDGE_PIN1, OUTPUT);
  pinMode(HBRIDGE_PIN2, OUTPUT);
  
  // Set initial state to coast (1,1)
  digitalWrite(HBRIDGE_PIN1, HIGH);
  digitalWrite(HBRIDGE_PIN2, HIGH);
  delay(200);

  Serial.begin(115200);
  Serial.println("Booting...");

  // ---- WIFI ----
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected. IP: " + WiFi.localIP().toString());

  // ---- MQTT ----
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

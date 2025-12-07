#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// ---- WIFI ----
const char* ssid     = "";
const char* password = "";

// ---- MQTT ----
const char* mqtt_server = "";
const int   mqtt_port   = 1883;
const char* mqtt_topic  = "train/pwm";

// ---- MQTT AUTH ----
const char* mqtt_user     = "";
const char* mqtt_pass     = "";

WiFiClient espClient;
PubSubClient client(espClient);

// ---- ESC ----
Servo esc;
const int ESC_PIN = 23;   // PWM pin

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

  int pwmVal = msg.toInt();
  if (pwmVal >= 1000 && pwmVal <= 2000) {
    Serial.print("Setting ESC to ");
    Serial.println(pwmVal);
    esc.writeMicroseconds(pwmVal);
  } else {
    Serial.println("Invalid PWM value (1000–2000 only).");
  }
}

// ---- MQTT RECONNECT ----
void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT connecting... ");

    // IMPORTANT: pass username + password here
    if (client.connect("TrainESC", mqtt_user, mqtt_pass)) {
      Serial.println("connected!");

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
  // Start ESC PWM IMMEDIATELY
  esc.attach(ESC_PIN, 1000, 2000);
  esc.writeMicroseconds(1500);   // Neutral
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

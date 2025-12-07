#include <WiFi.h>
#include <time.h>
#include <ESP32Servo.h>

// --- WIFI ---
const char* ssid     = "";
const char* password = "";

// --- TIME ---
const char* ntpServer = "ntp.se";
const long  gmtOffset_sec = 3600;     // Sweden UTC+1
const int   daylightOffset_sec = 0;   // DST adjustment if needed

// Re-sync NTP every hour
const unsigned long NTP_SYNC_INTERVAL_MS = 3600000UL;
unsigned long lastNtpSync = 0;

// --- ESC ---
Servo esc;
const int ESC_PIN   = 23;
const int NEUTRAL   = 1504;
const int FORWARD   = 1700;
const int REVERSE   = 1300;

// --- SHUTTLE TIMING ---
const unsigned long RUN_TIME_MS = 8000;

// WALL CLOCK TRIGGER: change these easily
int triggerSecond = 1800;

// State
bool goingForward = true;
bool hasTriggeredThisSecond = false;   // prevents multiple triggers

void printTimePrefix() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.print("[");
    Serial.print(buf);
    Serial.print("] ");
  } else {
    Serial.print("[no time] ");
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected! IP: " + WiFi.localIP().toString());
}

void syncTime() {
  Serial.println("Syncing time with NTP...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  int retries = 20;
  while (!getLocalTime(&timeinfo) && retries-- > 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println((retries > 0) ? "Time acquired." : "Failed to sync time.");
  lastNtpSync = millis();
}

// --- MOVEMENT FUNCTIONS ---
void runForward() {
  printTimePrefix();
  Serial.println("Running forward...");
  esc.writeMicroseconds(FORWARD);
  delay(RUN_TIME_MS);
  esc.writeMicroseconds(NEUTRAL);
  printTimePrefix();
  Serial.println("Stopped at right end.");
}

void runReverse() {
  printTimePrefix();
  Serial.println("Running reverse...");
  esc.writeMicroseconds(REVERSE);
  delay(RUN_TIME_MS);
  esc.writeMicroseconds(NEUTRAL);
  printTimePrefix();
  Serial.println("Stopped at left end.");
}

void setup() {
  // SAFELY OUTPUT NEUTRAL *BEFORE* ANYTHING ELSE
  esc.attach(ESC_PIN, 1000, 2000);
  esc.writeMicroseconds(NEUTRAL);
  delay(200);

  Serial.begin(115200);
  delay(200);

  Serial.println("Booting...");

  connectWiFi();
  syncTime();

  Serial.println("ESC armed and system ready.");
}

void loop() {
  unsigned long now = millis();

  // --- Hourly NTP re-sync ---
  if (now - lastNtpSync >= NTP_SYNC_INTERVAL_MS) {
    connectWiFi();
    syncTime();
  }

  // --- WALL CLOCK TRIGGER EVERY 20 SECONDS ---
  struct tm timeinfo;

  if (getLocalTime(&timeinfo)) {
    int sec = timeinfo.tm_sec;

    // Trigger when seconds hits 0, 20, 40
    if (sec % triggerSecond == 0) {

      if (!hasTriggeredThisSecond) {
        // Run action
        if (goingForward) runForward();
        else              runReverse();

        // Flip direction for *next* event
        goingForward = !goingForward;

        hasTriggeredThisSecond = true;
      }

    } else {
      // Reset trigger once we leave the matching second
      hasTriggeredThisSecond = false;
    }
  }

  delay(10);
}

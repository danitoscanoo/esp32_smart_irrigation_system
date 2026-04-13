#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Vector>
#include <WebServer.h>
#include <ESP.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>

const char* ssid = "NAME_WIFI_NETWORK";
const char* password = "WIFI_NETWORK_PASSWORD";

// --- NTP CONFIGURATION ---
WiFiUDP ntpUDP;

// Creates an NTPClient object.
// Arguments:
// 1. UDP object (ntpUDP)
// 2. NTP server address (pool.ntp.org is a reliable public server)
// 3. Timezone offset in seconds relative to UTC.
//    For Italy:
//    - Standard Time (CET): +1 hour = 3600 seconds
//    - Daylight Saving Time (CEST): +2 hours = 7200 seconds
//    For now, it is set for Daylight Saving Time. You will need to change it when Standard Time returns.
// 4. Time update interval in milliseconds (default is 600 seconds = 600000 ms, set explicitly here)
NTPClient timeClient(ntpUDP, "ntp1.inrim.it", 2 * 3600); // 7200 secondi = +2 ore (CEST)
const int NTP_SYNC_HOUR = 1;
bool ntpSyncedToday = false; 
const int MAX_NTP_FAILURES = 10;
int ntpFailures = 0;
const int WDT_TIMEOUT_MS = 5000;
hw_timer_t *wdt_timer = NULL;

// ---TELEGRAM BOT CONFIGURATION---
#define BOT_TOKEN "WRITE HERE YOUR TOKEN"
const unsigned long BOT_MTBS = 1000;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
unsigned long bot_lasttime;

String CHAT_ID = "WRITE HERE YOUR USER ID"; //user id

// --- VARIABLES FOR SOLENOID VALVES AND SENSOR CONTROL ---
const int RELAY_PIN_1 = 27;
const int RELAY_PIN_2 = 32;
const int RELAY_PIN_3 = 33;
// Replace 34 with the GPIO pin number connected to the rain sensor (if analog, use an ADC pin like 34, 35, 32, 33, 36, 39)
const int RAIN_SENSOR_PIN = 34;

// Rain sensor threshold (to be calibrated).
// If the sensor is analog (ADC), a lower value indicates more water.
// If your sensor is digital, this variable will not be used and you will check for HIGH/LOW.
const int RAIN_THRESHOLD = 500;

// Irrigation time and duration
const int IRRIGATION_HOUR = 6; // Example: 6 AM
const int IRRIGATION_MINUTE_1 = 0; // Example: 0 minutes
const int IRRIGATION_MINUTE_2 = 3;
const int IRRIGATION_MINUTE_3 = 6;
const int IRRIGATION_DURATION_MINUTES_1 = 2; // Irrigation 1 duration in minutes
const int IRRIGATION_DURATION_MINUTES_2 = 2; // Irrigation 2 duration in minutes
const int IRRIGATION_DURATION_MINUTES_3 = 2; // Irrigation 3 duration in minutes

// Flags to manage daily irrigation and rain history
bool irrigationScheduledToday1 = false;
bool irrigationScheduledToday2 = false;
bool irrigationScheduledToday3 = false;
// Vector to store the timestamps (Unix seconds) of rain events in the last 24 hours
std::vector<unsigned long> rainEvents;

//VALUE FOR OVERRIDE
bool OverrideManuale = false;

int infoH = 200;

// --- WEB SERVER ---
WebServer server(80);
String logBuffer="";

// Helper function to append messages to the log and print them to the serial monitor
void appendLog(String message){
  Serial.println(message);
  logBuffer += message;
  logBuffer += "\n";
  if (logBuffer.length() > 2048){
    logBuffer = logBuffer.substring(1024);
  }
}

// Function for the main web page
void handleRoot(){
  String html = "<html><head><title>Stato Irrigazione</title>";
  html += "<meta http-equiv='refresh'content='10'>";
  html += "<style>body{font-family:monospace; background-color: #f0f0f0; margin: 20px;}h1{color: #333;}pre{background-color: #fff; padding: 15px;";
  html += "border: 1px solid #ccc;border-radius: 5px; white-space: pre-wrap; word-wrap: break-word;}</style>";
  html += "</head><body>";
  html += "<h1>Stato sistema di irrigazione</h1>";
  html += "<p>Questa pagina si aggiorna automaticamente ogni 10 secondi.</p>";
  html += "<pre>";
  html += logBuffer;
  html += "</pre>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLog() {
  server.send(200, "text/plain", logBuffer);
}

void onWatchdogTimeout() {
  appendLog("Watchdog timeout, riavvio in corso…");
  ESP.restart();
}

void handleNewMessages (int numNewMessages) {
  for (int i=0; i < numNewMessages; i++) {
    String current_chatID = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
  
    if (CHAT_ID == 0) {
      CHAT_ID = current_chatID;
      bot.sendMessage(CHAT_ID, "ID utente salvato!Puoi iniziare a inviare comandi.", "");
      appendLog("ID utente telegram salvato: " + CHAT_ID);
    }
    if (current_chatID == CHAT_ID) {
      if (text == "/log") {
        if (logBuffer.length() >0 ){
          bot.sendMessage(CHAT_ID, logBuffer, "");
        }
        else {
          bot.sendMessage(CHAT_ID, "Il log è vuoto.", "");
        }
      }
      if (text == "/valvola1_on"){
        digitalWrite (RELAY_PIN_1, HIGH);
        bot.sendMessage(CHAT_ID, "Valvola 1 attivata", "");
        appendLog("Comando Telegram, valvola 1 attivata");
      }
      if (text == "/valvola1_off"){
        digitalWrite (RELAY_PIN_1, LOW);
        bot.sendMessage(CHAT_ID, "Valvola 1 disattivata", "");
        appendLog("Comando Telegram, valvola 1 disattivata");
      }
      if (text == "/valvola2_on"){
        digitalWrite (RELAY_PIN_2, HIGH);
        bot.sendMessage(CHAT_ID, "Valvola 2 attivata", "");
        appendLog("Comando Telegram, valvola 2 attivata");
      }
      if (text == "/valvola2_off"){
        digitalWrite (RELAY_PIN_2, LOW);
        bot.sendMessage(CHAT_ID, "Valvola 2 disattivata", "");
        appendLog("Comando Telegram, valvola 2 disattivata");
      }
      if (text == "/valvola3_on"){
        digitalWrite (RELAY_PIN_3, HIGH);
        bot.sendMessage(CHAT_ID, "Valvola 3 attivata", "");
        appendLog("Comando Telegram, valvola 3 attivata");
      }
      if (text == "/valvola3_off"){
        digitalWrite (RELAY_PIN_3, LOW);
        bot.sendMessage(CHAT_ID, "Valvola 3 disattivata", "");
        appendLog("Comando Telegram, valvola 3 disattivata");
      }
    }
  }
}

void telegramSendMessage(String message) {
  if (CHAT_ID != "0" && WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(CHAT_ID, message, "");
  }
}

void setup() {
  // Initialize serial communication for debugging (open the Serial Monitor to see the messages)
  Serial.begin(115200);
  wdt_timer = timerBegin(1000000);
  timerAttachInterrupt (wdt_timer, &onWatchdogTimeout);
  timerAlarm (wdt_timer, WDT_TIMEOUT_MS * 1000, false, 0);
  appendLog ("Watchdog avviato!");
  while (!Serial) {
    ;
  }
  appendLog("Avvio sistema di irrigazione...");

  // Telegram Bot Starting
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  delay(1000);
  if (CHAT_ID != 0) {
    telegramSendMessage("ESP32 avviato e pronto.");
  }


  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);
  pinMode(RELAY_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN_2, LOW);
  pinMode(RELAY_PIN_3, OUTPUT);
  digitalWrite(RELAY_PIN_3, LOW);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // --- cONNECTION TO Wi-Fi ---
  appendLog("Connection to " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  appendLog("\nWiFi connected!");
  appendLog("IP: " + WiFi.localIP().toString());

  // --- Configuration of web server ---
  server.on("/", handleRoot);
  server.on("/log", handleLog);
  server.begin();
  appendLog("Web server started!");

  // --- NTP SYNCHRONIZATION ---
  timeClient.begin();
  appendLog("NTP SYNCHRONIZATION...");
  int syncAttempts = 0;
  while(!timeClient.update() && syncAttempts < MAX_NTP_FAILURES) {
    timeClient.forceUpdate(); // Force an update if the first attempt fails
    delay(1000);
    syncAttempts++;
  }
  if (syncAttempts >= MAX_NTP_FAILURES) {
    appendLog("Error: Initial NTP synchronization failed. RESTART IN PROGRESS...");
    delay(5000);
    ESP.restart();
  }
  appendLog("Time synchronized.");
  appendLog("Current time: " + timeClient.getFormattedTime());
}

// --- hasRainedInLastHours FUNCTION: Checks if it has rained in the last N hours ---
// This function clears the rain event vector and checks if there are any recent ones.
bool hasRainedInLastHours(int hours) {
  unsigned long currentTime = timeClient.getEpochTime();
  unsigned long timeLimit = currentTime - (hours * 3600UL); // Calculate the time limit (e.g. 24 hours ago)

// Remove rain events older than the time limit
// We use an iterator to efficiently loop through and remove elements
  for (auto it = rainEvents.begin(); it != rainEvents.end(); ) {
    if (*it < timeLimit) {
      it = rainEvents.erase(it);
    } else {
      ++it;
    }
  }
  return !rainEvents.empty();
}

// --- LOOP FUNCTION: Executed repeatedly ad infinitum ---
void loop() {
  timerWrite(wdt_timer, 0);
  server.handleClient();
  timeClient.forceUpdate();

  if (millis() > bot_lasttime + BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentSecond = timeClient.getSeconds();
  int currentDayOfWeek = timeClient.getDay(); // Day of the week (0=Sunday, 1=Monday... 6=Saturday)
  unsigned long currentEpochTime = timeClient.getEpochTime();

  if (currentHour == NTP_SYNC_HOUR && !ntpSyncedToday) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!timeClient.update()) {
        //ntpFailures++;
        appendLog("Error: incorrect synchronization, attempt number " + String(ntpFailures));
      }
      else {
        ntpSyncedToday = true;
        appendLog("Time synced correctly for today");
      }
    }
    else {
      appendLog("Unable to sync, wifi not connected");
    }
  }

  if (currentHour == (NTP_SYNC_HOUR + 1) && ntpSyncedToday) {
    ntpSyncedToday = false;
    appendLog("New day, sync flag reset");
  }

  if (ntpFailures >= MAX_NTP_FAILURES) {
    appendLog("Error: Daily NTP synchronization failed. RESTART IN PROGRESS...");
    delay(5000);
    ESP.restart();
  }

// --- RAIN SENSOR LOGIC ---
//int rainSensorValue = analogRead(RAIN_SENSOR_PIN); // Read the value from the analog sensor
  bool isRaining = digitalRead(RAIN_SENSOR_PIN);

  if (isRaining == LOW) {
    if (rainEvents.empty() || (currentEpochTime - rainEvents.back() > 60UL)) {
      rainEvents.push_back(currentEpochTime);
      appendLog("Pioggia rilevata a: " + timeClient.getFormattedTime());
      appendLog("Eventi pioggia registrati: " + String(rainEvents.size()));
    }
  }

  // --- TIMED IRRIGATION LOGIC ---
  // Checks whether the irrigation start time is reached and whether it has not already been performed today.
  if (currentHour == IRRIGATION_HOUR && currentMinute == IRRIGATION_MINUTE_1 && !irrigationScheduledToday1) {
    if (hasRainedInLastHours(24)) {
      appendLog("Rain detected in the last 24 hours. Irrigation 1 canceled for today.");
      irrigationScheduledToday1 = true; // Mark as done for today to not try again
    } else {
      appendLog("Start irrigation 1...");
      digitalWrite(RELAY_PIN_1, HIGH);
      unsigned long irrigationStartTime = millis();
      while (millis() - irrigationStartTime < (IRRIGATION_DURATION_MINUTES_1 * 60UL * 1000UL)) {
        delay(100);
      }

      // If the valve is still open (it has not been interrupted by live rain)
      if (digitalRead(RELAY_PIN_1) == HIGH) {
        appendLog("End irrigation 1.");
        digitalWrite(RELAY_PIN_1, LOW);
      }
      irrigationScheduledToday1 = true;
    }
  }
  //IRRIGATION 2
  if (currentHour == IRRIGATION_HOUR && currentMinute == IRRIGATION_MINUTE_2 && !irrigationScheduledToday2) {
    if (hasRainedInLastHours(24)) {
      appendLog("Rain detected in the last 24 hours. Irrigation 2 canceled for today.");
      irrigationScheduledToday2 = true;
    } else {
      appendLog("Start irrigation 2...");
      digitalWrite(RELAY_PIN_2, HIGH);

      unsigned long irrigationStartTime = millis();
      while (millis() - irrigationStartTime < (IRRIGATION_DURATION_MINUTES_2 * 60UL * 1000UL)) {
        delay(100);
      }

      if (digitalRead(RELAY_PIN_2) == HIGH) {
        appendLog("End irrigation 2...");
        digitalWrite(RELAY_PIN_2, LOW);
      }
      irrigationScheduledToday2 = true;
    }
  }
  //IRRIGATION 3
  if (currentHour == IRRIGATION_HOUR && currentMinute == IRRIGATION_MINUTE_3 && !irrigationScheduledToday3) {
    if (hasRainedInLastHours(24)) {
      appendLog("Rain detected in the last 24 hours. Irrigation 3 canceled for today.");
      irrigationScheduledToday3 = true;
    } else {
      appendLog("Start irrigation 3...");
      digitalWrite(RELAY_PIN_3, HIGH);

      unsigned long irrigationStartTime = millis();
      while (millis() - irrigationStartTime < (IRRIGATION_DURATION_MINUTES_3 * 60UL * 1000UL)) {
        delay(100);
      }

      if (digitalRead(RELAY_PIN_3) == HIGH) {
        appendLog("End irrigation 3.");
        digitalWrite(RELAY_PIN_3, LOW);
      }
      irrigationScheduledToday3 = true;
    }
  }

  // --- RESET THE IRRIGATION FLAG AT MIDNIGHT ---
  // This resets the 'irrigationScheduledToday' flag for the next day.
  // It triggers shortly after midnight (e.g., 1 AM).
  if (currentHour == 1 && currentMinute == 0 && irrigationScheduledToday1) {
    irrigationScheduledToday1 = false;
    appendLog("New day, watering flag reset.");
  }

  if (currentHour == 1 && currentMinute == 0 && irrigationScheduledToday2) {
    irrigationScheduledToday2 = false;
    appendLog("New day, watering flag reset.");
  }

  if (currentHour == 1 && currentMinute == 0 && irrigationScheduledToday3) {
    irrigationScheduledToday3 = false;
    appendLog("New day, watering flag reset.");
  }

  // -- MANUAL CONTROL LOGIC FROM SERIAL PORT --
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command=="ON") {
      OverrideManuale = true;
      appendLog("Override command received, value: ON");
    }
    else if (command=="OFF"){
      OverrideManuale = false;
      appendLog("Override command received, value: OFF");
    }
    else if (command=="RESET RAIN"){
      rainEvents.clear();
      appendLog("Rain vector reset");
    }
    else {
      appendLog(String("Command not recognized. \n") + "Available commands:\n" + "1. ON \n" + "2. OFF \n" + "3. RAIN RESET \n");
    }
    //Valve override actuation
    if (OverrideManuale) {
      digitalWrite(RELAY_PIN_1, HIGH);
    }
    if (!OverrideManuale) {
      digitalWrite(RELAY_PIN_1, LOW);
    }
  }

 // Small delay in the loop to avoid overloading the processor
  delay(1000);
}
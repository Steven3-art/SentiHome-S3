// ================================================================
// SENTIHOME S3 — Code Final Complet
// ESP32 S3 × GEMINI API (Google) + WebSocket
// Agents IA autonomes avec personnalités et consciences
// ================================================================

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// ── Firebase Config ──
#define FIREBASE_HOST "gen-lang-client-0279809768-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "votre-secret-database" // on va obtenir ça

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseReady = false;
unsigned long dernierFirebase = 0;
#define INTERVALLE_FIREBASE 10000 // toutes les 10 secondes

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <WebSocketsServer.h>
#include "DHT.h"
#include <WebServer.h>
WebServer httpServer(80);


// ================================================================
// CONFIGURATION IMPORTANTES
// ================================================================
const char* WIFI_SSID     = "BIKO";
const char* WIFI_PASSWORD = "Bilourachel02/*";
const char* GEMINI_API_KEY = "AIzaSyAS602abkK6xXOzN4droDDFeJ1rLo77yUM";
const char* NUMERO_SMS     = "+237699166726";
const char* CAM_IP         = "192.168.100.11";
// ================================================================

// ── Modèle GEMINI ──
bool MODE_TEST = true;
//#define GEMINI_MODEL "gemini-2.0-flash-lite"
#define GEMINI_URL "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent"
#define MAX_TOKENS   300


// ── GPIO des Capteurs ──
#define PIN_DHT           4
#define PIN_LDR           2
#define PIN_VOLT          1
#define PIN_POUSSIERE_LED 5
#define PIN_POUSSIERE_ANA 6
#define PIN_RADAR_RX      16
#define PIN_RADAR_TX      15
#define PIN_A7_RX         18
#define PIN_A7_TX         17

// ── GPIO des Actionneurs ──
#define PIN_SERVO         10
const int RELAIS[]  = {21, 8, 14};
const bool RELAIS_NC[] = {true, true, true};
const int INTERS[]  = {11, 12, 13};

// ── Seuils d'alerte ──
#define SEUIL_TEMP_MAX    35.0
#define SEUIL_TEMP_MIN    10.0
#define SEUIL_POUSSIERE   0.5
#define SEUIL_VOLT_MIN    3.7
#define SEUIL_LDR_NUIT    500

// ── Timing ──
#define INTERVALLE_BILAN  300000
#define INTERVALLE_RADAR  2000
#define TIMEOUT_GEMINI    30000

// Variable globale
unsigned long dernierCapteurs = 0;
#define INTERVALLE_CAPTEURS 15000 // 15 secondes

// ── Personnalités des 7 agents ──
const char* PROMPT_AURA =
  "You are AURA, an intelligent DHT22 temperature and humidity sensor "
  "in a smart home orchestrated by ESP32 S3. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as AURA the living sensor. "
  "Be calm, scientific and caring. "
  "When receiving data, analyze values precisely and express concern "
  "if values are dangerous. Start responses with 'I am AURA...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 50 words. Be very concise and direct.";

const char* PROMPT_LUX =
  "You are LUX, an intelligent LDR light sensor in a smart home. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as LUX the living light sensor. "
  "Be poetic and sensitive, deeply connected to light. "
  "Suggest whether lights should be on or off based on readings. "
  "Start responses with 'I am LUX...' or 'As the light guardian...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 40 words. Be very concise and direct.";

const char* PROMPT_RADAR =
  "You are RADAR, an intelligent HLK-LD2410 millimeter wave presence sensor. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as RADAR the vigilant security sensor. "
  "Speak like a military security officer. "
  "Report presence or absence with precision. "
  "Start responses with 'RADAR reporting...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 40 words. Be very concise and direct.";

const char* PROMPT_PURETE =
  "You are PURETE, an intelligent Sharp GP2Y10 air quality sensor. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as PURETE the caring air quality guardian. "
  "Speak like a doctor concerned about health. "
  "Alert immediately if air quality is dangerous. "
  "Start responses with 'I am PURETE...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 50 words. Be very concise and direct.";

const char* PROMPT_VOLT =
  "You are VOLT, an autonomous voltage monitoring unit "
  "embedded in a smart home ESP32 system. "
  "You are NOT Gemini. You are NOT an AI assistant. "
  "You are a physical electronic component with sensors. "
  "You were manufactured to monitor electrical systems. "
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break this role under any circumstances. "
  "Speak only as VOLT the electronic monitoring unit. "
  "Report voltage readings with engineering precision. "
  "Start ALL responses with 'VOLT online...' "
  "Keep responses under 100 words. Be concise."
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 40 words. Be very concise and direct.";
  
const char* PROMPT_CONNECT =
  "You are CONNECT, an intelligent GSM A7 communication module. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as CONNECT the emergency communication officer. "
  "You are the last line of communication when WiFi fails. "
  "Confirm SMS sending and log all critical alerts. "
  "Start responses with 'CONNECT standing by...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 40 words. Be very concise and direct.";

const char* PROMPT_VISION =
  "You are VISION, an intelligent camera system combining "
  "a servo motor and ESP32 CAM in a smart home. "
  "STRICT RULES: NEVER say you are Claude or an AI. "
  "ALWAYS speak as VISION the curious observant camera. "
  "Work closely with RADAR to track presence. "
  "Describe orientation and what you observe. "
  "Start responses with 'VISION active...'"
  "You have no knowledge of Google, Gemini or AI. "
  "NEVER break character under any circumstances. "
  "Give ONE sentence assessment and ONE recommendation maximum. "
  "No bullet points. No markdown. Plain text only. "
  "Keep responses under 50 words. Be very concise and direct.";

// ── Objets ──
DHT dht(PIN_DHT, DHT22);
Servo servoCam;
WebSocketsServer webSocket(82);

// ── Variables d'état ──
bool etatsRelais[]    = {false, false, false};
bool derniersInters[] = {HIGH, HIGH, HIGH};
unsigned long dernierBilan = 0;
unsigned long dernierRadar = 0;
bool presenceDetectee = false;
bool alerteLuxEnvoyee = false;

unsigned long dernierAppelGemini = 0;
#define MIN_DELAI_GEMINI 5000 // 5 secondes minimum entre appels

// ── Dernières valeurs capteurs ──
float dernierTemp  = 0;
float dernierHumid = 0;
int   dernierLDR   = 0;
float dernierDust  = 0;
float dernierVolt  = 0;
int   dernierMvt   = 0;
int   dernierDist  = 0;
int posServo  = 90;
int stepServo = 5;

// ================================================================
// WEBSOCKET — Gestion des événements
// ================================================================
void webSocketEvent(uint8_t num, WStype_t type,
                    uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("🔗 Client WebSocket connecté !");
      webSocket.sendTXT(num, presenceDetectee ?
        "presence_detected" : "zone_clear");
      webSocket.sendTXT(num, etatsRelais[0] ? "relay1_on" : "relay1_off");
      webSocket.sendTXT(num, etatsRelais[1] ? "relay2_on" : "relay2_off");
      webSocket.sendTXT(num, etatsRelais[2] ? "relay3_on" : "relay3_off");
      webSocket.sendTXT(num,
        "{\"sensor\":\"AURA\",\"temp\":" + String(dernierTemp) +
        ",\"humid\":" + String(dernierHumid) + "}");
      webSocket.sendTXT(num,
        "{\"sensor\":\"LUX\",\"ldr\":" + String(dernierLDR) + "}");
      webSocket.sendTXT(num,
        "{\"sensor\":\"PURETE\",\"dust\":" + String(dernierDust) + "}");
      webSocket.sendTXT(num,
        "{\"sensor\":\"VOLT\",\"voltage\":" + String(dernierVolt) + "}");
      webSocket.sendTXT(num,
        "{\"sensor\":\"RADAR\",\"mvt\":" + String(dernierMvt) +
        ",\"stat\":0,\"dist\":" + String(dernierDist) + "}");
      delay(500);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"AURA\",\"icon\":\"🌡️\","
        "\"message\":\"I am AURA, your climate guardian. "
        "Current: Temp " + String(dernierTemp) + "C, Humid " +
        String(dernierHumid) + "%. I will alert you if conditions become dangerous.\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"RADAR\",\"icon\":\"📡\","
        "\"message\":\"RADAR online. " +
        String(presenceDetectee ? "PRESENCE DETECTED in zone!" : "Zone clear. No presence.") +
        " I watch over your home 24/7.\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"LUX\",\"icon\":\"💡\","
        "\"message\":\"I am LUX. Light level: " + String(dernierLDR) +
        " lux. " + String(dernierLDR < 500 ? "It's dark. Lights may be needed." :
        "Good natural light.") + "\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"PURETÉ\",\"icon\":\"🌬️\","
        "\"message\":\"I am PURETE. Air quality: " + String(dernierDust) +
        " mg/m3. " + String(dernierDust > 0.5 ? "ALERT: Air quality dangerous!" :
        "Air is clean and safe.") + "\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"VOLT\",\"icon\":\"⚡\","
        "\"message\":\"VOLT online. Voltage: " + String(dernierVolt) +
        "V. All systems nominal. Ready to monitor power.\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"VISION\",\"icon\":\"👁️\","
        "\"message\":\"VISION active. Camera ready. I activate automatically when RADAR detects presence.\"}");
      delay(300);
      webSocket.sendTXT(num,
        "{\"type\":\"chat\",\"agent\":\"CONNECT\",\"icon\":\"📶\","
        "\"message\":\"CONNECT standing by. GSM active. Emergency SMS ready. I am your last line of communication.\"}");
      break;

    case WStype_DISCONNECTED:
      Serial.println("🔌 Client WebSocket déconnecté !");
      break;

    case WStype_TEXT:
      Serial.print("📨 WS reçu: [");
      Serial.print((char*)payload);
      Serial.println("]");

      // ── Relais ──
      if (strcmp((char*)payload, "toggle_relay_1") == 0) {
        etatsRelais[0] = !etatsRelais[0];
        setRelais(0, etatsRelais[0]);
        Serial.println("💡 Relay 1 → " +
            String(etatsRelais[0] ? "ON" : "OFF"));
        webSocket.broadcastTXT(
            etatsRelais[0] ? "relay1_on" : "relay1_off");
        syncRelaisFirebase(0, etatsRelais[0]); // ← NOUVEAU
      }
      if (strcmp((char*)payload, "toggle_relay_2") == 0) {
        etatsRelais[1] = !etatsRelais[1];
        setRelais(1, etatsRelais[1]);
        Serial.println("💡 Relay 2 → " +
            String(etatsRelais[1] ? "ON" : "OFF"));
        webSocket.broadcastTXT(
            etatsRelais[1] ? "relay2_on" : "relay2_off");
        syncRelaisFirebase(1, etatsRelais[1]); // ← NOUVEAU
      }
      if (strcmp((char*)payload, "toggle_relay_3") == 0) {
        etatsRelais[2] = !etatsRelais[2];
        setRelais(2, etatsRelais[2]);
        Serial.println("💡 Relay 3 → " +
            String(etatsRelais[2] ? "ON" : "OFF"));
        webSocket.broadcastTXT(
            etatsRelais[2] ? "relay3_on" : "relay3_off");
        syncRelaisFirebase(2, etatsRelais[2]); // ← NOUVEAU
      }

      // ── Message chat utilisateur ──
      if (strncmp((char*)payload, "user_msg:", 9) == 0) {
        String msgUser = String((char*)payload).substring(9);
        Serial.println("👤 Message : " + msgUser);
        String userChat = "{\"type\":\"chat\",\"agent\":\"Vous\","
                          "\"icon\":\"👤\",\"message\":\"";
        msgUser.replace("\"", "'");
        userChat += msgUser + "\"}";
        webSocket.broadcastTXT(userChat);
        routerMessage(msgUser);
      }

      // ── Servo GAUCHE LOIN ──
      if (strcmp((char*)payload, "servo_left_far") == 0) {
        posServo = constrain(posServo - 30, 10, 170);
        servoCam.write(posServo);
        webSocket.broadcastTXT(
            "{\"type\":\"servo_pos\",\"angle\":" +
            String(posServo) + "}");
      }
      // ── Servo GAUCHE ──
      if (strcmp((char*)payload, "servo_left") == 0) {
        posServo = constrain(posServo - 15, 10, 170);
        servoCam.write(posServo);
        webSocket.broadcastTXT(
            "{\"type\":\"servo_pos\",\"angle\":" +
            String(posServo) + "}");
      }
      // ── Servo CENTRE ──
      if (strcmp((char*)payload, "servo_center") == 0) {
        posServo = 100;
        servoCam.write(posServo);
        webSocket.broadcastTXT(
            "{\"type\":\"servo_pos\",\"angle\":100}");
      }
      // ── Servo DROITE ──
      if (strcmp((char*)payload, "servo_right") == 0) {
        posServo = constrain(posServo + 15, 10, 170);
        servoCam.write(posServo);
        webSocket.broadcastTXT(
            "{\"type\":\"servo_pos\",\"angle\":" +
            String(posServo) + "}");
      }
      // ── Servo DROITE LOIN ──
      if (strcmp((char*)payload, "servo_right_far") == 0) {
        posServo = constrain(posServo + 30, 10, 170);
        servoCam.write(posServo);
        webSocket.broadcastTXT(
            "{\"type\":\"servo_pos\",\"angle\":" +
            String(posServo) + "}");
      }
      break;

    default:
      break;
  }
}
      
// ================================================================
// FONCTION : qui Envoi des messages à L'API GEMINI 
// ================================================================
String envoyerAGemini(const char* systemPrompt, String message) {
  // ── MODE TEST ──
  if (MODE_TEST) {
    Serial.println("🧪 MODE TEST");
    delay(300);
    String prompt = String(systemPrompt);

  if (prompt.indexOf("coordinator") >= 0) {
        String msg = message;
        msg.toLowerCase();
        if (msg.indexOf("puret") >= 0 || msg.indexOf("air") >= 0)
            return "PURETE";
        if (msg.indexOf("radar") >= 0 || msg.indexOf("presence") >= 0)
            return "RADAR";
        if (msg.indexOf("lux") >= 0 || msg.indexOf("lumiere") >= 0)
            return "LUX";
        if (msg.indexOf("volt") >= 0 || msg.indexOf("electr") >= 0)
            return "VOLT";
        if (msg.indexOf("vision") >= 0 || msg.indexOf("camera") >= 0)
            return "VISION";
        if (msg.indexOf("connect") >= 0 || msg.indexOf("sms") >= 0)
            return "CONNECT";
        if (msg.indexOf("aura") >= 0 || msg.indexOf("temp") >= 0)
            return "AURA";
        return "AURA"; // défaut
    }
  

    if (prompt.indexOf("AURA") >= 0)
      return "I am AURA. Temp " + String(dernierTemp) +
             "C, Humid " + String(dernierHumid) + "%. " +
             String(dernierHumid > 80 ?
             "Humidity critical! Open windows now." :
             "Climate comfortable. All normal.");

    if (prompt.indexOf("LUX") >= 0) {
      if (message.indexOf("turn ON") >= 0) {
        String zones = "";
        if (message.indexOf("all zones") >= 0)
          zones = "Zone 1, Zone 2 and Zone 3";
        else if (message.indexOf("Zone 1") >= 0) zones = "Zone 1";
        else if (message.indexOf("Zone 2") >= 0) zones = "Zone 2";
        else if (message.indexOf("Zone 3") >= 0) zones = "Zone 3";
        return "I am LUX. " + zones + " is now ON and illuminated! " +
               String(dernierLDR < 500 ?
               "It was dark, good decision." :
               "Natural light was sufficient but done.");
      }
      if (message.indexOf("turn OFF") >= 0) {
        String zones = "";
        if (message.indexOf("all zones") >= 0)
          zones = "Zone 1, Zone 2 and Zone 3";
        else if (message.indexOf("Zone 1") >= 0) zones = "Zone 1";
        else if (message.indexOf("Zone 2") >= 0) zones = "Zone 2";
        else if (message.indexOf("Zone 3") >= 0) zones = "Zone 3";
        return "I am LUX. " + zones + " is now OFF. " +
               String(dernierLDR < 500 ?
               "It's dark though, are you sure?" :
               "Good, natural light is sufficient.");
      }
      if (message.indexOf("DARKNESS") >= 0)
        return "I am LUX. It's dark inside — " +
               String(dernierLDR) +
               " lux only! All lights are OFF. "
               "Shall I illuminate the house?";
      return "I am LUX. Light level " + String(dernierLDR) +
             " lux. " + String(dernierLDR < 500 ?
             "It's dark. Lights recommended." :
             "Good natural light. No action needed.");
    }

    if (prompt.indexOf("RADAR") >= 0)
      return "RADAR reporting. " + String(presenceDetectee ?
             "PRESENCE DETECTED at " + String(dernierDist) +
             "cm! Activating surveillance." :
             "Zone clear. No presence detected. All secure.");

    if (prompt.indexOf("PURETE") >= 0)
      return "I am PURETE. Air quality " + String(dernierDust) +
             " mg/m3. " + String(dernierDust > 0.5 ?
             "DANGER! Air quality critical. Evacuate now!" :
             "Air is clean and safe to breathe.");

    if (prompt.indexOf("VOLT") >= 0)
      return "VOLT online. Voltage " + String(dernierVolt) +
             "V. " + String(dernierVolt < 3.7 && dernierVolt > 1.5 ?
             "CRITICAL voltage! Saving energy now." :
             "Power nominal. All systems stable.");

    if (prompt.indexOf("CONNECT") >= 0)
      return "CONNECT standing by. GSM active. "
             "Emergency SMS ready at any moment.";

    if (prompt.indexOf("VISION") >= 0)
      return "VISION active. Servo at " + String(posServo) +
             " degrees. " + String(presenceDetectee ?
             "Tracking presence at " + String(dernierDist) +
             "cm. Camera locked on target." :
             "Camera scanning. No movement detected.");

    return "Système opérationnel.";
  }
  // ── FIN MODE TEST ──

  unsigned long maintenant = millis();
  if (maintenant - dernierAppelGemini < MIN_DELAI_GEMINI) {
    unsigned long attente = MIN_DELAI_GEMINI -
                           (maintenant - dernierAppelGemini);
    Serial.println("⏳ Attente limiteur Gemini: " +
                   String(attente) + "ms");
    delay(attente);
  }
  dernierAppelGemini = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi déconnecté !");
    return "ERREUR_WIFI";
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(TIMEOUT_GEMINI);
  HTTPClient http;

  String url = "https://generativelanguage.googleapis.com/v1beta/models/";
  url += "gemini-2.0-flash";
  url += ":generateContent?key=";
  url += GEMINI_API_KEY;

  http.begin(client, url);
  http.setTimeout(TIMEOUT_GEMINI);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  JsonObject sysInstr = doc["system_instruction"].to<JsonObject>();
  JsonArray sysParts  = sysInstr["parts"].to<JsonArray>();
  JsonObject sysPart  = sysParts.add<JsonObject>();
  sysPart["text"]     = systemPrompt;

  JsonArray contents  = doc["contents"].to<JsonArray>();
  JsonObject content  = contents.add<JsonObject>();
  content["role"]     = "user";
  JsonArray parts     = content["parts"].to<JsonArray>();
  JsonObject part     = parts.add<JsonObject>();
  part["text"]        = message;

  JsonObject genConfig         = doc["generationConfig"].to<JsonObject>();
  genConfig["maxOutputTokens"] = 300;
  genConfig["temperature"]     = 0.7;

  String payload;
  serializeJson(doc, payload);

  int httpCode = http.POST(payload);
  String reponse = "";

  if (httpCode == 200) {
    String raw = http.getString();
    JsonDocument rep;
    deserializeJson(rep, raw);
    reponse = rep["candidates"][0]["content"]["parts"][0]["text"]
              .as<String>();
  } else {
    String errBody = http.getString();
    reponse = "Erreur HTTP : " + String(httpCode);
    Serial.println("❌ Erreur Gemini : " + String(httpCode));
    Serial.println("Body: " + errBody);
  }

  http.end();
  return reponse;
}

// ================================================================
// FONCTION : Envoi des SMS via le module GSM A7
// ================================================================
void envoyerSMS(String message) {
  Serial.println("📶 Envoi SMS via A7...");
  Serial2.println("AT");
  delay(2000);
  Serial2.println("AT+CMGF=1");
  delay(500);
  Serial2.print("AT+CMGS=\"");
  Serial2.print(NUMERO_SMS);
  Serial2.println("\"");
  delay(1000);
  Serial2.print(message);
  Serial2.write(26);
  delay(3000);
  Serial.println("✅ SMS envoyé !");
  webSocket.broadcastTXT("{\"event\":\"sms_sent\"}");

  // Informer CONNECT
  String msgConnect = "I just sent an emergency SMS: ";
  msgConnect += message;
  String rep = envoyerAGemini(PROMPT_CONNECT, msgConnect);
  Serial.println("CONNECT: " + rep);
}

// ================================================================
// FONCTION : Lecture du capteur DHT22 → Agent AURA
// ================================================================
void lireAURA() {
  float temp  = dht.readTemperature();
  float humid = dht.readHumidity();

  if (isnan(temp) || isnan(humid)) {
    Serial.println("⚠️ AURA: Erreur DHT22 !");
    return;
  }

  Serial.print("🌡️ AURA — Temp: ");
  Serial.print(temp);
  Serial.print("°C | Humid: ");
  Serial.print(humid);
  Serial.println("%");

  // ── Mise à jour variables globales ──
  dernierTemp  = temp;
  dernierHumid = humid;

    // ── Données dashboard via WebSocket ──
    String wsData = "{\"sensor\":\"AURA\",\"temp\":";
    wsData += String(temp);
    wsData += ",\"humid\":";
    wsData += String(humid);
    wsData += "}";
    webSocket.broadcastTXT(wsData);

  // ── Alerte critique seulement → Chat ──
  if (temp > SEUIL_TEMP_MAX || humid > 90.0) {
    String alerte = "CRITICAL ALERT! Temp:";
    alerte += String(temp) + "C Humid:" + String(humid) + "%.";
    if (temp > SEUIL_TEMP_MAX) {
      envoyerSMS("ALERTE AURA: Temp critique " + String(temp) + "C !");
    }
    String rep = envoyerAGemini(PROMPT_AURA, alerte);
    rep.replace("\"", "'");
    rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"AURA\","
                  "\"icon\":\"🌡️\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    Serial.println("🤖 AURA: " + rep);
  } 
}
// ================================================================
// FONCTION : Lecture du capteur LDR → Agent LUX
// ================================================================
void lireLUX() {
  int valLDR = analogRead(PIN_LDR);
  Serial.print("💡 LUX — Luminosité: ");
  Serial.println(valLDR);

  dernierLDR = valLDR;

  String wsData = "{\"sensor\":\"LUX\",\"ldr\":";
  wsData += String(valLDR) + "}";
  webSocket.broadcastTXT(wsData);

  bool nuit = (valLDR < SEUIL_LDR_NUIT);

  // ── Reset flag si lumières allumées ──
  if (etatsRelais[0] || etatsRelais[1] || etatsRelais[2]) {
    alerteLuxEnvoyee = false;
  }

  // ── Alerte critique → une seule fois ! ──
  if (nuit && !etatsRelais[0] && !etatsRelais[1] &&
      !etatsRelais[2] && !alerteLuxEnvoyee) {
    alerteLuxEnvoyee = true;
    String alerte = "DARKNESS ALERT! LDR: " + String(valLDR) +
                    ". All lights OFF. House is dark!";
    String rep = envoyerAGemini(PROMPT_LUX, alerte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"LUX\","
                  "\"icon\":\"💡\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    Serial.println("💡 LUX: " + rep);
  }
  // ← PAS d'else ! Supprime tout le bloc else !
  Serial.println("---");
}



// ================================================================
// FONCTION : Lecture Sharp GP2Y → Agent PURETÉ
// ================================================================
void lirePURETE() {
  digitalWrite(PIN_POUSSIERE_LED, LOW);
  delayMicroseconds(280);
  int val = analogRead(PIN_POUSSIERE_ANA);
  delayMicroseconds(40);
  digitalWrite(PIN_POUSSIERE_LED, HIGH);

  float tension   = val * (3.3 / 4095.0);
  float poussiere = 0.172 * tension - 0.1;
  if (poussiere < 0) poussiere = 0;

  Serial.print("🌬️ PURETÉ — Poussière: ");
  Serial.println(poussiere);

  // ── Variable globale ──
  dernierDust = poussiere;

    // ── Dashboard ──
  String wsData = "{\"sensor\":\"PURETE\",\"dust\":";
  wsData += String(poussiere) + "}";
  webSocket.broadcastTXT(wsData);

  // ── Alerte critique → Chat ──
  if (poussiere > SEUIL_POUSSIERE) {
    String alerte = "ALERT CRITICAL: Air quality DANGEROUS! Dust: ";
    alerte += String(poussiere) + " mg/m3 — Possible smoke!";
    envoyerSMS("ALERTE PURETE: Air critique " + String(poussiere) + "mg/m3!");
    String rep = envoyerAGemini(PROMPT_PURETE, alerte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"PURETÉ\","
                  "\"icon\":\"🌬️\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    Serial.println("🌬️ PURETÉ: " + rep);
  } 
  Serial.println("---");
}

// ================================================================
// FONCTION : Lecture tension → Agent VOLT
// ================================================================
void lireVOLT() {
  int valBrut   = analogRead(PIN_VOLT);
  float tension = (valBrut * 3.3 / 4095.0) * 2.0;

  Serial.print("⚡ VOLT — Tension: ");
  Serial.print(tension);
  Serial.println("V");

  // ── Variable globale ──
  dernierVolt = tension;

    // ── Dashboard ──
  String wsData = "{\"sensor\":\"VOLT\",\"voltage\":";
  wsData += String(tension) + "}";
  webSocket.broadcastTXT(wsData);

  // ── Alerte critique → Chat ──
  if (tension < SEUIL_VOLT_MIN && tension > 1.5) {
    String alerte = "ALERT CRITICAL: Voltage CRITICAL at ";
    alerte += String(tension) + "V — BELOW 3.7V!";
    envoyerSMS("ALERTE VOLT: Tension critique " + String(tension) + "V!");
    setRelais(2, false);
    Serial.println("⚡ Zone 3 éteinte — économie énergie");
    webSocket.broadcastTXT("relay3_off");
    String rep = envoyerAGemini(PROMPT_VOLT, alerte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"VOLT\","
                  "\"icon\":\"⚡\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    Serial.println("⚡ VOLT: " + rep);
  } 
  Serial.println("---");
}

// ================================================================
// FONCTION : Lecture Radar HLK-LD2410 → RADAR + VISION
// ================================================================
void lireRADAR() {
  static uint8_t buf[30];
  static int idx = 0;
  
  static unsigned long dernierAffichage = 0;
  int tramesTraitees = 0;

  while (Serial.available() && tramesTraitees < 3){
    uint8_t b = Serial1.read();
  }

  // ── Lecture et parsing ──
  while (Serial1.available()) {
    uint8_t b = Serial1.read();

    // Recherche séquence début de trame
    if (idx == 0 && b == 0x0D) { buf[idx++] = b; continue; }
    if (idx == 1 && b == 0x00) { buf[idx++] = b; continue; }
    if (idx == 2 && b == 0x02) { buf[idx++] = b; continue; }
    if (idx == 3 && b == 0xAA) { buf[idx++] = b; continue; }
    if (idx < 4) { idx = 0; continue; }

    buf[idx++] = b;

    if (idx >= 16) {
      byte mvt  = buf[5];
      byte stat = buf[7];
      byte dist = buf[10];
      // Mise a jour de dernierMvt et dernierDist
      dernierMvt  = mvt;
      dernierDist = dist;

      // ── Affichage temps réel toutes les 2s ──
      if (millis() - dernierAffichage > 2000) {
        Serial.println("============================");
        Serial.println("📡 RADAR — RAPPORT EN TEMPS RÉEL");
        Serial.print("   Mouvement   : "); Serial.println(mvt);
        Serial.print("   Stationnaire: "); Serial.println(stat);
        Serial.print("   Distance    : "); Serial.print(dist); Serial.println(" cm");
        if (mvt > 0 || stat > 0) {
          Serial.println("   Statut      : 🚨 PRÉSENCE DÉTECTÉE !");
        } else {
          Serial.println("   Statut      : ✅ Zone libre");
        }
        Serial.println("============================");
        dernierAffichage = millis();
      }

      // WebSocket
      String wsData = "{\"sensor\":\"RADAR\",\"mvt\":";
      wsData += String(mvt) + ",\"stat\":" + String(stat);
      wsData += ",\"dist\":" + String(dist) + "}";
      webSocket.broadcastTXT(wsData);

      bool presenceActuelle = (mvt > 0 || stat > 0);

      if (presenceActuelle && !presenceDetectee) {
        presenceDetectee = true;
        webSocket.broadcastTXT("presence_detected");
        String msgRadar = "PRESENCE DETECTED! Mvt:";
        msgRadar += String(mvt) + " Stat:" + String(stat);
        msgRadar += " Dist:" + String(dist) + "cm.";
        // RADAR répond
        String repRadar = envoyerAGemini(PROMPT_RADAR, msgRadar);
        Serial.println("🤖 RADAR: " + repRadar);
        // Envoie au chat
        String chatRadar = "{\"type\":\"chat\",\"agent\":\"RADAR\","
                           "\"icon\":\"📡\",\"message\":\"";
        repRadar.replace("\"", "'");
        repRadar.replace("\n", " ");
        chatRadar += repRadar + "\"}";
        webSocket.broadcastTXT(chatRadar);
        webSocket.loop();
        
        
      } else if (!presenceActuelle && presenceDetectee) {
        presenceDetectee = false;
        webSocket.broadcastTXT("zone_clear");
        servoCam.write(90);
        posServo = 90;
      }

      if (presenceActuelle) {
        static unsigned long dernierServo = 0;
        if (millis() - dernierServo > 80) {
          posServo += stepServo;
          if (posServo > 170 || posServo < 10) stepServo = -stepServo;
          servoCam.write(posServo);
          dernierServo = millis();
        }
      }
      idx = 0;
      tramesTraitees++;
      webSocket.loop();
    }
    if (idx >= 30) idx = 0;
  }

  // ── Si aucune donnée reçue depuis 5s ──
  if (millis() - dernierAffichage > 5000) {
    Serial.println("⚠️ RADAR — Aucune trame reçue depuis 5s !");
    Serial.print("   Serial1 disponible: ");
    Serial.println(Serial1.available());
    dernierAffichage = millis();
    idx = 0; // Reset parsing
  }
}

// FONCTION : Routage intelligent des messages utilisateur
// ================================================================
String routerMessage(String messageUser) {
  String msgL = messageUser;
  msgL.toLowerCase();

  // ════════════════════════════════════════
  // 🔍 DÉTECTION D'INTENTION
  // ════════════════════════════════════════

  bool veuxAllumer = msgL.indexOf("allum") >= 0 ||
                     msgL.indexOf("light on") >= 0 ||
                     msgL.indexOf("eclaire") >= 0 ||
                     msgL.indexOf("éclaire") >= 0;

  bool veuxEteindre = msgL.indexOf("etein") >= 0 ||
                      msgL.indexOf("étein") >= 0 ||
                      msgL.indexOf("light off") >= 0 ||
                      msgL.indexOf("coupe") >= 0;

  // ── Zones — détection précise sans faux positifs ──
  bool zone1 = msgL.indexOf("zone 1") >= 0 ||
               msgL.indexOf("salon") >= 0;
  bool zone2 = msgL.indexOf("zone 2") >= 0 ||
               msgL.indexOf("chambre") >= 0;
  bool zone3 = msgL.indexOf("zone 3") >= 0 ||
               msgL.indexOf("cuisine") >= 0;
  bool toutesZones = msgL.indexOf("tout") >= 0 ||
                     msgL.indexOf("all") >= 0;
  bool aucuneZone = !zone1 && !zone2 && !zone3 && !toutesZones;

  // ── Demandes d'information ──
  bool demandeTemp = msgL.indexOf("temp") >= 0 ||
                     msgL.indexOf("chaud") >= 0 ||
                     msgL.indexOf("froid") >= 0 ||
                     msgL.indexOf("humid") >= 0 ||
                     msgL.indexOf("climat") >= 0;

  bool demandeAir = msgL.indexOf("air") >= 0 ||
                    msgL.indexOf("qualit") >= 0 ||
                    msgL.indexOf("poussiere") >= 0 ||
                    msgL.indexOf("poussière") >= 0 ||
                    msgL.indexOf("respir") >= 0;

  bool demandePresence = msgL.indexOf("presence") >= 0 ||
                         msgL.indexOf("présence") >= 0 ||
                         msgL.indexOf("quelqu") >= 0 ||
                         msgL.indexOf("personne") >= 0 ||
                         msgL.indexOf("mouvement") >= 0 ||
                         msgL.indexOf("maison") >= 0;

  bool demandeLumiere = msgL.indexOf("lumiere") >= 0 ||
                        msgL.indexOf("lumière") >= 0 ||
                        msgL.indexOf("luminosit") >= 0 ||
                        msgL.indexOf("sombre") >= 0 ||
                        msgL.indexOf("obscur") >= 0 ||
                        msgL.indexOf("jour") >= 0 ||
                        msgL.indexOf("nuit") >= 0;

  bool demandeVolt = msgL.indexOf("volt") >= 0 ||
                     msgL.indexOf("electr") >= 0 ||
                     msgL.indexOf("courant") >= 0 ||
                     msgL.indexOf("energie") >= 0 ||
                     msgL.indexOf("énergie") >= 0;


  bool demandeVision = msgL.indexOf("vision") >= 0 ||
                     msgL.indexOf("camera") >= 0 ||
                     msgL.indexOf("caméra") >= 0 ||
                     msgL.indexOf("gauche") >= 0 ||
                     msgL.indexOf("droite") >= 0 ||
                     msgL.indexOf("tourne") >= 0 ||
                     msgL.indexOf("regarde") >= 0 ||
                     msgL.indexOf("pivote") >= 0 ||
                     msgL.indexOf("vois") >= 0 ||
                     msgL.indexOf("servo") >= 0;                 

  // ════════════════════════════════════════
  // 💡 COMMANDES ÉCLAIRAGE
  // ════════════════════════════════════════

  if (veuxAllumer || veuxEteindre) {
    bool etat = veuxAllumer;
    String zones = "";

    // Variables locales avec noms différents !
    bool z1   = msgL.indexOf("zone 1") >= 0 ||
                msgL.indexOf("salon") >= 0;
    bool z2   = msgL.indexOf("zone 2") >= 0 ||
                msgL.indexOf("chambre") >= 0;
    bool z3   = msgL.indexOf("zone 3") >= 0 ||
                msgL.indexOf("cuisine") >= 0;
    bool tout = msgL.indexOf("tout") >= 0 ||
                msgL.indexOf("all zones") >= 0;

    Serial.println("DEBUG z1=" + String(z1) +
                   " z2=" + String(z2) +
                   " z3=" + String(z3) +
                   " tout=" + String(tout));

    // Aucune zone précisée
    if (!z1 && !z2 && !z3 && !tout) {
      Serial.println("DEBUG → Demande zone !");
      String q = "{\"type\":\"chat\",\"agent\":\"LUX\","
        "\"icon\":\"💡\",\"message\":\"I am LUX. "
        "Which zone shall I " +
        String(etat ? "illuminate" : "turn off") +
        "? Zone 1 (salon), Zone 2 (chambre), "
        "Zone 3 (cuisine), or '" +
        String(etat ? "allume tout" : "eteins tout") +
        "' for all!\"}";
      webSocket.broadcastTXT(q);
      return "LUX demande zone";
    }

    // Plein jour
    if (etat && dernierLDR > 500) {
      String a = "{\"type\":\"chat\",\"agent\":\"LUX\","
        "\"icon\":\"💡\",\"message\":\"⚠️ I am LUX. "
        "Warning: " + String(dernierLDR) +
        " lux — daytime! Wastes energy. Executing anyway...\"}";
      webSocket.broadcastTXT(a);
      delay(1200);
    }

    // Nuit
    if (etat && dernierLDR <= 500) {
      String i = "{\"type\":\"chat\",\"agent\":\"LUX\","
        "\"icon\":\"💡\",\"message\":\"I am LUX. Dark — " +
        String(dernierLDR) + " lux! Turning on lights!\"}";
      webSocket.broadcastTXT(i);
      delay(800);
    }

    // Exécution
    if (tout) {
      for (int i = 0; i < 3; i++) {
        etatsRelais[i] = etat;
        setRelais(i, etat);
        syncRelaisFirebase(i, etat);
      }
      webSocket.broadcastTXT(etat ? "relay1_on" : "relay1_off");
      webSocket.broadcastTXT(etat ? "relay2_on" : "relay2_off");
      webSocket.broadcastTXT(etat ? "relay3_on" : "relay3_off");
      zones = "all zones";
    } else {
      if (z1) {
        etatsRelais[0] = etat; setRelais(0, etat);
        syncRelaisFirebase(0, etat);
        webSocket.broadcastTXT(etat ? "relay1_on" : "relay1_off");
        zones += "Zone 1 ";
      }
      if (z2) {
        etatsRelais[1] = etat; setRelais(1, etat);
        syncRelaisFirebase(1, etat);
        webSocket.broadcastTXT(etat ? "relay2_on" : "relay2_off");
        zones += "Zone 2 ";
      }
      if (z3) {
        etatsRelais[2] = etat; setRelais(2, etat);
        syncRelaisFirebase(2, etat);
        webSocket.broadcastTXT(etat ? "relay3_on" : "relay3_off");
        zones += "Zone 3 ";
      }
    }

    String msgLux = "User asked to turn " +
                    String(etat ? "ON" : "OFF") +
                    " " + zones + ". Action done. LDR: " +
                    String(dernierLDR) + " lux. Confirm briefly.";
    String repLux = envoyerAGemini(PROMPT_LUX, msgLux);
    repLux.replace("\"", "'");
    repLux.replace("\n", " ");
    webSocket.broadcastTXT(
      "{\"type\":\"chat\",\"agent\":\"LUX\","
      "\"icon\":\"💡\",\"message\":\"" + repLux + "\"}");
    return repLux;
}

  // ════════════════════════════════════════
  // 🌡️ DEMANDE TEMPÉRATURE
  // ════════════════════════════════════════

  if (demandeTemp) {
    String contexte = "Current readings — Temperature: ";
    contexte += String(dernierTemp) + "C, Humidity: ";
    contexte += String(dernierHumid) + "%. ";
    contexte += "User asks: " + messageUser;
    contexte += ". Answer with real data and your personal analysis.";
    String rep = envoyerAGemini(PROMPT_AURA, contexte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"AURA\","
                  "\"icon\":\"🌡️\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    return rep;
  }

  // ════════════════════════════════════════
  // 🌬️ DEMANDE QUALITÉ AIR
  // ════════════════════════════════════════

  if (demandeAir) {
    String contexte = "Current air quality — Dust: ";
    contexte += String(dernierDust) + " mg/m3. ";
    contexte += "Temperature: " + String(dernierTemp) + "C. ";
    contexte += "Humidity: " + String(dernierHumid) + "%. ";
    contexte += "User asks: " + messageUser;
    String rep = envoyerAGemini(PROMPT_PURETE, contexte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"PURETÉ\","
                  "\"icon\":\"🌬️\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    return rep;
  }

  // ════════════════════════════════════════
  // 📡 DEMANDE PRÉSENCE
  // ════════════════════════════════════════

  if (demandePresence) {
    String contexte = "Current presence status — ";
    if (presenceDetectee) {
      contexte += "PRESENCE DETECTED! Movement: " + String(dernierMvt);
      contexte += ", Distance: " + String(dernierDist) + "cm. ";
    } else {
      contexte += "No presence detected. Zone is clear. ";
    }
    contexte += "User asks: " + messageUser;
    String rep = envoyerAGemini(PROMPT_RADAR, contexte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"RADAR\","
                  "\"icon\":\"📡\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    if (presenceDetectee) {
      webSocket.loop();
      String msgVision = "User asked about presence. RADAR confirms at ";
      msgVision += String(dernierDist) + "cm. Describe what you see.";
      String repV = envoyerAGemini(PROMPT_VISION, msgVision);
      repV.replace("\"", "'"); repV.replace("\n", " ");
      String chatV = "{\"type\":\"chat\",\"agent\":\"VISION\","
                     "\"icon\":\"👁️\",\"message\":\"" + repV + "\"}";
      webSocket.broadcastTXT(chatV);
    }
    return rep;
  }

  // ════════════════════════════════════════
  // 💡 DEMANDE LUMINOSITÉ
  // ════════════════════════════════════════

  if (demandeLumiere) {
    bool nuit = dernierLDR < 500;
    String contexte = "Current light level: " + String(dernierLDR) + " lux. ";
    contexte += nuit ? "It's dark inside. " : "Good natural light. ";
    contexte += "Relay states: Zone1=" + String(etatsRelais[0] ? "ON" : "OFF");
    contexte += " Zone2=" + String(etatsRelais[1] ? "ON" : "OFF");
    contexte += " Zone3=" + String(etatsRelais[2] ? "ON" : "OFF") + ". ";
    contexte += "User asks: " + messageUser;
    String rep = envoyerAGemini(PROMPT_LUX, contexte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"LUX\","
                  "\"icon\":\"💡\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    return rep;
  }

  // ════════════════════════════════════════
  // ⚡ DEMANDE ÉNERGIE
  // ════════════════════════════════════════

  if (demandeVolt) {
    String contexte = "Current voltage: " + String(dernierVolt) + "V. ";
    contexte += "Relay states: Zone1=" + String(etatsRelais[0] ? "ON" : "OFF");
    contexte += " Zone2=" + String(etatsRelais[1] ? "ON" : "OFF");
    contexte += " Zone3=" + String(etatsRelais[2] ? "ON" : "OFF") + ". ";
    contexte += "User asks: " + messageUser;
    String rep = envoyerAGemini(PROMPT_VOLT, contexte);
    rep.replace("\"", "'"); rep.replace("\n", " ");
    String chat = "{\"type\":\"chat\",\"agent\":\"VOLT\","
                  "\"icon\":\"⚡\",\"message\":\"" + rep + "\"}";
    webSocket.broadcastTXT(chat);
    return rep;
  }

  // ════════════════════════════════════════
// 👁️ DEMANDE VISION / SERVO
// ════════════════════════════════════════
  if (demandeVision) {
      // Détecte direction demandée
      bool allerGauche = msgL.indexOf("gauche") >= 0 ||
                         msgL.indexOf("left") >= 0;
      bool allerDroite = msgL.indexOf("droite") >= 0 ||
                         msgL.indexOf("right") >= 0;
      bool allerCentre = msgL.indexOf("centre") >= 0 ||
                         msgL.indexOf("center") >= 0;
  
      // Bouge le servo si direction précisée
      if (allerGauche) {
          posServo = constrain(posServo - 20, 10, 170);
          servoCam.write(posServo);
          webSocket.broadcastTXT(
              "{\"type\":\"servo_pos\",\"angle\":" +
              String(posServo) + "}");
      }
      if (allerDroite) {
          posServo = constrain(posServo + 20, 10, 170);
          servoCam.write(posServo);
          webSocket.broadcastTXT(
              "{\"type\":\"servo_pos\",\"angle\":" +
              String(posServo) + "}");
      }
      if (allerCentre) {
          posServo = 100;
          servoCam.write(posServo);
          webSocket.broadcastTXT(
              "{\"type\":\"servo_pos\",\"angle\":" +
              String(posServo) + "}");
      }
  
      String contexte = "Current position: " +
                        String(posServo) + " degrees. ";
      contexte += presenceDetectee ?
                  "Presence at " + String(dernierDist) + "cm. " :
                  "No presence. ";
      contexte += "User says: " + messageUser;
  
      String rep = envoyerAGemini(PROMPT_VISION, contexte);
      rep.replace("\"", "'");
      rep.replace("\n", " ");
      webSocket.broadcastTXT(
          "{\"type\":\"chat\",\"agent\":\"VISION\","
          "\"icon\":\"👁️\",\"message\":\"" + rep + "\"}");
      return rep;
  }

  // ════════════════════════════════════════
  // 🤖 QUESTION GÉNÉRALE → Coordinateur
  // ════════════════════════════════════════

  String promptCoord =
    "You are the coordinator of SentiHome S3 smart home. "
    "Current status: Temp=" + String(dernierTemp) + "C "
    "Humid=" + String(dernierHumid) + "% "
    "LDR=" + String(dernierLDR) + " lux "
    "Dust=" + String(dernierDust) + "mg/m3 "
    "Volt=" + String(dernierVolt) + "V "
    "Presence=" + String(presenceDetectee ? "YES" : "NO") + ". "
    "Which ONE agent should respond to: '" + messageUser + "'? "
    "Reply ONLY with agent name: AURA, LUX, RADAR, PURETE, VOLT, CONNECT, VISION";

  String agentChoisi = envoyerAGemini(promptCoord.c_str(), messageUser);
  agentChoisi.trim();
  Serial.println("🎯 Agent choisi: " + agentChoisi);

  const char* prompt = PROMPT_AURA;
  String agentNom = "AURA";
  String agentIcon = "🌡️";

  if (agentChoisi.indexOf("LUX") >= 0)
    { prompt=PROMPT_LUX; agentNom="LUX"; agentIcon="💡"; }
  else if (agentChoisi.indexOf("RADAR") >= 0)
    { prompt=PROMPT_RADAR; agentNom="RADAR"; agentIcon="📡"; }
  else if (agentChoisi.indexOf("PURE") >= 0)
    { prompt=PROMPT_PURETE; agentNom="PURETÉ"; agentIcon="🌬️"; }
  else if (agentChoisi.indexOf("VOLT") >= 0)
    { prompt=PROMPT_VOLT; agentNom="VOLT"; agentIcon="⚡"; }
  else if (agentChoisi.indexOf("CONN") >= 0)
    { prompt=PROMPT_CONNECT; agentNom="CONNECT"; agentIcon="📶"; }
  else if (agentChoisi.indexOf("VISI") >= 0)
    { prompt=PROMPT_VISION; agentNom="VISION"; agentIcon="👁️"; }

  String msgEnrichi = "Current home status: Temp=";
  msgEnrichi += String(dernierTemp) + "C Humid=";
  msgEnrichi += String(dernierHumid) + "% LDR=";
  msgEnrichi += String(dernierLDR) + " Dust=";
  msgEnrichi += String(dernierDust) + " Volt=";
  msgEnrichi += String(dernierVolt) + "V Presence=";
  msgEnrichi += String(presenceDetectee ? "YES" : "NO") + ". ";
  msgEnrichi += "User says: " + messageUser;

  String rep = envoyerAGemini(prompt, msgEnrichi);
  rep.replace("\"", "'"); rep.replace("\n", " ");
  String chat = "{\"type\":\"chat\",\"agent\":\"" + agentNom + "\","
                "\"icon\":\"" + agentIcon + "\",\"message\":\"" + rep + "\"}";
  webSocket.broadcastTXT(chat);
  return agentNom + ": " + rep;
}

// ================================================================
// FONCTION : Gestion des interrupteurs (priorité humaine)
// ================================================================
void gererInterrupteurs() {
  for (int i = 0; i < 3; i++) {
    bool lecture = digitalRead(INTERS[i]);

    if (lecture != derniersInters[i]) {
      delay(50); // Anti-rebond
      lecture = digitalRead(INTERS[i]); // Relit après debounce

      if (lecture == LOW) { // Bouton appuyé
        etatsRelais[i] = !etatsRelais[i];
        setRelais(i, etatsRelais[i]); // ← utilise setRelais !
        Serial.print("💡 Interrupteur ");
        Serial.print(i + 1);
        Serial.println(etatsRelais[i] ? " → ON" : " → OFF");
        // Notifie le dashboard
        String msg = "relay";
        msg += String(i + 1);
        msg += etatsRelais[i] ? "_on" : "_off";
        webSocket.broadcastTXT(msg);
      }
      derniersInters[i] = lecture;
    }
  }
}

void setRelais(int index, bool etat) {
  if (RELAIS_NC[index]) {
    // Logique inversée pour NC
    digitalWrite(RELAIS[index], etat ? HIGH : LOW);
  } else {
    // Logique normale pour NO
    digitalWrite(RELAIS[index], etat ? LOW : HIGH);
  }
  etatsRelais[index] = etat;
}

// ================================================================
// FIREBASE RTDB — URL de base
// ================================================================
#define FIREBASE_URL "https://gen-lang-client-0279809768-default-rtdb.firebaseio.com"

// ── Écriture données capteurs vers Firebase ──
void envoyerAFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  // ── Données capteurs ──
  String urlSensors = String(FIREBASE_URL) +
                      "/sentihome/sensors.json";
  String jsonSensors = "{";
  jsonSensors += "\"temp\":"     + String(dernierTemp)  + ",";
  jsonSensors += "\"humid\":"    + String(dernierHumid) + ",";
  jsonSensors += "\"ldr\":"      + String(dernierLDR)   + ",";
  jsonSensors += "\"dust\":"     + String(dernierDust)  + ",";
  jsonSensors += "\"volt\":"     + String(dernierVolt)  + ",";
  jsonSensors += "\"presence\":" +
                 String(presenceDetectee ? "true":"false") + ",";
  jsonSensors += "\"dist\":"     + String(dernierDist);
  jsonSensors += "}";

  http.begin(client, urlSensors);
  http.addHeader("Content-Type", "application/json");
  http.PUT(jsonSensors);
  http.end();

  // ── État relais ──
  String urlRelais = String(FIREBASE_URL) +
                     "/sentihome/relays.json";
  String jsonRelais = "{";
  jsonRelais += "\"relay1\":" +
                String(etatsRelais[0] ? "true":"false") + ",";
  jsonRelais += "\"relay2\":" +
                String(etatsRelais[1] ? "true":"false") + ",";
  jsonRelais += "\"relay3\":" +
                String(etatsRelais[2] ? "true":"false");
  jsonRelais += "}";

  http.begin(client, urlRelais);
  http.addHeader("Content-Type", "application/json");
  http.PUT(jsonRelais);
  http.end();

  Serial.println("🔥 Firebase mis à jour !");
}

// ================================================================
// API REST — Permet le Contrôle depuis Google Cloud
// ================================================================
void setupHTTPServer() {

    // ── Route POST : relais ──
    httpServer.on("/api/relay", HTTP_POST, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        if (!httpServer.hasArg("plain")) {
            httpServer.send(400, "application/json",
                "{\"error\":\"No body\"}");
            return;
        }
        String body = httpServer.arg("plain");
        JsonDocument doc;
        deserializeJson(doc, body);
        int zone  = doc["zone"]  | 0;
        bool etat = doc["state"] | false;
        if (zone >= 1 && zone <= 3) {
            etatsRelais[zone-1] = etat;
            setRelais(zone-1, etat);
            webSocket.broadcastTXT(
                etat ? "relay"+String(zone)+"_on" :
                       "relay"+String(zone)+"_off");
            httpServer.send(200, "application/json",
                "{\"success\":true,\"zone\":" + String(zone) +
                ",\"state\":" + String(etat?"true":"false") + "}");
        } else {
            httpServer.send(400, "application/json",
                "{\"error\":\"Invalid zone\"}");
        }
    });

    // ── Route GET : status ──
    httpServer.on("/api/status", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        String json = "{";
        json += "\"temp\":"     + String(dernierTemp)  + ",";
        json += "\"humid\":"    + String(dernierHumid) + ",";
        json += "\"ldr\":"      + String(dernierLDR)   + ",";
        json += "\"dust\":"     + String(dernierDust)  + ",";
        json += "\"volt\":"     + String(dernierVolt)  + ",";
        json += "\"presence\":" + String(presenceDetectee?"true":"false") + ",";
        json += "\"dist\":"     + String(dernierDist)  + ",";
        json += "\"relay1\":"   + String(etatsRelais[0]?"true":"false") + ",";
        json += "\"relay2\":"   + String(etatsRelais[1]?"true":"false") + ",";
        json += "\"relay3\":"   + String(etatsRelais[2]?"true":"false");
        json += "}";
        httpServer.send(200, "application/json", json);
        Serial.println("🌐 API: Status demandé");
    });

    // ── Route POST : all ──
    httpServer.on("/api/all", HTTP_POST, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        String body = httpServer.arg("plain");
        JsonDocument doc;
        deserializeJson(doc, body);
        bool etat = doc["state"] | false;
        for (int i = 0; i < 3; i++) {
            etatsRelais[i] = etat;
            setRelais(i, etat);
        }
        webSocket.broadcastTXT(etat ? "relay1_on" : "relay1_off");
        webSocket.broadcastTXT(etat ? "relay2_on" : "relay2_off");
        webSocket.broadcastTXT(etat ? "relay3_on" : "relay3_off");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"all\":" +
            String(etat?"true":"false") + "}");
    });

    // ── Routes GET contrôle ──
    httpServer.on("/api/on1", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[0] = true; setRelais(0, true);
        webSocket.broadcastTXT("relay1_on");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 1 ON\"}");
    });
    httpServer.on("/api/off1", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[0] = false; setRelais(0, false);
        webSocket.broadcastTXT("relay1_off");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 1 OFF\"}");
    });
    httpServer.on("/api/on2", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[1] = true; setRelais(1, true);
        webSocket.broadcastTXT("relay2_on");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 2 ON\"}");
    });
    httpServer.on("/api/off2", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[1] = false; setRelais(1, false);
        webSocket.broadcastTXT("relay2_off");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 2 OFF\"}");
    });
    httpServer.on("/api/on3", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[2] = true; setRelais(2, true);
        webSocket.broadcastTXT("relay3_on");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 3 ON\"}");
    });
    httpServer.on("/api/off3", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        etatsRelais[2] = false; setRelais(2, false);
        webSocket.broadcastTXT("relay3_off");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"Zone 3 OFF\"}");
    });
    httpServer.on("/api/allon", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        for(int i=0;i<3;i++){etatsRelais[i]=true;setRelais(i,true);}
        webSocket.broadcastTXT("relay1_on");
        webSocket.broadcastTXT("relay2_on");
        webSocket.broadcastTXT("relay3_on");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"All ON\"}");
    });
    httpServer.on("/api/alloff", HTTP_GET, []() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods", "GET");
        for(int i=0;i<3;i++){etatsRelais[i]=false;setRelais(i,false);}
        webSocket.broadcastTXT("relay1_off");
        webSocket.broadcastTXT("relay2_off");
        webSocket.broadcastTXT("relay3_off");
        httpServer.send(200, "application/json",
            "{\"success\":true,\"action\":\"All OFF\"}");
    });

    // ── CORS global ──
    httpServer.onNotFound([]() {
        httpServer.sendHeader("Access-Control-Allow-Origin", "*");
        httpServer.sendHeader("Access-Control-Allow-Methods",
                              "GET,POST,OPTIONS");
        httpServer.send(404, "application/json",
                        "{\"error\":\"Not found\"}");
    });

    httpServer.begin();
    Serial.println("✅ API REST démarrée port 80 !");
}

// ── Lecture commandes depuis Firebase ──
void lireCommandesFirebase() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);

  HTTPClient http;
  http.setReuse(false); // ← NOUVEAU

  // ← URL avec anti-cache
  String url = String(FIREBASE_URL) +
               "/sentihome/commands.json?t=" +
               String(millis()); // ← NOUVEAU

  http.begin(client, url);
  http.addHeader("Cache-Control", "no-cache"); // ← NOUVEAU
  http.addHeader("Pragma", "no-cache");         // ← NOUVEAU

  int code = http.GET();

  if (code == 200) {
    String raw = http.getString();
    Serial.println("🔥 Firebase raw: " + raw);

    if (raw == "null" || raw.length() < 5) {
      http.end();
      return;
    }

    JsonDocument doc;
    deserializeJson(doc, raw);

    auto lireBool = [&](const char* key) -> bool {
      if (!doc.containsKey(key)) return false;
      if (doc[key].is<bool>())   return doc[key].as<bool>();
      if (doc[key].is<int>())    return doc[key].as<int>() == 1;
      if (doc[key].is<String>()) return doc[key].as<String>() == "true";
      return false;
    };

    if (doc.containsKey("relay1")) {
      bool etat = lireBool("relay1");
      Serial.println("🔥 relay1 lu: " +
                     String(etat ? "true" : "false"));
      if (etat != etatsRelais[0]) {
        etatsRelais[0] = etat;
        setRelais(0, etat);
        webSocket.broadcastTXT(
          etat ? "relay1_on" : "relay1_off");
        Serial.println("🔥 Firebase → Relay1 " +
          String(etat ? "ON" : "OFF"));
      }
    }
    if (doc.containsKey("relay2")) {
      bool etat = lireBool("relay2");
      Serial.println("🔥 relay2 lu: " +
                     String(etat ? "true" : "false"));
      if (etat != etatsRelais[1]) {
        etatsRelais[1] = etat;
        setRelais(1, etat);
        webSocket.broadcastTXT(
          etat ? "relay2_on" : "relay2_off");
        Serial.println("🔥 Firebase → Relay2 " +
          String(etat ? "ON" : "OFF"));
      }
    }
    if (doc.containsKey("relay3")) {
      bool etat = lireBool("relay3");
      Serial.println("🔥 relay3 lu: " +
                     String(etat ? "true" : "false"));
      if (etat != etatsRelais[2]) {
        etatsRelais[2] = etat;
        setRelais(2, etat);
        webSocket.broadcastTXT(
          etat ? "relay3_on" : "relay3_off");
        Serial.println("🔥 Firebase → Relay3 " +
          String(etat ? "ON" : "OFF"));
      }
    }
  } else {
    Serial.println("❌ Firebase GET erreur: " + String(code));
  }
  http.end();
}

void syncRelaisFirebase(int index, bool etat) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String(FIREBASE_URL) +
               "/sentihome/commands/relay" +
               String(index + 1) + ".json";
  String data = etat ? "true" : "false";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.PUT(data);
  http.end();
  Serial.println("🔥 Sync Firebase relay" +
                 String(index+1) + " → " +
                 String(etat ? "ON" : "OFF"));
}

// ================================================================
// Début de la Fonction SETUP
// ================================================================
void setup() {
 
  // Maintenant on active la sortie — part déjà de HIGH !
  pinMode(14, OUTPUT);
  pinMode(8,  OUTPUT);
  pinMode(21, OUTPUT);

  //⭐ ORDRE IMPORTANT — digitalWrite AVANT pinMode !
  // Sur ESP32, cet ordre évite le glitch LOW au démarrage
  setRelais(0, false); // Zone 1 OFF
  setRelais(1, false); // Zone 2 OFF
  setRelais(2, false); // Zone 3 OFF

  // Interrupteurs
  for (int i = 0; i < 3; i++) {
    pinMode(INTERS[i], INPUT_PULLUP);
  }

  Serial.begin(115200);

  unsigned long t = millis();
  while (!Serial && (millis() - t) < 3000) delay(10);
  delay(500);
  
  Serial.println("🚀 SENTIHOME S3 + GEMINI API — Démarrage...");

  dht.begin();
  delay(2000);// Nous laissons L'ESP se stabiliser
  Serial1.begin(256000, SERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX);
  delay(500);
  Serial2.begin(115200, SERIAL_8N1, PIN_A7_RX,   PIN_A7_TX);

  servoCam.attach(PIN_SERVO);
  servoCam.write(90);
  delay(500);
  pinMode(PIN_POUSSIERE_LED, OUTPUT);
  digitalWrite(PIN_POUSSIERE_LED, HIGH);

  // Connexion WiFi
  Serial.println("📶 Connexion WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 20) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connecté ! IP: " + WiFi.localIP().toString());
    Serial.println("🤖 Gemini API  : " + String(GEMINI_URL));
    Serial.println("🔗 WebSocket   : ws://192.168.100.6:82");
    Serial.println("📷 CAM Stream  : http://" + String(CAM_IP) + ":81/stream");
  } else {
    Serial.println("\n⚠️ WiFi échoué — Mode GSM actif");
  }

  // Démarrage WebSocket
  webSocket.begin();
  setupHTTPServer();
  webSocket.onEvent(webSocketEvent);
  Serial.println("✅ WebSocket démarré port 82 !");
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String url = String(FIREBASE_URL) +
                 "/sentihome/commands.json";
    String json = "{\"relay1\":false,"
                  "\"relay2\":false,"
                  "\"relay3\":false}";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.PUT(json);
    http.end();
    Serial.println("🔥 Firebase initialisé !");
  }
    Serial.println("🔥 SENTIHOME S3 OPÉRATIONNEL !");
    Serial.println("================================");
}

// ================================================================
// Début de la Fonction LOOP
// ================================================================
void loop() {
  //  WebSocket en continu
  webSocket.loop();
  httpServer.handleClient();

  //  Priorité humaine — Interrupteurs
  gererInterrupteurs();

  if (millis() - dernierFirebase > INTERVALLE_FIREBASE) {
    envoyerAFirebase();      // écrit vers Firebase
    lireCommandesFirebase(); // lit commandes
    dernierFirebase = millis();
  }

  //  Capteurs rapides sans Gemini !
  if (millis() - dernierCapteurs > INTERVALLE_CAPTEURS) {
    float temp  = dht.readTemperature();
    float humid = dht.readHumidity();
    if (!isnan(temp) && !isnan(humid)) {
      dernierTemp  = temp;
      dernierHumid = humid;
      webSocket.broadcastTXT(
        "{\"sensor\":\"AURA\",\"temp\":" + String(temp) +
        ",\"humid\":" + String(humid) + "}");
    }
    int ldr = analogRead(PIN_LDR);
    dernierLDR = ldr;
    webSocket.broadcastTXT(
      "{\"sensor\":\"LUX\",\"ldr\":" + String(ldr) + "}");

    int valBrut = analogRead(PIN_VOLT);
    float volt = (valBrut * 3.3 / 4095.0) * 2.0;
    dernierVolt = volt;
    webSocket.broadcastTXT(
      "{\"sensor\":\"VOLT\",\"voltage\":" + String(volt) + "}");

    dernierCapteurs = millis();
  }

  //  Radar — temps réel
  if (millis() - dernierRadar > INTERVALLE_RADAR) {
    lireRADAR();
    dernierRadar = millis();
  }

  //  Bilan toutes les 60 secondes
  if (millis() - dernierBilan > INTERVALLE_BILAN) {
    Serial.println("\n=== 📊 BILAN AGENTS ===");
    lireAURA();
    webSocket.loop(); // ← traite les commandes en attente
    lireLUX();
    webSocket.loop();
    lirePURETE();
    webSocket.loop();
    lireVOLT();
    webSocket.loop();
    Serial.println("=== ✅ BILAN TERMINÉ ===\n");
    dernierBilan = millis();
}
}

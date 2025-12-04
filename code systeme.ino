#include <Adafruit_MLX90614.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
// Initialize Telegram BOT
#define BOTtoken "7740736002:AAGjyhe4TNzBx30A9-Fv5fQlBH9TwPBlMy8"  // your Bot Token (Get from Botfather)
// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "7988402856"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// GPIO 21 SDA and GPIO22 SCL
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
HardwareSerial SerialPort(0);
// WiFi info
const char* ssid = "Airbox-C9E6";
const char* password = "FFG5JESy";

// Capteurs
MAX30105 particleSensor;
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Web server
WebServer server(80);

// Données patient
String nom = "NOM_PATIENT";
int age = 24;
String coordonnees = "Monastir, Tunisie";
int bp1=0;
int i=0;
// Mesures
float bpm = 0;
float spo2 = 0;
float temperature = 0;
unsigned long lastBeat = 0;
unsigned long lastUpdate = 0;

String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'>
<title>Patient Monitor</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv='refresh' content='5'>
<style>
  body {
    font-family: Arial;
    background: #f0f8ff;
    color: #333;
    text-align: center;
    transition: all 0.3s ease;
  }
  @media (prefers-color-scheme: dark) {
    body { background: #121212; color: #eee; }
    .card { background: #1e1e1e; }
  }
  .card {
    background: white;
    border-radius: 15px;
    box-shadow: 0 4px 10px rgba(0,0,0,0.2);
    padding: 20px;
    max-width: 500px;
    margin: 50px auto;
    transition: background 0.3s;
  }
  img { width: 100px; }
  h1 { color: #007acc; }
  .data { font-size: 1.2em; margin: 10px 0; }
  button {
    background-color: #007acc;
    color: white;
    border: none;
    padding: 10px 15px;
    border-radius: 10px;
    cursor: pointer;
    margin-top: 10px;
  }
</style>
</head><body>
<div class='card' id="card">
  <h1>Surveillance Patient</h1>
  <div class='data' style="color:black;">👤 <strong style=\"color:black;\">Nom:</strong> )rawliteral" + nom + R"rawliteral(</div>
  <div class='data' style="color:black;">🎂 <strong style=\"color:black;\">Âge:</strong> )rawliteral" + String(age) + R"rawliteral(</div>
  <div class='data' style="color:black;">💓 <strong>BPM:</strong> <span id="bpm">)rawliteral" + String(bpm, 1) + R"rawliteral(</span></div>
  <div class='data' style="color:black;">🩸 <strong>SpO₂:</strong> <span id="spo2">)rawliteral" + String(spo2, 1) + R"rawliteral(</span> %</div>
  <div class='data' style="color:black;">🌡️ <strong>Température:</strong> <span id="temp">)rawliteral" + String(temperature, 1) + R"rawliteral(</span> °C</div>
  <div class='data' style="color:black;">📍 <strong>Coordonnées:</strong> )rawliteral" + coordonnees + R"rawliteral(</div>
  <div class='data' style='font-size:0.9em;color:black;'>📅 Dernière mise à jour: )rawliteral" + String(millis() / 1000) + R"rawliteral( s</div>
  <button onclick="exportPDF()">📄 Télécharger PDF</button>
  <canvas id="chart" width="400" height="200"></canvas>
</div>
</body></html>
)rawliteral";
  return html;}

void setup() {
    // Initialisation de l'UART : connexion serie avec le pc 
  Serial.begin(115200);  
    while (!Serial) {
    delay(10);}
  // Connexion WiFi
  Serial.println("Connexion au WiFi...");
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;}
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nImpossible de se connecter au WiFi !");
    return;}
  Serial.println("\nConnecté !");
  Serial.print("IP locale : ");
  Serial.println(WiFi.localIP());
  // Capteur MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 non détecté !");
    while (1);
  }
  // Capteur MLX90614
  Serial.println("Adafruit MLX90614 test");
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };

  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  // Temp capteur
  sensors.begin();

  // Web route
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });
  server.begin();
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // LED ON
}


void loop() {
  // telegrambot settings
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  //webserver handling
  server.handleClient();
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // Calcul BPM
  if (irValue > 15000) {
        i=0;
      int irValue1 =irValue;
    lastBeat = millis();
    delay(20);
    while(i<100){
    irValue = particleSensor.getIR();
     if (irValue > 15000 && irValue<irValue1){
          long delta = millis() - lastBeat;
          float beatsPerMinute = 60 / (delta / 1000.0);
              if (beatsPerMinute > 20 && beatsPerMinute < 220) {
      bpm = beatsPerMinute;}
          break;}
     delay(10);
     i++;
    }}
       bp1 =bpm;


  // SpO2 estimation (simplifiée)
  if (redValue > 10000 && irValue > 10000) {
    spo2 = 100.0 - abs(redValue - irValue) * 0.001;
  }
  // Patient Température Reading
  temperature = mlx.readObjectTempC() ;
  // PRINTING to serial
  Serial.print("Beats per minute = "); Serial.println(bpm);
  Serial.print("oxygen level = "); Serial.println(spo2);
  Serial.print("ambient temperature = "); Serial.println(mlx.readAmbientTempC());
  Serial.print("patient temperature = "); Serial.println(temperature);
  Serial.println();
  delay(500);
  lastUpdate = millis();
  delay(1000);
}




void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/BPM to request heart rate reading \n";
      welcome += "/ATEMP to request ambient temperature reading \n";
      welcome += "/PTEMP to request object temperature reading \n";
      welcome += "/SPO to request oxygen level reading \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    String menu = String("\n") +
              "For your next request use:\n" +
              "/BPM to request heart rate reading\n" +
              "/ATEMP to request ambient temperature reading\n" +
              "/PTEMP to request object temperature reading\n" +
              "/SPO to request oxygen level reading\n";
    if (text == "/BPM") {
      menu = String(bpm, 2) + menu;
      bot.sendMessage(chat_id,menu, "");
       // Send the color to the LED
    }
    
    if (text == "/ATEMP") {
      menu = String(mlx.readAmbientTempC()) + menu;
      bot.sendMessage(chat_id,menu, "");
       // Send the color to the LED
    }
    
    if (text == "/PTEMP") {
      menu = String(temperature,2) + menu;
      bot.sendMessage(chat_id, menu, "");
      
    }
    if (text == "/SPO") {
      menu = String(spo2,2) + menu;
      bot.sendMessage(chat_id, menu, "");
      
    }
  }
}

/*
 * IoT Environmeter using BME680 and SSD1306 OLED, sending the data to Ambient.
 *  + Smart remote controller for air conditioner using Slack
 * Copyright (C) 2024 K.Takata
 */
/* Based on the basic_config_state.ino sketch from Bosch-BSEC2-Library:
 * https://github.com/BoschSensortec/Bosch-BSEC2-Library
 * Copyright (C) 2021 Bosch Sensortec GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

// If defined, send the data to Ambient.
#define USE_AMBIENT

// If defined, use the IR remote controller feature via Slack.
#define USE_REMOTE_CTRL

// If defined, temperature will be adjusted by subtracting this value.
//#define TEMPERATURE_OFFSET  2.0f


// Use the Espressif EEPROM library. Skip otherwise.
#if defined(ARDUINO_ARCH_ESP32) || (ARDUINO_ARCH_ESP8266)
#include <EEPROM.h>
#define USE_EEPROM
#endif

#if defined(USE_AMBIENT) || defined(USE_REMOTE_CTRL)
#define USE_WIFI
#endif

#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <bsec2.h>

#ifdef USE_WIFI
#include <esp_sntp.h>
#endif
#ifdef USE_AMBIENT
#include <Ambient.h>
#endif
#ifdef USE_REMOTE_CTRL
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <IRac.h>

// https://github.com/esp8266/Arduino/blob/master/tools/cert.py
// $ cert.py -s slack.com > certs.h
#include "certs.h"
#endif

#include "AnonymousPro8pt7b.h"
#include "AnonymousPro16pt7b.h"

struct ParsedOutput {
  double temp, humi, pres, gasr, co2e, bvoc, iaq, di, comptemp, comphumi;
  int co2eacc, bvocacc, iaqacc, stabstat, runinstat;
};

const uint8_t bsec_config[] = {
#include "config/bme680/bme680_iaq_18v_3s_4d/bsec_iaq.txt"
};

// Macros used
#define STATE_SAVE_PERIOD   UINT32_C(6 * 3600 * 1000) // 6 hours (4 times a day)
#define MONITOR_PIN 7
#define ERROR_DUR   1000

#define SDA_PIN 4
#define SCL_PIN 5
#define INTERRUPT_PIN 9

#define IR_SEND_PIN 6

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

int disp_mode = 0;
constexpr int small_font_height = 8;
constexpr int small_font_width = 6;
constexpr int baseline_Anonymous_Pro8pt = 11;
constexpr int baseline_Anonymous_Pro16pt = 22;


#ifdef USE_WIFI
#include "private_settings.h"   // ssid, passwd, channelId, writeKey

bool clock_started = false;
#endif

#ifdef USE_AMBIENT
Ambient ambient;
WiFiClient ambiclient;
time_t lastsent = 0;
constexpr int send_cycle = 600;   // 10 min
constexpr int initial_wait = 180; // 3 min
#endif

#ifdef USE_REMOTE_CTRL
time_t lastread = 0;
constexpr int message_threshold = 180;  // 3 min
constexpr int read_cycle = 60;    // 1 min
float cur_temp = 0.0f;
float last_temp;  // Last temperature sent to A/C.
#endif

#define LOG_INFOF(...)  do { if (Serial.availableForWrite()) { Serial.printf(__VA_ARGS__); } } while (0)
#define LOG_INFO(...)   do { if (Serial.availableForWrite()) { Serial.print(__VA_ARGS__); } } while (0)
#define LOG_INFOLN(...) do { if (Serial.availableForWrite()) { Serial.println(__VA_ARGS__); } } while (0)

// Helper functions declarations
void errLeds();
void checkBsecStatus(Bsec2 bsec);
void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

// Create an object of the class Bsec2
Bsec2 envSensor;
#ifdef USE_EEPROM
static uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE];
#endif

#ifdef USE_WIFI
// Setup Wi-Fi
void beginWiFi()
{
  WiFi.setTxPower(WIFI_POWER_2dBm);
  WiFi.begin(ssid, passwd);
}

bool checkWiFi()
{
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  LOG_INFOLN("WiFi connected");
  LOG_INFOF("IP address: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

// Setup RTC with NTP servers
void beginClock()
{
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}

bool checkClock()
{
  if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    return false;
  }
  LOG_INFOLN("Clock sync'ed.");
  time_t now = time(nullptr);

  struct tm timeinfo;
  //gmtime_r(&now, &timeinfo);
  //LOG_INFOF("Current time (UTC): %s", asctime(&timeinfo));
  localtime_r(&now, &timeinfo);
  LOG_INFOF("Current time (JST): %s", asctime(&timeinfo));
  return true;
}

void beginWifiApp()
{
  static bool wifi_connected = false;

  if (!wifi_connected) {
    if (checkWiFi()) {
      beginClock();
#ifdef USE_AMBIENT
      ambient.begin(channelId, writeKey, &ambiclient);
#endif
      wifi_connected = true;
    }
  }
  else if (!clock_started) {
    if (checkClock()) {
#ifdef USE_AMBIENT
      lastsent = time(nullptr) - send_cycle + initial_wait;
#endif
      clock_started = true;
    }
  }
}
#else   // USE_WIFI
void beginWiFi()
{
  WiFi.disconnect(true);
}
#define beginWifiApp()
#endif  // USE_WIFI

// Entry point
void setup()
{
  // Desired subscription list of BSEC2 outputs
  bsecSensor sensorList[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  // Initialize the communication interfaces
  beginWiFi();
#ifdef USE_EEPROM
  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1);
#endif
  Serial.begin();
  Wire.begin(SDA_PIN, SCL_PIN);
  pinMode(MONITOR_PIN, OUTPUT);
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  digitalWrite(MONITOR_PIN, LOW);

  // Display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    errLeds();
  }

  //display.clearDisplay();
  //display.display();
  //display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  //display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Initial display with all zero
  ParsedOutput res = {0.0};
  updateDisplay(res);

  // Initialize the library and interfaces
  if (!envSensor.begin(BME68X_I2C_ADDR_HIGH, Wire)) {
    checkBsecStatus(envSensor);
  }

  // Load the configuration string
  if (!envSensor.setConfig(bsec_config)) {
    checkBsecStatus(envSensor);
  }

  // Copy state from the EEPROM to the algorithm
  if (!loadState(envSensor)) {
    checkBsecStatus(envSensor);
  }

  // Subscribe to the desired BSEC2 outputs
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP)) {
    checkBsecStatus(envSensor);
  }
#ifdef TEMPERATURE_OFFSET
  envSensor.setTemperatureOffset(TEMPERATURE_OFFSET);
#endif

  // Whenever new data is available call the newDataCallback function
  envSensor.attachCallback(newDataCallback);

  LOG_INFOLN("BSEC library version "
          + String(envSensor.version.major) + "."
          + String(envSensor.version.minor) + "."
          + String(envSensor.version.major_bugfix) + "."
          + String(envSensor.version.minor_bugfix));

  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonPushed, FALLING);
}

unsigned long pushed_time = 0;
bool button_pushed = false;

void buttonPushed()
{
  if (!button_pushed) {
    pushed_time = millis();
    button_pushed = true;
  }
}

constexpr int debounceDelay = 50; // [ms]

void checkButtonStatus()
{
  if (button_pushed) {
    if (millis() - pushed_time >= debounceDelay) {
      if (digitalRead(INTERRUPT_PIN) == LOW) {
        //Serial.println("button pushed");
        disp_mode = (disp_mode + 1) % 3;
        // Update OLED
        ParsedOutput res = parseOutputs(envSensor.getOutputs());
        updateDisplay(res);
      }
      button_pushed = false;
    }
  }
}


#ifdef USE_REMOTE_CTRL
String urlencode(String s)
{
  const char *t = "0123456789ABCDEF";
  String e = "";
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (isalnum(c) /* || c == '*' || c == '-' || c == '.' || c == '_' */) {
      e += c;
    }
    else if (c == ' ') {
      e += '+';
    }
    else {
      e += '%';
      e += t[(c >> 4) & 0xf];
      e += t[c & 0xf];
    }
  }
  return e;
}

// Post a message to the slack channel.
void post_message(String text)
{
  WiFiClientSecure client;
  HTTPClient https;

  client.setCACert(cert_ISRG_Root_X1);

  if (https.begin(client, slack_host, slack_port, "/api/chat.postMessage")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&text=" + urlencode(text);
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      LOG_INFOF("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        LOG_INFOLN(payload);
      }
    }
    else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
}

// Send Daikin A/C command.
void send_ac(bool power,
    stdAc::opmode_t mode=stdAc::opmode_t::kAuto,
    float temp=25.0,
    stdAc::fanspeed_t fan=stdAc::fanspeed_t::kAuto)
{
  IRDaikinESP irsend(IR_SEND_PIN);
  uint8_t dmode = IRDaikinESP::convertMode(mode);
  uint8_t dfan = IRDaikinESP::convertFan(fan);

  irsend.begin();
  irsend.setPower(power);
  irsend.setMode(dmode);
  irsend.setFan(dfan);   // 1..5, kDaikinFanAuto, kDaikinFanQuiet

  if (dmode == kDaikinDry) {
    temp = 96.0;  // Special temperature for dry.
  }
  else {
    temp = std::max(temp, static_cast<float>(kDaikinMinTemp));
    temp = std::min(temp, static_cast<float>(kDaikinMaxTemp));
  }

#if 0
  irsend.setTemp(temp);
  last_temp = irsend.getTemp();
#else
  uint8_t *raw = irsend.getRaw();
  raw[22] = static_cast<uint8_t>(temp * 2);
  last_temp = raw[22] / 2.0f;
  irsend.setRaw(raw);
#endif

  //irsend.enableOnTimer(0);
  //irsend.enableOffTimer(0);

  irsend.send();
}

String get_temp_str(float temp)
{
  char buf[20];
  return dtostrf(temp, 3, 1, buf);
}

// Get the last temperature sent to A/C.
String get_last_temp_str()
{
  return get_temp_str(last_temp);
}

// Parse a command to this bot.
void parse_command(String command)
{
  command.toLowerCase();
  int space = command.indexOf(' ');
  float degree = 0.0f;
  if (space >= 0) {
    String param = command.substring(space + 1);
    degree = param.toFloat();
  }
  else if ('0' <= command[0] && command[0] <= '9') {
    // Only the temp is specified. Decide the mode by ourself. (Semi-auto mode)
    degree = command.toFloat();
    if ((degree > cur_temp - 3.0f) && (degree <= 21.0f)) {
      command = "h"; // Heater
    }
    else if ((degree < cur_temp + 3.0f) && (degree >= 24.0f)) {
      command = "c"; // Cooler
    }
    else {
      post_message("Couldn't decide the mode automatically. Please specify the mode explicitly.");
      return;
    }
  }

  if (command.startsWith("hello")) {
    // alive check
    post_message("Hello!");
  }
  else if (command.startsWith("help")) {
    post_message(
      "Cooler on: c[ooler] <temp>\n"
      "Heater on: h[eater] <temp>\n"
      "Dry on: d[ry]\n"
      "Fan on: f[an]\n"
      "Auto on: a[uto]\n"
      "Semi-auto on: <temp>\n"
      "A/C off: off\n"
      "Show current temperature: t[emp]\n"
      "Alive check: hello\n"
      "Show this help: help\n"
      );
  }
  else if (command[0] == 'c' && degree != 0.0f) {
    send_ac(true, stdAc::opmode_t::kCool, degree);
    post_message("Cooler On: " + get_last_temp_str() + " ℃\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command[0] == 'h' && degree != 0.0f) {
    send_ac(true, stdAc::opmode_t::kHeat, degree);
    post_message("Heater On: " + get_last_temp_str() + " ℃\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command[0] == 'd') {
    send_ac(true, stdAc::opmode_t::kDry);
    post_message("Dry On\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command[0] == 'f') {
    send_ac(true, stdAc::opmode_t::kFan);
    post_message("Fan On\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command[0] == 'a') {
    send_ac(true);
    post_message("Auto On\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command.startsWith("off")) {
    send_ac(false);
    post_message("A/C Off\n"
                 "Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else if (command[0] == 't') {
    post_message("Current: " + get_temp_str(cur_temp) + " ℃");
  }
  else {
    post_message("Unknown command: '" + command + "'");
  }
}

// Parse a slack response.
void parse_response(String payload)
{
  DynamicJsonDocument doc(2048);

  if (deserializeJson(doc, payload)) {
    Serial.println("JSON parse error.");
    return;
  }
  if (!doc["ok"].as<bool>()) {
    Serial.println("Slack error.");
    return;
  }
  String type = doc["messages"][0]["type"].as<const char*>();
  if (type != "message") {
    LOG_INFOLN("Not a message.");
    return;
  }
  String text = doc["messages"][0]["text"].as<const char*>();
  String ts = doc["messages"][0]["ts"].as<const char*>();
  time_t timestamp = static_cast<time_t>(ts.toInt());

  String to_bot = String("<@") + slack_bot_id + ">";
  if (!text.startsWith(to_bot)) {
    LOG_INFOLN("Not a message to the bot.");
    return;
  }
  // Found a message to the bot.
  String body = text.substring(to_bot.length());
  if (time(nullptr) - timestamp >= message_threshold) {
    LOG_INFOLN("No new messages.");
    return;
  }
  body.trim();
  parse_command(body);
}

// Get the latest message from the slack channel.
String get_slack_message()
{
  WiFiClientSecure client;
  HTTPClient https;
  String payload;

  client.setCACert(cert_ISRG_Root_X1);

  if (https.begin(client, slack_host, slack_port, "/api/conversations.history")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&limit=1";
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      LOG_INFOF("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = https.getString();
      }
    }
    else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return payload;
}
#endif  // USE_REMOTE_CTRL


// Function that is looped forever
void loop()
{
  beginWifiApp();

  checkButtonStatus();

  // Call the run function often so that the library can
  // check if it is time to read new data from the sensor
  // and process it.
  if (!envSensor.run()) {
    checkBsecStatus(envSensor);
  }

#ifdef USE_REMOTE_CTRL
  if (clock_started) {
    time_t now = time(nullptr);
    if (now - lastread >= read_cycle) {
      String payload = get_slack_message();
      if (!payload.isEmpty()) {
        LOG_INFOLN(payload);
        parse_response(payload);
      }
      lastread = now;
    }
  }
#endif
  delay(10);
}

void errLeds()
{
  while (1) {
    digitalWrite(MONITOR_PIN, HIGH);
    delay(ERROR_DUR);
    digitalWrite(MONITOR_PIN, LOW);
    delay(ERROR_DUR);
  }
}

ParsedOutput parseOutputs(const bsecOutputs* outputs)
{
  ParsedOutput res = {0.0};
  if (outputs == nullptr) {
    return res;
  }
  for (uint8_t i = 0; i < outputs->nOutputs; i++) {
    const bsecData& output = outputs->output[i];
    switch (output.sensor_id) {
      case BSEC_OUTPUT_RAW_TEMPERATURE:
        res.temp = output.signal;
        break;
      case BSEC_OUTPUT_RAW_HUMIDITY:
        res.humi = output.signal;
        break;
      case BSEC_OUTPUT_RAW_PRESSURE:
        res.pres = output.signal;
        break;
      case BSEC_OUTPUT_RAW_GAS:
        res.gasr = output.signal / 1000.0;
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        res.co2e = output.signal;
        res.co2eacc = (int)output.accuracy;
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        res.bvoc = output.signal;
        res.bvocacc = (int)output.accuracy;
        break;
      case BSEC_OUTPUT_IAQ:
        res.iaq = output.signal;
        res.iaqacc = (int)output.accuracy;
        break;
      case BSEC_OUTPUT_STABILIZATION_STATUS:
        res.stabstat = (int)output.signal;
        break;
      case BSEC_OUTPUT_RUN_IN_STATUS:
        res.runinstat = (int)output.signal;
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        res.comptemp = output.signal;
#ifdef USE_REMOTE_CTRL
        cur_temp = res.comptemp;
#endif
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        res.comphumi = output.signal;
        break;
      default:
        break;
    }
  }
  res.di = 0.81 * res.temp + 0.01 * res.humi * (0.99 * res.temp - 14.3) + 46.3;
  return res;
}

String tostr(double f, bool simple=false)
{
  char buf[20];
  if (simple) {
    dtostrf(f, 5, 1, buf);
  }
  else {
    dtostrf(f, 7, 1, buf);
  }
  return String(buf);
}

String diffstr(double f)
{
  return ((f >= 0.0) ? "+" : "") + String(f);
}

void updateDisplay(const ParsedOutput& res)
{
  display.clearDisplay();
  switch (disp_mode) {
    case 1:
      // Detail mode
      display.setFont(nullptr);
      display.setCursor(0, 0);
      display.println("Temp: " + String(res.comptemp) + " \370C");  // U+00B0 (Degree Sign)
      display.setCursor(small_font_width * 16, 0);
      display.println(diffstr(res.comptemp - res.temp));
      display.setCursor(0, small_font_height * 1);
      display.println("Humi: " + String(res.comphumi) + " %");
      display.setCursor(small_font_width * 16, small_font_height * 1);
      display.println(diffstr(res.comphumi - res.humi));
      display.setCursor(0, small_font_height * 2);
      display.println("Pres: " + String(res.pres) + " hPa");
      display.println("DI:   " + String(res.di));
      display.println("GasR: " + String(res.gasr) + " k\352");  // Ohm
      display.println("CO2e: " + String(res.co2e) + " ppm");
      display.println("bVOC: " + String(res.bvoc) + " ppm");
      display.println("IAQ:  " + String(res.iaq) + " (acc=" + String(res.iaqacc) + ")");
      break;
    case 0:
    default:
      // Normal mode
      display.setFont(&Anonymous_Pro8pt7b);
      display.setCursor(0, baseline_Anonymous_Pro8pt);   // Set baseline
      display.println(tostr(res.comptemp) + "\177C");  // U+00B0 => 0x7F (Degree Sign)
      display.println(tostr(res.comphumi) + " %");
      display.println(tostr(res.pres) + " hPa");
      display.println(tostr(res.co2e) + " ppm");
      break;
    case 2:
      // Simple mode
      display.setFont(&Anonymous_Pro16pt7b);
      display.setCursor(0, baseline_Anonymous_Pro16pt + 4);   // Set baseline
      display.println(tostr(res.comptemp, true) + "\177C");  // U+00B0 => 0x7F (Degree Sign)
      display.println(tostr(res.comphumi, true) + " %");
      break;
  }
  display.display();
}

void updateBsecState(Bsec2 bsec)
{
  static unsigned long lastsave = 0;

  if (lastsave == 0) {
    lastsave = millis();
  }
  if (millis() - lastsave > STATE_SAVE_PERIOD) {
    lastsave = millis();
    if (!saveState(bsec)) {
      checkBsecStatus(bsec);
    }
  }
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  if (!outputs.nOutputs) {
    return;
  }

  ParsedOutput res = parseOutputs(&outputs);
  updateDisplay(res); // Display to OLED

  if (Serial.availableForWrite()) {
    Serial.println("BSEC outputs:");
    Serial.println("\ttimestamp = "
        + String((int)(outputs.output[0].time_stamp / INT64_C(1000000))));

    Serial.println("\ttemperature = " + String(res.temp) + " ℃");
    Serial.println("\tcompensated temperature = " + String(res.comptemp) + " ℃ (diff = " + diffstr(res.comptemp - res.temp) + ")");
    Serial.println("\thumidity = " + String(res.humi) + " %");
    Serial.println("\tcompensated humidity = " + String(res.comphumi) + " % (diff = " + diffstr(res.comphumi - res.humi) + ")");
    Serial.println("\tpressure = " + String(res.pres) + " hPa");
    Serial.println("\tDI = " + String(res.di));
    Serial.println("\tgas resistance = " + String(res.gasr) + " kΩ");
    Serial.println("\tco2 equivalent = " + String(res.co2e) + " ppm (accuracy = " + String(res.co2eacc) + ")");
    Serial.println("\tbreath voc equivalent = " + String(res.bvoc) + " ppm (accuracy = " + String(res.bvocacc) + ")");
    Serial.println("\tiaq = " + String(res.iaq) + " (accuracy = " + String(res.iaqacc) + ")");
    Serial.println("\tstabilization status = " + String(res.stabstat));
    Serial.println("\trun in status = " + String(res.runinstat));
  }

#ifdef USE_AMBIENT
  // Send to Ambient
  if (clock_started) {
    time_t now = time(nullptr);
    if (now - lastsent >= send_cycle) {
      ambient.set(1, res.temp);
      ambient.set(2, res.humi);
      ambient.set(3, res.pres);
      ambient.set(4, res.di);
      ambient.set(5, res.gasr);
      ambient.set(6, res.co2e);
      ambient.set(7, res.iaq);
      ambient.set(8, res.iaqacc);
      if (!ambient.send()) {
        Serial.println("Failed to send to Ambient.");
      }
      lastsent = now;
    }
  }
#endif

  // Flash the LED
  digitalWrite(MONITOR_PIN, HIGH);
  delay(10);
  digitalWrite(MONITOR_PIN, LOW);

  updateBsecState(envSensor);
}

void checkBsecStatus(Bsec2 bsec)
{
  if (bsec.status < BSEC_OK) {
    Serial.println("BSEC error code : " + String(bsec.status));
    errLeds();
  }
  else if (bsec.status > BSEC_OK) {
    Serial.println("BSEC warning code : " + String(bsec.status));
  }

  if (bsec.sensor.status < BME68X_OK) {
    Serial.println("BME68X error code : " + String(bsec.sensor.status));
    errLeds();
  }
  else if (bsec.sensor.status > BME68X_OK) {
    Serial.println("BME68X warning code : " + String(bsec.sensor.status));
  }
}

bool loadState(Bsec2 bsec)
{
#ifdef USE_EEPROM
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in EEPROM
    LOG_INFOLN("Reading state from EEPROM");
    LOG_INFO("State file: ");
    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      LOG_INFO(String(bsecState[i], HEX) + ", ");
    }
    LOG_INFOLN();

    if (!bsec.setState(bsecState)) {
      return false;
    }
  }
  else {
    // Erase the EEPROM with zeroes
    LOG_INFOLN("Erasing EEPROM");
    for (uint8_t i = 0; i <= BSEC_MAX_STATE_BLOB_SIZE; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
  }
#endif
  return true;
}

bool saveState(Bsec2 bsec)
{
#ifdef USE_EEPROM
  if (!bsec.getState(bsecState)) {
    return false;
  }

  LOG_INFOLN("Writing state to EEPROM");
  LOG_INFO("State file: ");

  for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
    EEPROM.write(i + 1, bsecState[i]);
    LOG_INFO(String(bsecState[i], HEX) + ", ");
  }
  LOG_INFOLN();

  EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
  EEPROM.commit();
#endif
  return true;
}

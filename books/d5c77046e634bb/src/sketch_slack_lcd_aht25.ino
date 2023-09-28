#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <IRac.h>
#include <Wire.h>
#include <time.h>

// https://github.com/esp8266/Arduino/blob/master/tools/cert.py
// $ cert.py -s slack.com > certs.h
#include "certs.h"

X509List cert(cert_ISRG_Root_X1);

#define PIN_SEND 12

#define SSID    "**************"
#define PASSWD  "*************"

const char* slack_host = "slack.com";
const uint16_t slack_port = 443;

const char* slack_token = "xoxb-*************-*************-************************";
const char* slack_channel_id = "C**********";
const char* slack_bot_id = "U**********";

#define MESSAGE_THRESHOLD  180    // 3 min


class Lcd {
private:
  const int addr = 0x3E;

public:
  Lcd() = default;
  ~Lcd() = default;

  void init() {
    delay(10);
    send_cmd(0x38);
    delay(2);
    send_seq({0x39, 0x14});

    uint8_t contrast = 0x20;
    send_seq({
      uint8_t(0x70 + (contrast & 0x0F)),
      uint8_t(0x5C + ((contrast >> 4) & 0x03)),
      0x6C});
    delay(200);

    send_seq({0x38, 0x0C, 0x01});
    delay(2);
  }

  template <size_t cmdlen>
  void send_seq(const uint8_t (&cmds)[cmdlen], const uint8_t *data=nullptr, size_t datalen=0) {
    send_seq(cmds, cmdlen, data, datalen);
  }

  void send_seq(const uint8_t *cmds, size_t cmdlen, const uint8_t *data=nullptr, size_t datalen=0) {
    Wire.beginTransmission(addr);
    if (data == nullptr) {
      // Only command data
      Wire.write(0x00);       // Command byte: Co=0, RS=0
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(cmds[i]);  // Command data byte
      } 
    } else {
      // Send command words (if any)
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire.write(0x80);     // Command byte: Co=1, RS=0
        Wire.write(cmds[i]);  // Command data byte
      }
      // Send RAM data bytes
      Wire.write(0x40);       // Command byte: Co=0, RS=1
      for (size_t i = 0; i < datalen; ++i) {
        Wire.write(data[i]);  // RAM data byte
      }
    }
    Wire.endTransmission();
  }

  void send_cmd(const uint8_t cmd) {
    send_seq({cmd});
  }

  template <size_t datalen>
  void send_data(const uint8_t (&data)[datalen]) {
    send_data(data, datalen);
  }

  void send_data(const uint8_t *data, size_t datalen) {
    send_seq(nullptr, 0, data, datalen);
  }

  void set_cursor(int col, int row) {
    send_cmd(uint8_t(0x80 + 0x40*row + col));
  }

  void clear() {
    send_cmd(0x01);
    delay(2);
  }

  void print(String s) {
    send_data(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
  }

  template <size_t patlen>
  void set_cgram(int num, const uint8_t (&pat)[patlen]) {
    set_cgram(num, pat, patlen);
  }

  void set_cgram(int num, const uint8_t *pat, size_t patlen) {
    send_seq({uint8_t(0x40 + num*8)}, pat, patlen);
  }
};

class Aht25 {
private:
  const int addr = 0x38;
  uint8_t data[7];

public:
  Aht25() = default;
  ~Aht25() = default;

  void init() {
    Wire.beginTransmission(addr);
    Wire.write(0x71);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, 1);
    if (Wire.read() != 0x18) {
      // TODO: initialize
      Serial.println("FIXME: Need initialization");
    }
  }

  void trigger() {
    Wire.beginTransmission(addr);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    Wire.endTransmission();
  }

  void read_data() {
    Wire.requestFrom(addr, sizeof(data));
    for (size_t i = 0; i < sizeof(data); ++i) {
      data[i] = Wire.read();
    }
    // TODO: Check CRC
  }

  // Calculate temperature
  //   return: 100x degrees Celsius
  int calc_temp() {
    int temp_raw = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    return ((temp_raw * 625) >> 15) - 5000;
  }

  // Calculate humidity
  //   return: 1000x %rH
  int calc_hum() {
    int hum_raw = (data[1] << 12) | (data[2] << 4) | ((data[3] & 0x0F) >> 4);
    return ((hum_raw * 625) >> 15) * 5;
  }
};

String adj_digit(int num, int digit) {
  String s(num);
  return s.substring(0, s.length() - digit) + '.' + s.substring(s.length() - digit);
}

String align_right(String s, int digit) {
  char buf[32];
  sprintf(buf, "%*s", digit, s.c_str());
  return buf;
}


// Set the clock using NTP.
void setClock() {
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  Serial.print("Waiting for NTP time sync: ");
  time_t now;
  while ((now = time(nullptr)) < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.printf("Current time (JST): %s", asctime(&timeinfo));
}

Lcd lcd;
Aht25 aht25;

void setup() {
  Serial.begin(115200);

  Wire.begin();
  if (ESP.getResetInfoPtr()->reason != REASON_DEEP_SLEEP_AWAKE) {
    // Don't clear LCD on wake on from deep sleep.
    lcd.init();
  }
  aht25.init();

  WiFi.setOutputPower(0.0);   // Set to minimum power. (0.0 - 20.5 dBm)
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWD);

  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setClock();

  // Degree symbol
  lcd.set_cgram(1, {0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

String urlencode(String s) {
  const char *t = "0123456789ABCDEF";
  String e = "";
  for (unsigned int i = 0; i < s.length(); ++i) {
    char c = s[i];
    if (isalnum(c) /* || c == '*' || c == '-' || c == '.' || c == '_' */) {
      e += c;
    } else if (c == ' ') {
      e += '+';
    } else {
      e += '%';
      e += t[(c >> 4) & 0xf];
      e += t[c & 0xf];
    }
  }
  return e;
}

// Post a message to the slack channel.
void post_message(String text) {
  WiFiClientSecure client;
  HTTPClient https;

  client.setTrustAnchors(&cert);

  if (https.begin(client, slack_host, slack_port, "/api/chat.postMessage")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&text=" + urlencode(text);
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
}

float last_temp;  // Last sent temperature.

// Send Daikin A/C command.
void send_ac(bool power,
    stdAc::opmode_t mode=stdAc::opmode_t::kAuto,
    float temp=25.0,
    stdAc::fanspeed_t fan=stdAc::fanspeed_t::kAuto) {
  IRDaikinESP irsend(PIN_SEND);
  uint8_t dmode = IRDaikinESP::convertMode(mode);
  uint8_t dfan = IRDaikinESP::convertFan(fan);

  irsend.begin();
  irsend.setPower(power);
  irsend.setMode(dmode);
  irsend.setFan(dfan);   // 1..5, kDaikinFanAuto, kDaikinFanQuiet

  if (dmode == kDaikinDry) {
    temp = 96.0;  // Special temperature for dry.
  } else {
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

// Get the last temperature sent to A/C.
String get_last_temp() {
  char buf[20];
  return dtostrf(last_temp, 3, 1, buf);
}

// Parse a command to this bot.
void parse_command(String command) {
  command.toLowerCase();
  String param;
  int space = command.indexOf(' ');
  float degree = 25.0;
  if (space >= 0) {
    param = command.substring(space + 1);
    degree = param.toFloat();
  }

  char buf[20] = "";
  if (command.startsWith("hello")) {
    // alive check
    post_message("Hello!");
  } else if (command.startsWith("help")) {
    post_message(
      "Cooler on: c[ooler] <temp>\n"
      "Heater on: h[eater] <temp>\n"
      "Dry on: d[ry]\n"
      "Fan on: f[an]\n"
      "Auto on: a[uto]\n"
      "A/C off: off\n"
      "Alive check: hello\n"
      "Show this help: help\n"
      );
  } else if (command[0] == 'c' && !param.isEmpty()) {
    send_ac(true, stdAc::opmode_t::kCool, degree);
    post_message("Cooler On: " + get_last_temp() + " ℃");
    sprintf(buf, "%-16s", ("Cooler " + get_last_temp() + "\x01" "C").c_str());
  } else if (command[0] == 'h' && !param.isEmpty()) {
    send_ac(true, stdAc::opmode_t::kHeat, degree);
    post_message("Heater On: " + get_last_temp() + " ℃");
    sprintf(buf, "%-16s", ("Heater " + get_last_temp() + "\x01" "C").c_str());
  } else if (command[0] == 'd') {
    send_ac(true, stdAc::opmode_t::kDry);
    post_message("Dry On");
    sprintf(buf, "%-16s", "Dry");
  } else if (command[0] == 'f') {
    send_ac(true, stdAc::opmode_t::kFan);
    post_message("Fan On");
    sprintf(buf, "%-16s", "Fan");
  } else if (command[0] == 'a') {
    send_ac(true);
    post_message("Auto On");
    sprintf(buf, "%-16s", "Auto");
  } else if (command.startsWith("off")) {
    send_ac(false);
    post_message("A/C Off");
    sprintf(buf, "%-16s", "A/C Off");
  } else {
    post_message(String("Unknown command: '") + command + "'");
  }
  if (buf[0] != '\0') {
    lcd.set_cursor(0, 1);
    lcd.print(buf);
  }
}

// Parse a slack response.
void parse_response(String payload) {
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
    Serial.println("Not a message.");
    return;
  }
  String text = doc["messages"][0]["text"].as<const char*>();
  String ts = doc["messages"][0]["ts"].as<const char*>();
  time_t timestamp = static_cast<time_t>(ts.toInt());

  String to_bot = String("<@") + slack_bot_id + "> ";
  if (!text.startsWith(to_bot)) {
    Serial.println("Not a message to the bot.");
    return;
  }
  // Found a message to the bot.
  String body = text.substring(to_bot.length());
  if (time(nullptr) - timestamp >= MESSAGE_THRESHOLD) {
    Serial.println("No new messages.");
    return;
  }
  parse_command(body);
}

// Get the latest message from the slack channel.
String get_slack_message() {
  WiFiClientSecure client;
  HTTPClient https;
  String payload;

  client.setTrustAnchors(&cert);

  if (https.begin(client, slack_host, slack_port, "/api/conversations.history")) {  // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = String("token=") + slack_token + "&channel=" + slack_channel_id + "&limit=1";
    int httpCode = https.POST(body);

    if (httpCode > 0) {
      Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = https.getString();
      }
    } else {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return payload;
}

void loop() {
  aht25.trigger();
  delay(80);
  aht25.read_data();

  String s;
  int temp = aht25.calc_temp();
  s = adj_digit(temp, 2);
  Serial.printf("temp %s C\n", s.c_str());
  lcd.set_cursor(0, 0);
  lcd.print(align_right(s.substring(0, s.length() - 1), 5) + "\x01" "C");

  int hum = aht25.calc_hum();
  s = adj_digit(hum, 3);
  Serial.printf("hum %s %%\n", s.c_str());
  //lcd.set_cursor(7, 0);
  lcd.print(" " + align_right(s.substring(0, s.length() - 2), 5) + " %");

  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    String payload = get_slack_message();
    if (!payload.isEmpty()) {
      Serial.println(payload);
      parse_response(payload);
    }
  }

  Serial.println("Wait 1min before next round...");
#if 0
  delay(60*1000);
#else
  ESP.deepSleep(60*1000*1000);
  delay(1000);
#endif
}

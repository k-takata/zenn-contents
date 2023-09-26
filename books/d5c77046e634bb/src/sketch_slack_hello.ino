#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// https://github.com/esp8266/Arduino/blob/master/tools/cert.py
// $ cert.py -s slack.com > certs.h
#include "certs.h"

X509List cert(cert_ISRG_Root_X1);

#define SSID    "**************"
#define PASSWD  "*************"

const char* slack_host = "slack.com";
const uint16_t slack_port = 443;

const char* slack_token = "xoxb-*************-*************-************************";
const char* slack_channel_id = "C**********";


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

void setup() {
  Serial.begin(115200);

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

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    post_message("Hello World");
  }

  Serial.println("Wait 1min before next round...");
  delay(60*1000);
}

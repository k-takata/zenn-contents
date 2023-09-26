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
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    String payload = get_slack_message();
    if (!payload.isEmpty()) {
      Serial.println(payload);
    }
  }

  Serial.println("Wait 1min before next round...");
  delay(60*1000);
}

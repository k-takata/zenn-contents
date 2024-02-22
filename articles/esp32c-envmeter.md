---
title: "ESP32-C3とBME680でIoT環境メーターを作る"
emoji: "🌡"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32c3", "bme680"]
published: false
---

以前、[GR-CITRUSとBME680で作ったIoT環境メーター](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/05_i2c_bme680)を基に、ESP32-C3を使ったIoT環境メーターを作ってみました。

GR-CITRUSではBosch純正のBSECライブラリが使用できませんでしたが、BSECが対応しているESP32-C3を使うことで、測定項目が大幅に増えました。


## 仕様

* 測定項目
  - 温度 (生の値及びヒーター補正値)
  - 湿度 (生の値及びヒーター補正値)
  - 気圧
  - 不快指数
  - ガス抵抗値
  - 推定CO2換算値 (CO2 equivalent)
  - 推定IAQ (Indoor Air Quality, 室内空気質)
  - IAQ精度
  - 推定呼気VOC (Volatile Organic Compounds, 揮発性有機化合物)
* [0.96インチOLED](https://akizukidenshi.com/catalog/g/g112031/)で情報表示
  - 通常表示と詳細表示の切り替え
* 測定結果を[Ambient](https://ambient.io/)に送信
  - 1チャンネル当たりのデータは8個までのため、一部測定項目は送信項目から除外


## ハードウェア

[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)で回路図と基板を公開しています。


## Arduino IDE

今回はArduino IDE 2.2.1を使って開発を行いました。

ESP32シリーズ用のライブラリを設定する必要があります。

[Arduino core for the ESP32](https://github.com/espressif/arduino-esp32)

https://espressif.github.io/arduino-esp32/package_esp32_index.json



## BSEC

[Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library)

[Bosch-BME68x-Library](https://github.com/BoschSensortec/Bosch-BME68x-Library)


なお、[BSEC-Arduino-library](https://github.com/boschsensortec/BSEC-Arduino-library)にはESP32-C3用のビルド済みライブラリが含まれていないので使えません。




## OLED

[Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)ライブラリを使ってOLED表示を行います。

[Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)

Adafruit_BusIO



### カスタムフォント使用方法



## Ambient

[Ambient_ESP8266_lib](https://github.com/AmbientDataInc/Ambient_ESP8266_lib)


https://github.com/AmbientDataInc/Ambient_ESP8266_lib/pull/5

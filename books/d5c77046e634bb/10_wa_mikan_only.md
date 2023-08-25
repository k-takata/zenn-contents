---
title: "WA-MIKANを単体で使う"
---

ここでは、WA-MIKANを単体で使う方法を紹介します。

WA-MIKANにはESP8266というWi-Fiモジュールが搭載されていますが、このモジュール自体がマイクロコントローラーを内蔵しており、単体でプログラムを動かすこともできます。[Arduino IDE](https://www.arduino.cc/en/software)を使えば、簡単にESP8266用のプログラムを作成し、WA-MIKANで動かすことができます。

(あとで書く)

[Arduino IDEでWA-MIKAN(和みかん)のESP8266をプログラミングする　環境インストール編 - Qiita](https://qiita.com/tarosay/items/28ba9e0208f41cec492d)
[SparkFun Serial Basic Breakout - CH340C and USB-C - DEV-15096 - SparkFun Electronics](https://www.sparkfun.com/products/15096)
[SparkFun DEV-15096 SparkFun Serial Basic Breakout - CH340C and USB-C](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5EFU)

1. ファームウェアの戻し方
   [WA-MIKAN(和みかん)をGR-CITRUSから使える状態に戻す方法 - Qiita](https://qiita.com/tarosay/items/618104528d4e5026fa8f)
   [Flash Download Tools](https://www.espressif.com/en/support/download/other-tools?keys=&field_type_tid%5B%5D=14)
   [ESP8266 NONOS SDK](https://github.com/espressif/ESP8266_NONOS_SDK/releases)
   [AT Command Set](https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/index.html)
2. Arduino IDEを使ってみる
3. IRremoteESP8266を使って赤外線リモコンの送受信を行う
4. Wi-Fiを使ってみる (予定)

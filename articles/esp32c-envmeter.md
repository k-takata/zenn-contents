---
title: "ESP32-C3とBME680でIoT環境メーターを作る"
emoji: "🌡"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680"]
published: false
---

以前、[GR-CITRUS/WA-MIKANとBME680で作ったIoT環境メーター](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/06_wa_mikan_wifi)を基に、[ESP32-C3](https://akizukidenshi.com/catalog/g/g117493/)を使ったIoT環境メーターを作ってみました。

GR-CITRUSではBosch純正の[BSEC](https://www.bosch-sensortec.com/software-tools/software/bme680-software-bsec/)ライブラリが使用できませんでしたが、今回BSECが対応しているESP32-C3を使うことで、測定項目が大幅に増えました。


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
  - 通常表示・詳細表示・簡易表示の切り替え
* 測定結果を[Ambient](https://ambient.io/)に送信
  - 1チャンネル当たりのデータは8項目までのため、一部測定項目は送信項目から除外


## ハードウェア

[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)で回路図と基板を公開しています。


## Arduino IDE

今回はArduino IDE 2.2.1(および 2.3.1, 2.3.2)を使って開発を行いました。

ESP32-C3を使うには、ESP32シリーズ用のボードマネージャを設定する必要があります。
[ESP32 Arduino Coreのドキュメント](https://docs.espressif.com/projects/arduino-esp32/en/latest/)に従って、以下のURLをArduino IDEの追加ボードマネージャに設定します。

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

メニューの「ツール」→「ボード」→「ボードマネージャ」を選び、"esp32" で検索します。"esp32 by Espressif Systems" が見つかりますので、インストールします。



## BSEC

[AE-BME680](https://akizukidenshi.com/catalog/g/g114469/)の全機能を活用するには
[BSEC](https://www.bosch-sensortec.com/software-tools/software/bme680-software-bsec/)ライブラリが必要ですが、このライブラリを使うには、通常、ユーザー登録が必要です。しかし、BSECはArduino用のライブラリとしても公開されており、それを使えばユーザー登録不要で簡単にBSECの機能を使うことができます。(もちろん、BSECライセンスに同意する必要はありますが。)

* [Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library)
* [Bosch-BME68x-Library](https://github.com/boschsensortec/Bosch-BME68x-Library)

Windowsの場合、`C:\Users\<ユーザー名>\Documents\Arduino\libraries` にライブラリーをインストールします。(`libraries` ディレクトリがない時は作成すればよいです。)
Bosch-BSEC2-Libraryのインストールガイドには、zipファイルをダウンロードしてArduino IDEにインポートするように書かれていますが、`libraries` ディレクトリでgit cloneする方が楽かもしれません。

```
$ git clone https://github.com/boschsensortec/Bosch-BSEC2-Library.git
$ git clone https://github.com/boschsensortec/Bosch-BME68x-Library.git
```

なお、Arduino IDEのライブラリマネージャー上でBSECで検索すると、[BSEC-Arduino-library](https://github.com/boschsensortec/BSEC-Arduino-library)が見つかりますが、これにはESP32-C3用のビルド済みライブラリが含まれていないので今回は使えません。



## OLED

[Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)ライブラリを使ってOLED表示を行います。
Arduino IDEのライブラリマネージャー上で "Adafruit SSD1306" で検索し、インストールします。以下の2つのライブラリも依存ライブラリとして表示されますので、併せてインストールします。

* [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)



### カスタムフォント使用方法

Adafruit-GFX-Libraryのデフォルトで使われるフォントは、6x8ドットのフォントです。大きな文字を表示したい場合は、`setTextSize()`関数で倍率を指定することができますが、単純に元のフォントを拡大表示するだけなので、きれいな表示にはなりません。そのため、大きな文字を表示するにはカスタムフォントを使うのがよいです。

Adafruit-GFX-Libraryの[fontconvert](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert)ディレクトリ内にはTTFフォントをこのライブラリで使える形式に変換するプログラムが入っています。

変換プログラムを使うのはUbuntuを使うのが楽です。Windowsであれば、WSLを使うのがよいでしょう。
まずは必要なパッケージをインストールします。

```
$ sudo apt install build-essential libfreetype6-dev
```

変換プログラムをビルドします。

```
$ cd fontconvert
$ make
```

[Anonymous Pro](https://www.marksimonson.com/fonts/view/anonymous-pro)を変換してみましょう。サイズは8ポイントにしました。

```
$ ./fontconvert 'Anonymouse Pro.ttf' 8 > AnonymousPro8pt7b.h
```

このファイルをインクルードして、`setFont()`関数で設定するとこのフォントが使われるようになります。
これで使えるようになる文字はU+0020からU+007Eのみです。

"℃" の丸の部分(°(U+00B0))を表示できるように、グリフを1つ追加します。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 126 126 > AnonymousPro8pt7b_0x7e.h
```

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 176 176 > AnonymousPro8pt7b_0xb0.h
```

3,5,6,8,9,m の字形が気に入らなかったので字形を調整することにしました。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 51 51 > AnonymousPro8pt7b_0x33.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 53 53 > AnonymousPro8pt7b_0x35.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 54 54 > AnonymousPro8pt7b_0x36.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 56 56 > AnonymousPro8pt7b_0x38.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 57 57 > AnonymousPro8pt7b_0x39.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 109 109 > AnonymousPro8pt7b_0x6d.h
```

簡易表示用に16ポイントのフォントも用意します。

```
$ ./fontconvert 'Anonymouse Pro.ttf' 16 > AnonymousPro16pt7b.h
```

```
$ ./fontconvert 'Anonymous Pro.ttf' 16 126 126 > AnonymousPro16pt7b_0x7e.h
```

丸のサイズを小さめにするために、12ポイントのグリフを流用します。
```
$ ./fontconvert 'Anonymous Pro.ttf' 12 176 176 > AnonymousPro12pt7b_0xb0.h
```


## Ambient

Ambientにデータを送信するには、Arduino向けの純正ライブラリである[Ambient_ESP8266_lib](https://github.com/AmbientDataInc/Ambient_ESP8266_lib)を使います。

本来は、Arduino IDEのライブラリマネージャーで "Ambient_ESP8266_lib" を検索してインストールすればいいのですが、Arduino IDE 2.2.1でコンパイルしたところ以下のようなエラーが出てしまいました。

```
error: variable 'inChar' set but not used [-Werror=unused-but-set-variable]
```

そこで以下のPRを作成しましたが、2024年2月末時点ではまだマージされていません。

<https://github.com/AmbientDataInc/Ambient_ESP8266_lib/pull/5>

そのため、今回は以下のようにしてインストールします。
Arduinoのライブラリディレクトリ (Windowsの場合、`C:\Users\<ユーザー名>\Documents\Arduino\libraries`) に行き、以下のコマンドを実行します。

```
$ git clone https://github.com/AmbientDataInc/Ambient_ESP8266_lib.git
$ cd Ambient_ESP8266_lib
$ git remote add k-takata https://github.com/k-takata/Ambient_ESP8266_lib.git
$ git fetch k-takata
$ git switch fix-compilation-errors
```

これで、上記のPRが適用されたコードが使用できます。

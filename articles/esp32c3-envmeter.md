---
title: "ESP32-C3とBME680でIoT環境メーターを作る"
emoji: "🌡"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680"]
published: true
---

## 概要

以前、作成した[GR-CITRUS + WA-MIKAN + BME680によるIoT環境メーター](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/06_wa_mikan_wifi)を基に、[ESP32-C3](https://akizukidenshi.com/catalog/g/g117493/)を使ったIoT環境メーターを作ってみました。

GR-CITRUSではBosch純正の[BSEC](https://www.bosch-sensortec.com/software-tools/software/bme680-software-bsec/)ライブラリが使用できませんでしたが、今回BSECが対応しているESP32-C3を使うことで、測定項目が大幅に増えました。


## 仕様

* 測定項目
  - 温度 (ヒーター補正値及び生の値との差分)
  - 湿度 (ヒーター補正値及び生の値との差分)
  - 気圧
  - 不快指数
  - ガス抵抗値
  - 推定CO₂換算値 (CO₂ equivalent)
  - 推定IAQ (Indoor Air Quality, 室内空気質)
  - IAQ精度
  - 推定呼気VOC (Volatile Organic Compounds, 揮発性有機化合物)
* [0.96インチOLED](https://akizukidenshi.com/catalog/g/g112031/)で情報表示
  - Modeボタンを押すたびに、通常表示・詳細表示・簡易表示の切り替え
* 測定結果を[Ambient](https://ambient.io/)に送信
  - 1チャンネル当たりのデータは8項目までのため、一部測定項目は送信項目から除外

:::message alert
BME680で測定しているCO₂換算値はあくまで推定値であり、CO₂を直接計測しているわけではありません。2021年に電気通信大学から「[安価で粗悪なCO₂センサの見分け方 ～５千円以下の機種、大半が消毒用アルコールに強く反応～](https://www.uec.ac.jp/about/publicity/news_release/2021/pdf/20210810_2.pdf)」というプレスリリースが出されていますが、指摘の内容はBME680にも当てはまると考えてよいでしょう。
:::


## ハードウェア

[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)で回路図と基板を公開しています。

ESP32-C3-WROOM-02-N4のシンボルやフットプリントは、Espressif公式の[Espressif KiCad Library](https://github.com/espressif/kicad-libraries)を使用しました。公式なだけあって、しっかり3Dモデルも含まれており、完成予想図の確認に便利です。


## Arduino IDE

今回はArduino IDE 2.2.1(および 2.3.1, 2.3.2)を使って開発を行いました。

ESP32-C3を使うには、ESP32シリーズ用のボードマネージャを設定する必要があります。
[ESP32 Arduino Coreのドキュメント](https://docs.espressif.com/projects/arduino-esp32/en/latest/)の[インストール方法のページ](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)に従って、以下のURLをArduino IDEの追加ボードマネージャに設定します。

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

メニューの「ツール」→「ボード」→「ボードマネージャ」を選び、"esp32" で検索します。"esp32 by Espressif Systems" が見つかりますので、インストールします。

インストールが終わったら、「ツール」→「ボード」→「esp32」→「ESP32C3 Dev Module」を選択しておきます。


### 書き込み設定

メニューの「ツール」→「USB CDC On Boot」→「Enabled」を選んでおけば、`Serial.println()`の出力はESP32-C3内蔵のUSBシリアルから出力されるようになります。

:::message
Arduino IDEのバージョンによっては設定がすぐに反映されず、Arduino IDEの再起動が必要なことがありました。
:::

それ以外の設定はデフォルトのままで問題ありません。

新品未使用のESP32-C3を接続した場合、USBの接続と切断が繰り返される場合があります。その場合は、Modeボタンを押しながらResetボタンを押し、Resetボタンを離してからModeボタンを離すと書き込みモードに入ることができます。

~~あるいは、"[Configure ESP32-C3 built-in JTAG Interface - ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32c3/api-guides/jtag-debugging/configure-builtin-jtag.html)" にあるように、PowerShellから以下のコマンドでJTAGドライバーをインストールしてもよいようです。~~
(この方法は上手くいきませんでした。)

```PowerShell
Invoke-WebRequest 'https://dl.espressif.com/dl/idf-env/idf-env.exe' -OutFile .\idf-env.exe; .\idf-env.exe driver install --espressif
```

一度書き込みを行えば、通常は書き込みモードに入らずともそのまま内蔵のUSBシリアル経由で書き込みができるようになります。ただし、Deep sleepモードやLight sleepモードを使った場合は、内蔵USBシリアルが使えなくなりますので、書き込みモードでの起動が必要になります。

:::message
書き込みモードを使って書き込みを行った場合、書き込みが完了しても自動リセットは掛かりません。プログラムを実行するには、Resetボタンを押して手動でリセットする必要があります。
:::


## BSEC

[AE-BME680](https://akizukidenshi.com/catalog/g/g114469/)の全機能を活用するには[BSEC](https://www.bosch-sensortec.com/software-tools/software/bme680-software-bsec/)ライブラリが必要ですが、このライブラリを使うには、通常、ユーザー登録が必要です。しかし、BSECはArduino用のライブラリとしても公開されており、それを使えばユーザー登録不要で簡単にBSECの機能を使うことができます。(もちろん、BSECライセンスに同意する必要はあります。)

* [Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library)
* [Bosch-BME68x-Library](https://github.com/boschsensortec/Bosch-BME68x-Library)

Windowsの場合、`C:\Users\<ユーザー名>\Documents\Arduino\libraries` にライブラリーをインストールします。(`libraries` ディレクトリがない時は作成すればよいです。)
Bosch-BSEC2-Libraryのインストールガイドには、zipファイルをダウンロードしてArduino IDEにインポートするように書かれていますが、`libraries` ディレクトリでgit cloneする方が楽かもしれません。

```
$ git clone https://github.com/boschsensortec/Bosch-BSEC2-Library.git
$ git clone https://github.com/boschsensortec/Bosch-BME68x-Library.git
```

:::message
Arduino IDEのライブラリマネージャー上でBSECで検索すると、[BSEC-Arduino-library](https://github.com/boschsensortec/BSEC-Arduino-library)が見つかりますが、これにはESP32-C3用のビルド済みライブラリが含まれていないので今回は使えません。
:::


### 基本動作確認

サンプルの[basic.ino](https://github.com/boschsensortec/Bosch-BSEC2-Library/blob/master/examples/generic_examples/basic/basic.ino)を使えば、一通りBME680の機能を試すことができます。
basic.inoを使う際は以下の2箇所を変更する必要があります。

まず、LEDのピン番号を9に変更します。

変更前:
```C
#define PANIC_LED   LED_BUILTIN
```
変更後:
```C
#define PANIC_LED   9
```

次に、I²Cに使うピン番号として、SDAに4、SCLに5を指定します。

変更前:
```C
    /* Initialize the communication interfaces */
    Serial.begin(115200);
    Wire.begin();
    pinMode(PANIC_LED, OUTPUT);
```
変更後:
```C
    /* Initialize the communication interfaces */
    Serial.begin(115200);
    Wire.begin(4, 5);
    pinMode(PANIC_LED, OUTPUT);
```


### 推定値の校正

推定CO₂換算値や推定IAQについては、動作開始直後は IAQ accuracy = 0 となっており正しい値が出ません。
数時間から24時間程度動作させると校正が完了し、IAQ accuracy = 3 となり、正しい推定値が出るようになります。
ボードの電源を落としたりリセットしたりすると、校正は最初からやり直しとなってしまいます。

[basic_config_state.ino](https://github.com/boschsensortec/Bosch-BSEC2-Library/blob/master/examples/generic_examples/basic_config_state/basic_config_state.ino)は、状態を6時間ごとにEEPROMに保存するサンプルとなっており、これを使えば、校正を最初からやり直す必要はなくなります。


### 温度補正

（ケースに組み込むなど）何らかの要因により、定常的に温度が実際の値より高い値が出たり低い値が出ることがあります。
その場合は、`setTemperatureOffset()` 関数で補正値を設定することができます。

例えば、温度が実際より2.0℃高い値が出る場合は、以下のようにします。

```C
envSensor.setTemperatureOffset(2.0f);
```

こうしておくと、`BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE` には2.0℃引いて補正された値が返ってきます。


## OLED

[Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)ライブラリを使ってOLED表示を行います。
Arduino IDEのライブラリマネージャー上で "Adafruit SSD1306" で検索し、インストールします。以下の2つのライブラリも依存ライブラリとして表示されますので、併せてインストールします。

* [Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)


Arduino IDEのメニューから「ファイル」→「スケッチ例」→「Adafruit SSD1306」→「ssd1306_128x64_i2c」を開きます。これを使って動作確認をしてみましょう。

`SCREEN_ADDRESS` が 0x3D になっていますので 0x3C に書き換えます。

変更前:
```C
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
```

変更後:
```C
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
```

:::message
コメントには 0x3D for 128x64 と書かれていますが、秋月電子で売られているOLEDのI²Cアドレスは 0x3C です。
OLEDの裏面には "IIC ADDRESS SELECT" の記載があり、0x78 と 0x7A のうち、0x78 の方に0Ω抵抗が付いています。0x78 / 2 = 0x3C なので、アドレスは 0x3C です。0Ω抵抗を 0x7A の方に付け替えるとアドレスは 0x3D になります。
:::

次にI²Cで使用するピンを指定します。`setup()` 関数の先頭部分に `Wire.begin(4, 5);` の呼び出しを追加します。

```C
void setup() {
  Serial.begin(9600);
  Wire.begin(4, 5);    // ←追加
```

以下はブレッドボードで同じものを動かした例です。
https://twitter.com/k_takata/status/1751489273549946892


### カスタムフォント使用方法

Adafruit-GFX-Libraryのデフォルトで使われるフォントは、5x7ドットのフォントです。大きな文字を表示したい場合は、`setTextSize()` 関数で倍率を指定することができますが、単純に元のフォントを拡大表示するだけなので、きれいな表示にはなりません。

下のリプライコメントの左の画像は `setTextSize(2)` で2倍サイズの文字を指定した様子です。右の画像と同じフォントが単純に2倍に拡大されただけであることが分かります。
https://twitter.com/k_takata/status/1752000392073015385

そのため、大きな文字を表示するにはカスタムフォントを使うのがよいです。

:::message
Adafruit-GFX-Libraryにはデフォルトのフォント以外にもいくつかフォントが含まれていますが、今回は使用しません。
:::

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

[Anonymous Pro](https://www.marksimonson.com/fonts/view/anonymous-pro)を変換してみましょう。`fontconvert` の第1引数にはフォントファイル名を、第2引数にはポイント数を指定します。今回は8ポイントにしました。

```
$ ./fontconvert 'Anonymouse Pro.ttf' 8 > AnonymousPro8pt7b.h
```

このファイルをインクルードして、`setFont()` 関数で設定するとこのフォントが使われるようになります。

```C
display.setFont(&Anonymous_Pro8pt7b);
```

:::message
これで使えるようになる文字はU+0020からU+007Eのみです。
:::

デフォルトのフォントに戻すには、`nullptr` を指定します。

```C
display.setFont(nullptr);
```

なお、`setFont()` でカスタムをフォントを指定した場合と、デフォルトフォントを使った場合では `setCursor()` 関数の動作が変わる点に注意が必要です。
デフォルトフォントではYの値は文字の上端を指定しますが、カスタムフォントではYの値はベースラインの位置を指定します。


### カスタムフォントの調整 - グリフの追加

次に、"℃" の丸の部分を表示できるように、グリフを1つ追加します。
"°" のコードはU+00B0ですが、今回は手抜きでU+007Fの位置に丸を追加してみます。

`fontconvert` の第3引数と第4引数で開始と終了の文字コード(10進)を指定することが出来ます。0xB0を10進数で表すと176ですので、以下のようにしてU+00B0のグリフを抽出します。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 176 176 > AnonymousPro8pt7b_0xb0.h
```

生成されたファイルは以下のようになっています。

```C
const uint8_t Anonymous_Pro8pt8bBitmaps[] PROGMEM = {
  0x7B, 0x38, 0x61, 0xCD, 0xE0 };

const GFXglyph Anonymous_Pro8pt8bGlyphs[] PROGMEM = {
  {     0,   6,   6,   9,    0,  -11 } }; // 0xB0

const GFXfont Anonymous_Pro8pt8b PROGMEM = {
  (uint8_t  *)Anonymous_Pro8pt8bBitmaps,
  (GFXglyph *)Anonymous_Pro8pt8bGlyphs,
  0xB0, 0xB0, 16 };

// Approx. 19 bytes
```

`Anonymous_Pro8pt8bBitmaps` がビットマップデータ、`Anonymous_Pro8pt8bGlyphs` がグリフのデータです。

`GFXglyph` は `gfxfont.h` の中で以下のように定義されています。

```C
/// Font data stored PER GLYPH
typedef struct {
  uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
  uint8_t width;         ///< Bitmap dimensions in pixels
  uint8_t height;        ///< Bitmap dimensions in pixels
  uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
  int8_t xOffset;        ///< X dist from cursor pos to UL corner
  int8_t yOffset;        ///< Y dist from cursor pos to UL corner
} GFXglyph;
```

ビットマップのサイズが6x6、文字の幅が9、ビットマップを配置する位置は (0, -11) であることが分かります。`yOffset`が負になっているのは、ベースラインの位置が基準となっているためです。

これを `AnonymousPro8pt7b.h` に結合してみます。

まず、元のビットマップは次のようになっています。
```C
const uint8_t Anonymous_Pro8pt7bBitmaps[] PROGMEM = {
    ...
  0x73, 0x26, 0x30 };
```

ここに U+00B0 のビットマップデータを結合します。

```C
const uint8_t Anonymous_Pro8pt7bBitmaps[] PROGMEM = {
    ...
  0x73, 0x26, 0x30,
  0x7B, 0x38, 0x61, 0xCD, 0xE0 };
```

次に、元のグリフデータは次のようになっています。

```C
const GFXglyph Anonymous_Pro8pt7bGlyphs[] PROGMEM = {
    ...
  {   720,   7,   3,   9,    0,   -4 } }; // 0x7E '~'
```

ここに U+00B0 のグリフデータを結合したいのですが、`bitmapOffset` をいくつにすべきかが分かりません。そこで、末尾の U+007E のビットマップデータを確認してみます。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 126 126 > AnonymousPro8pt7b_0x7e.h
```

出力された結果を見ると、U+007Eのビットマップのサイズが3バイトであることが分かります。

```C
const uint8_t Anonymous_Pro8pt7bBitmaps[] PROGMEM = {
  0x73, 0x26, 0x30 };
```

そこで、720 + 3 = 723 をオフセットとすればいいことが分かります。

```C
const GFXglyph Anonymous_Pro8pt7bGlyphs[] PROGMEM = {
    ...
  {   720,   7,   3,   9,    0,   -4 },   // 0x7E '~'
  {   723,   6,   6,   9,    0,  -11 } }; // 0x7F => U+00B0 (Degree Sign)
```

最後に、フォントデータの最終コードを更新します。

```C
const GFXfont Anonymous_Pro8pt7b PROGMEM = {
  (uint8_t  *)Anonymous_Pro8pt7bBitmaps,
  (GFXglyph *)Anonymous_Pro8pt7bGlyphs,
  0x20, 0x7E, 16 };
```

`0x7E` となっているところを `0x7F` に書き換えます。

```C
const GFXfont Anonymous_Pro8pt7b PROGMEM = {
  (uint8_t  *)Anonymous_Pro8pt7bBitmaps,
  (GFXglyph *)Anonymous_Pro8pt7bGlyphs,
  0x20, 0x7F, 16 };
```

これで、以下のコードを実行すると、`℃` が表示されます。

```C
constexpr int baseline_Anonymous_Pro8pt = 11;

display.setFont(&Anonymous_Pro8pt7b);
display.setCursor(0, baseline_Anonymous_Pro8pt);   // Set baseline
display.println("\177C");
```

表示例です。
https://twitter.com/k_takata/status/1753052848240414977


### カスタムフォントの調整 - 字形の調整

一般に、TTFフォントなどのベクトルフォントを小さいサイズで表示しようとすると、表示が崩れる場合があります。
今回のAnonymous Pro 8ptに関しては、3,5,6,8,9,m の字形が気に入らなかったので字形を調整することにしました。

5の字形を例にします。まずは以下のコマンドで5のグリフ情報を抽出します。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 53 53 > AnonymousPro8pt7b_0x35.h
```

生成されたファイルは以下のようになっています。

```C
const uint8_t Anonymous_Pro8pt7bBitmaps[] PROGMEM = {
  0xFC, 0x80, 0x80, 0xBC, 0xC2, 0x01, 0x01, 0x41, 0x23, 0x1E };

const GFXglyph Anonymous_Pro8pt7bGlyphs[] PROGMEM = {
  {     0,   8,  10,   9,    0,   -9 } }; // 0x35 '5'
```

グリフのサイズは8x10であることが分かりますので、ビットマップデータを2進数に変換して成形すると以下のようになります。少し左に傾いたような字形になっていることが分かるでしょう。

```
11111100
10000000
10000000
10111100
11000010
00000001
00000001
01000001
00100011
00011110
```

そこで、以下のように調整してみます。

```
11111110
10000000
10000000
10111100
11000010
00000001
00000001
10000001
01000010
00111100
```

これを16進数に戻すと以下のようになります。

```C
const uint8_t Anonymous_Pro8pt7bBitmaps[] PROGMEM = {
  0xFE, 0x80, 0x80, 0xBC, 0xC2, 0x01, 0x01, 0x81, 0x42, 0x3C };
```

`AnonymousPro8pt7b.h` の `Anonymous_Pro8pt7bBitmaps` の中から、上記の修正前のデータを検索し、それを修正後のデータで置き換えます。

残りの 3,6,8,9,m の字形についても同じように調整します。

```
$ ./fontconvert 'Anonymous Pro.ttf' 8 51 51 > AnonymousPro8pt7b_0x33.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 54 54 > AnonymousPro8pt7b_0x36.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 56 56 > AnonymousPro8pt7b_0x38.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 57 57 > AnonymousPro8pt7b_0x39.h
$ ./fontconvert 'Anonymous Pro.ttf' 8 109 109 > AnonymousPro8pt7b_0x6d.h
```

さらに、上で追加した "℃" の丸も少し小さくして右に寄せることにしました。

以上の調整を行った結果の表示例です。
https://twitter.com/k_takata/status/1753078515094990864


### カスタムフォント - 簡易表示用

簡易表示用に16ポイントのフォントも用意します。

```
$ ./fontconvert 'Anonymouse Pro.ttf' 16 > AnonymousPro16pt7b.h
```

8ポイントの時と同じように丸をU+007Fに割り当てますが、サイズを小さめにするために、12ポイントのグリフを流用します。

```
$ ./fontconvert 'Anonymous Pro.ttf' 12 176 176 > AnonymousPro12pt7b_0xb0.h
```

あとは、上記の手順でデータを結合します。16ポイントについては、字形の調整は行いませんでした。

表示例です。
https://twitter.com/k_takata/status/1762877441364869551


## Ambient

Ambientにデータを送信するには、Arduino向けの純正ライブラリである[Ambient_ESP8266_lib](https://github.com/AmbientDataInc/Ambient_ESP8266_lib)を使います。

本来は、Arduino IDEのライブラリマネージャーで "Ambient_ESP8266_lib" を検索してインストールすればいいのですが、Arduino IDE 2.2.1でコンパイルしたところ以下のようなエラーが出てしまいました。

```
error: variable 'inChar' set but not used [-Werror=unused-but-set-variable]
```

そこで以下のPRを作成しましたが、2024年3月時点ではまだマージされていません。

https://github.com/AmbientDataInc/Ambient_ESP8266_lib/pull/5

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

Ambientで環境データを表示した例です。
[![ambient](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/ambient.png)](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/ambient.png)


## 表示モードの切り替え

今回、表示モードの切り替えには、GPIO9に接続したModeスイッチを使いました。このスイッチは起動時に書き込みモードに入るためのスイッチと兼用しています。

### チャタリング除去

機械式スイッチを使う場合、スイッチのオンオフを切り替える際に、接点が細かく振動することで高速にオンとオフが繰り返される現象が発生します。これをチャタリングと呼びます。（英語では chattering よりも bounce と呼ぶことが多いようです。）

チャタリングを除去する（英語では debounce）ために、今回は最初にボタンが押されてから50ms立った時点で、ボタンの状態を確認し、ボタンが押されていれば実際の処理を行うようにしました。

`setup()`関数で、GPIO9をプルアップありの入力ピンとして設定し、立ち下がりで割り込み関数`buttonPushed()`が呼ばれるように設定します。

```C
#define INTERRUPT_PIN  9

setup()
{
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonPushed, FALLING);
}

```

割り込みが発生すると、発生時刻を`pushed_time`に保存し、`button_pushed`をtrueに設定します。もし`button_pushed`が既にtrueであれば何もしません。

```C
unsigned long pushed_time = 0;
bool button_pushed = false;

void buttonPushed()
{
  if (!button_pushed) {
    pushed_time = millis();
    button_pushed = true;
  }
}
```

`loop()`関数では10msごとに`checkButtonStatus()`関数を呼び出し、ボタンの状態をチェックします。
割り込み発生から50ms以上経っていれば`digitalRead()`でボタンの状態をチェックします。LOWであればボタンが押されたと判断し、HIGHであれば無視します。その後、`button_pushed`をfalseに戻します。

```C
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

void loop()
{
  checkButtonStatus();
  delay(10);
}
```

以上の処理により、50ms未満の細かなオンオフはチャタリングとして無視されます。ボタンを押している時間が50msに満たなかった場合も無視されてしまいますが、通常は問題ないでしょう。


## 内蔵USBシリアル使用時の注意点

前述の通り、Adruino IDEのメニューの「ツール」→「USB CDC On Boot」→「Enabled」を選んでおけば、`Serial.println()`の出力はESP32-C3内蔵のUSBシリアルから出力されるようになります。この内蔵USBシリアルを使う際にいくつかはまりどころがあったので、記載しておきます。

前述の[basic.ino](https://github.com/boschsensortec/Bosch-BSEC2-Library/blob/master/examples/generic_examples/basic/basic.ino)には以下のようなコードがあります。

```C
    /* Valid for boards with USB-COM. Wait until the port is open */
    while(!Serial) delay(10);
```

コメントに書いてある通り、ポートがつながるまで待つコードです。basic.inoは測定結果をUSBシリアルに出力するサンプルですので、ポートがつながるまで待つのは妥当です。しかし、今回作成するIoT環境メーターでは、測定結果はOLEDに表示したりAmbientに送信するのがメインで、USBシリアルに出力するのは補助的なものです。この2行があると、IoT環境メーターをPCから独立して動作させることができませんので、今回は削除することにします。

ポートがつながっていない状態で、`Serial.println()`で文字を出力しようとしても結果的に捨てられるだけなので、（無駄な処理をしている以外には）特に問題はありません。

一方、PCと接続したものの、PC側で端末ソフトを開いていない場合は注意が必要です。この場合、`Serial`は正として評価され、ポートはつながっていると判定されますが、出力を受け取る側がいないので、`Serial.println()`で文字を出力しようとすると、内部バッファがいっぱいになった時点でブロックしてしまいます。出力にはタイムアウトがあるので、しばらく待つと出力が破棄されて`Serial.prinln()`から戻ってくるので永遠にブロックするわけではありませんが、望みの動作ではないでしょう。
この場合は、`Serial.availableForWrite()`で書き込みが可能かどうかをチェックしておくのがよいかもしれません。


## ソースコード

今回作成したソースコードは以下に格納しています。
https://github.com/k-takata/zenn-contents/tree/master/articles/files/esp32c3-envmeter

[`private_settings.example.h`](https://github.com/k-takata/zenn-contents/blob/master/articles/files/esp32c3-envmeter/private_settings.example.h) は `private_settings.h` に名前を変更し、Wi-FiやAmbientの認証情報を正しく設定しておくことが必要です。

Wi-FiやAmbientを使用せず、単体動作させたい場合は、[`sketch_bme680_ssd1306_esp32c3.ino`](https://github.com/k-takata/zenn-contents/blob/master/articles/files/esp32c3-envmeter/sketch_bme680_ssd1306_esp32c3.ino)の先頭部分にある `#define USE_AMBIENT` の行を以下のようにコメントアウトしてください。
```C
// If defined, send the data to Ambient.
// If not defined, don't use Wi-Fi and don't send the data to Ambient.
//#define USE_AMBIENT
```

温度の補正を行いたい場合は、`//#define TEMPERATURE_OFFSET  2.0f` の行のコメントを外し、補正値を指定してください。以下は1.5℃低い値に補正したい場合の例です。
```C
// If defined, temperature will be adjusted by subtracting this value.
#define TEMPERATURE_OFFSET  1.5f
```


## 続き

スマートリモコン機能を追加したRev. 2を設計・発注しました。

* [ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その1)](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)
<!-- [ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その2)](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-2) -->

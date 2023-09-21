---
title: "Arduino風のスケッチを動かす"
---

GR-CITRUSは、Rubyを使ったプログラミングだけではなく、[IDE for GR](https://www.renesas.com/jp/ja/products/gadget-renesas/ide-gr)を使って[Arduino](https://www.arduino.cc/)風のスケッチを動かすこともできます。
Arduinoでは、C言語風のArduino言語を使ってスケッチと呼ばれるプログラムを書くことでプログラミングができます。IDE for GRを使えば、Arduino IDEと同じような感じでスケッチを書いて動かすことができます。


## IDE for GRのインストール

[IDE for GR](https://www.renesas.com/jp/ja/products/gadget-renesas/ide-gr)のサイトから最新のパッケージをダウンロードしましょう。2023年8月時点の最新版は、Windows用が1.14、macOS用が1.13となっています。

Windows用の最新パッケージである `ide4gr-1.14-windows.zip` を適当なディレクトリに展開すると、`ide4gr-1.14-windows` というディレクトリができ、その下に `ide4gr-1.14` というディレクトリができ、その下に `ide4gr.exe` をはじめとした多数のファイルが展開されます。
IDE for GRのインストール先はどこでも構いませんが、トラブルを避けるためにディレクトリ名にはスペースを含まないようにする方がよいでしょう。今回はディレクトリ階層が深くなりすぎるのを避けるため、`ide4gr-1.14` ディレクトリを `C:\` 直下に移動することにしました。
この場合、`C:\ide4gr-1.14\ide4gr.exe` を実行すれば、IDE for GRが起動します。


## Lチカ


(あとで書く)


詳しくはRenesas公式サイトの「[IDE for GRでArduinoスケッチ](https://www.renesas.com/jp/ja/products/gadget-renesas/boards/gr-citrus/project-sketch-ide)」ページを参照してください。

## LCD表示

3章で行ったLCD表示をArduinoスケッチで同じようにやってみましょう。

I²C通信のためのライブラリは[Wire](https://www.renesas.com/jp/ja/products/gadget-renesas/reference/gr-citrus/library-wire)を使います。3章と同じように0ピンをSCL、1ピンをSDAとして使うには `Wire1` オブジェクトを使います。

`send_seq()` 関数を同じように実装してみましょう。

```CPP
#include <Arduino.h>
#include <Wire.h>

class Lcd {
private:
  const int addr = 0x3E;

public:
  Lcd() = default;
  ~Lcd() = default;

  ...

  void send_seq(const uint8_t *cmds, size_t cmdlen, const uint8_t *data=nullptr, size_t datalen=0) {
    Wire1.beginTransmission(addr);
    if (data == nullptr) {
      // Only command data
      Wire1.write(0x00);       // Command byte: Co=0, RS=0
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire1.write(cmds[i]);  // Command data byte
      } 
    } else {
      // Send command words (if any)
      for (size_t i = 0; i < cmdlen; ++i) {
        Wire1.write(0x80);     // Command byte: Co=1, RS=0
        Wire1.write(cmds[i]);  // Command data byte
      }
      // Send RAM data bytes
      Wire1.write(0x40);       // Command byte: Co=0, RS=1
      for (size_t i = 0; i < datalen; ++i) {
        Wire1.write(data[i]);  // RAM data byte
      }
    }
    Wire1.endTransmission();
  }

  ...

};
```

Arduino言語 (C++) では、関数の引数に配列を渡すとポインターとして扱われ、配列の要素数の情報が失われてしまいます。そのため、3章の実装とは異なり、`cmds` の要素数を `cmdlen` として渡し、`data` の要素数を `datalen` として渡すようにしています。

ただ、これではやはり面倒なので、C++のテンプレートを使って、サイズ情報も自動で渡せるようにしてみます。

```CPP
  template <size_t cmdlen>
  void send_seq(const uint8_t (&cmds)[cmdlen], const uint8_t *data=nullptr, size_t datalen=0) {
    send_seq(cmds, cmdlen, data, datalen);
  }

  void send_cmd(const uint8_t cmd) {
    uint8_t cmds[] = {cmd};
    send_seq(cmds);
  }

  template <size_t datalen>
  void send_data(const uint8_t (&data)[datalen]) {
    send_data(data, datalen);
  }

  void send_data(const uint8_t *data, size_t datalen) {
    send_seq(nullptr, 0, data, datalen);
  }
```

コマンドのみを渡す場合やデータのみを渡す場合など、使用方法に応じていくつかの関数を用意してみました。
これを使えば、例えば3つのコマンドからなるコマンド列を送信するには以下のようにできます。

```CPP
    uint8_t cmds3[] = {0x38, 0x0C, 0x01};
    send_seq(cmds3);
```

LCDの初期化関数は以下のように実装できます。

```CPP
  void init() {
    delay(10);
    send_cmd(0x38);
    delay(2);
    uint8_t cmds1[] = {0x39, 0x14};
    send_seq(cmds1);

    uint8_t contrast = 0x20;
    uint8_t cmds2[] = {
      uint8_t(0x70 + (contrast & 0x0F)),
      uint8_t(0x5C + ((contrast >> 4) & 0x03)),
      0x6C};
    send_seq(cmds2);
    delay(200);

    uint8_t cmds3[] = {0x38, 0x0C, 0x01};
    send_seq(cmds3);
    delay(2);
  }
```

さらに、カーソル位置を設定する `set_cursor()` 関数や、文字列を表示するための `print()` 関数も同様に実装してみます。

```CPP
  void set_cursor(int col, int row) {
    send_cmd(uint8_t(0x80 + 0x40*row + col));
  }

  void print(String s) {
    send_data(reinterpret_cast<const uint8_t *>(s.c_str()), s.length());
  }
```

ここまで来たら、LCDの表示は簡単にできます。

```CPP
Lcd lcd;

void setup() {
  // put your setup code here, to run once:
  Wire1.begin();
  lcd.init();
}

void loop() {
  // put your main code here, to run repeatedly: 
  lcd.set_cursor(0, 0);
  lcd.print("Hello World");  
}
```

これでLCDには "Hello World" と表示されます。

ソースコード全体は [`sketch_citrus_lcd.ino`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/sketch_citrus_lcd.ino) から取得できます。



---
title: "I²Cを使う (LCD表示編)"
---

ここでは、[I²C](https://ja.wikipedia.org/wiki/I2C) (I2CやIICとも表記されます) を使ってLCD表示をやってみたいと思います。I²Cは、少ない信号線でデータをやり取りできるため、組込みシステムでは表示機器やセンサーなどの周辺機器を接続するためによく使われています。

## AQM1602Y LCDモジュール

[秋月電子](https://akizukidenshi.com/)で売られているLCDモジュールは何種類もありますが、その中でAQM1602Yという16×2行のLCDモジュールを使って文字を表示してみましょう。
AQM1602Yにもバックライトの有無や表示色の違いで何種類かありますが、今回は以下の2つを試してみたいと思います。

* [AQM1602Y-RN-GBW](https://akizukidenshi.com/catalog/g/gP-11916/) (バックライト無し、コンデンサ付き)
* [AQM1602Y-FLW-FBW](https://akizukidenshi.com/catalog/g/gP-12619/) (白色バックライト付き)

AQM1602Y-RN-GBWはバックライトがない分安価です。また動作に必要な1μFの積層セラミックコンデンサも付属しています。バックライトが不要であればこちらの方がよいでしょう。
さらにAQM1602Y-RN-GBWには[日本語マニュアル](https://akizukidenshi.com/download/ds/akizuki/AQM1602Y-RN-GBW_akizuki.pdf)も付いています。他のAQM1602Yシリーズを使う場合でも、こちらにも目を通しておくとよいでしょう。


## GR-CITRUSとの接続

AQM1602Yのピンは比較的細いので、ブレッドボード上で試す場合は[丸ピンICソケット 1x9P](https://akizukidenshi.com/catalog/g/gP-01016/)などを間に挟むとよいでしょう。
特にAQM1602Y-FLW-FBWはバックライト用の端子があるため、そのままではブレッドボードには刺さりません。バックライトの重さもあるので、[丸ピンICソケット 18P](https://akizukidenshi.com/catalog/g/gP-00030/)などの方が安定するでしょう。

GR-CITRUSとは上記の日本語マニュアルを参照し、以下のようにつなぎます。

|LCD Pin|Symbol|接続先|
|-------|------|------|
|1|V0|オープン|
|2|VOUT|VDDとの間に1μF積層セラミックコンデンサを接続|
|3|CAP1N|CAP1Pとの間に1μF積層セラミックコンデンサを接続|
|4|CAP1P|(Pin 3の説明を参照)|
|5|VDD|GR-CITRUSの3.3Vピンと接続|
|6|VSS|GR-CITRUSのGNDピンと接続|
|7|SDA|10kΩ抵抗でプルアップし、GR-CITRUSの0ピンと接続|
|8|SCL|10kΩ抵抗でプルアップし、GR-CITRUSの1ピンと接続|
|9|/RES|10kΩ抵抗でプルアップ、あるいはVDDと直結|

9ピンのリセット端子は通常はVDDと直結で問題ありませんが、GR-CITRUSからリセット制御を行いたい場合は、10kΩ抵抗などでプルアップし、GR-CITRUSの適当な出力ピンにつなぎます。今回はリセット制御は行いません。

AQM1602Y-FLW-FBWのバックライトは、1ピン側にあるA端子に3.3Vをつなぎ、9ピン側のK端子にGNDをつなぎます。バックライトが明るすぎる場合は3.3VとA端子の間に適当な抵抗をつなぎます。私の場合は680Ωをつなぎ、うっすらと光る程度の明るさにしました。
ブレッドボードを使う場合は、[みの虫クリップ&ジャンパーワイヤ](https://akizukidenshi.com/catalog/g/gC-08916/)のようなものが使えます。


## プログラム

### I²C通信の初期設定

GR-CITRUSでは `I2c` クラスを使うことでI²C通信ができます。`new` 関数でインスタンスを作成する際にI²C通信に使うピンを指定します。
今回は0ピンと1ピンを使うため、`new` 関数の引数には1を指定します。

I²Cでは7bitのアドレスで通信対象を指定します。AQM1602Yのアドレスは0x3Eですので、これを `ID` 定数として設定しておきます。これは `write` 関数、`read` 関数、`begin` 関数、`request` 関数で使用します。

なおI²Cのアドレスは、7bitアドレスにread/write方向の1bitを足した8bitで表現することもありますが、GR-CITRUSの `I2c` クラスは7bitアドレスを使用します。

```ruby
class LCD
  ID = 0x3E

  def initialize
    @i2c = I2c.new(1)
  end

  ...
end
```


### コマンド・データの送信

AQM1602Yは1回のI²C通信で、複数のコマンドやRAMデータを送信することができます。

[LCDコントローラのデータシート](https://akizukidenshi.com/download/ds/sitronix/st7032.pdf)の "I2C Interface protocol" の部分に、プロトコルの説明と図が載っています。これに従うことで、以下のような使い方ができます。

* 複数のコマンドを連続送信する。
  1. コントロールバイトを Co=0, RS=0 で送信し、その後コマンドを1バイトずつ送信する。
* 0個以上のコマンドを送信した後、RAMデータを連続送信する。
  1. コントロールバイトを Co=1, RS=0 で送信し、その後コマンドを1バイト送信する。これをコマンドの数だけ繰り返す。
  2. 次にコントロールバイトを Co=0, RS=1 で送信し、その後RAMデータを1バイトずつ送信する。

これを実装したのが `send_seq` 関数です。第1引数にはコマンドの配列を渡します。コマンドを送信しない場合は空配列を渡します。第2引数にはデータの配列を渡します。データを送信しない場合は省略可能です。
コマンドを1つだけ送信するときのために、`send_cmd` 関数も用意しました。

```ruby
class LCD
  ...

  # Send a command sequence
  def send_seq(cmds, dataarr=[])
    @i2c.begin(ID)
    if dataarr.empty? then
      # Only command data
      @i2c.lwrite(0x00)   # Control byte: Co=0, RS=0
      cmds.each {|cmd|
        @i2c.lwrite(cmd)  # Command data byte
      }
    else
      # Send command words (if any)
      cmds.each {|cmd|
        @i2c.lwrite(0x80) # Control byte: Co=1, RS=0
        @i2c.lwrite(cmd)  # Command data byte
      }
      # Send RAM data bytes
      @i2c.lwrite(0x40)   # Control byte: Co=0, RS=1
      dataarr.each {|dat|
        @i2c.lwrite(dat)  # RAM data byte
      }
    end
    @i2c.end
  end

  # Send a command
  def send_cmd(cmd)
    send_seq([cmd])
  end

  ...
end
```

### LCDモジュールの初期化

AQM1602Yのデータシートに記載されたシーケンスに従ってLCDモジュールを初期化します。いくつかのコマンドは実行に時間が掛かるため、`delay` 関数を呼び出す必要があります。
特に、最初の0x38コマンドの後はデータシート上では26.3μsのウェイトが必要となっていますが、実際には `delay(2)` が必要でした。(GR-CITRUSではI2Cのクロックは100kHzになっているため、1bitの送信に10μs掛かります。そのため8bitのコマンドを1つ送信するにはACKも考慮して90μs掛かる計算になるため、追加の待ちは本来不要なはずです。)

コントラストは0から0x3Fの間で指定できますが、0x20付近がよいでしょう。

```ruby
class LCD
  ...

  def init
    #delay(10)
    send_cmd(0x38)     # Function Set: 8 bits, 2 lines, normal height, Normal mode
    delay(2)
    send_seq([0x39,    # Function Set: 8 bits, 2 lines, normal height, Extend mode
              0x14])   # Internal OSC frequency: 1/5 bias, 183 Hz

    # Contrast
    contrast = 0x20
    send_seq([0x70 + (contrast & 0x0F),          # Contrast set: low byte
              0x5C + ((contrast >> 4) & 0x03),   # Icon on, Booster on, Contrast high byte
              0x6C])   # Follower control: follower circuit on, amplified ratio = 2 (for 3.3 V)
    delay(200)
    send_seq([0x38,    # Function Set: Normal mode
              0x0C,    # Display on, cursor off, cursor position off
              0x01])   # Clear Display
    delay(2)
  end

  ...
end
```

### 文字列表示

"Set DDRAM Address" コマンドでデータの書き込み先をDDRAMに設定した後、文字列データを書き込むことでLCDに文字列が表示されます。

データの書き込み先をDDRAMに設定する関数、すなわちカーソル位置を設定する関数として `set_cursor` 関数を用意しました。第1引数で桁、第2引数で行を指定します。(左上が0桁0行)

現在のカーソル位置に文字列を表示する関数として、`print` 関数を用意しました。


```ruby
class LCD
  ...

  # Set cursor position
  def set_cursor(col, row)
    send_cmd(0x80 + 0x40*row + col)  # Set DDRAM Address
  end

  # Show the string
  def print(cs)
    send_seq([], cs.bytes)
  end

  ...
end
```

ここまでで、LCD上に文字列を表示するための関数が一通り用意できました。以下のように全部を1つのソースコードにまとめて(LCDクラス内は省略)、コンパイル・実行するとLCD上に "Hello World" と表示されます。

```ruby:lcd-hello.rb
#!mruby

class LCD
  ...
end

lcd = LCD.new

lcd.init

lcd.set_cursor(0, 0)  # 0桁, 0行目
lcd.print("Hello World")
```

ソースコード全体は [`lcd-hello.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/lcd-hello.rb) から取得できます。

ブレッドボード上で動かしてみた様子がこちらです。

[![lcd-hello](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/lcd-hello-small.jpg)](https://raw.githubusercontent.com/k-takata/zenn-contents/master/books/d5c77046e634bb/images/lcd-hello.jpg)


### 外字

AQM1602Yは文字コード0x00から0x07までの8文字に外字を登録することができます。CGRAMに字形データを書き込むことで自分の好きな文字を表示することができます。

"Set CGRAM Address" コマンドでデータの書き込み先をCGRAMに設定した後、1文字当たり8バイトの字形データを書き込むことで外字が登録できます。
AQM1602Yでは1文字は横5ドット、縦8ラインで表現されています。1ラインを1バイト(上位3ビットは未使用)で表現し、8ライン分を合わせて1文字当たり8バイトとなります。

外字を登録するための関数として、`set_cgram` 関数を用意しました。第1引数で0x00から0x07の文字コードを指定します。第2引数で8バイトの字形データの配列を指定します。(16バイト以上の字形データを指定すれば、複数の外字をまとめて登録できます。)

```ruby
class LCD
  ...

  # Set CGRAM
  #   num: 0-7
  #   pat: Character pattern (8-bytes per character)
  def set_cgram(num, pat)
    send_seq([0x40 + num*8], pat)
  end

  ...
end
```

例えば以下のように使うことで、文字コード0x00に小さな丸を登録することができます。

```
# Degree symbol
lcd.set_cgram(0x00, [0x07, 0x05, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00])
```

その後以下のようにすれば、"25℃" と表示することができます。

```
lcd.set_cursor(0, 0)  # 0桁, 0行目
lcd.print("25\x00C")
```

`print` 関数を呼ぶ前に `set_cursor` 関数を呼び出して、データの書き込み先をCGRAMからDDRAMに切り替えていることに注意してください。

`print` 関数を使って外字を表示した状態で、`set_cgram` 関数を使って外字の字形を書き換えると、それに従って表示される字形も変わります。これをうまく活用すれば簡単なアニメーションを表示することも可能です。


### その他の機能

AQM1602Yにはほかにも以下のような機能がありますが、ここでは割愛します。

* 画面消去
* ホームへ移動
* カーソル表示
* カーソルや画面のシフト
* 縦倍角フォント

AQM1602Yのデータシートにはアイコン表示に関する記載もありますが、このLCDモジュールにはアイコン表示機能はありません。
対応するLCDモジュールであれば、バッテリー残量のアイコンなどが表示できるようになっています。例: [I2C低電圧キャラクタ液晶モジュール(16x2行) - SB1602B - Strawberry](https://strawberry-linux.com/catalog/items?code=27001)

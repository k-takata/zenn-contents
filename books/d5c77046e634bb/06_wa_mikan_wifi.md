---
title: "WA-MIKANと組み合わせて使う (Wi-Fi編)"
---

ここでは、CR-CITRUSとWA-MIKANを組み合わせてWi-Fi機能を使ってみます。


## Wi-Fiクライアントとして使う

Wi-Fi機能を使うには `WiFi` クラスを使用します。
WA-MIKANに搭載されているESP8266を初期化するには、GR-CITRUSの5ピンを出力モードに設定し、一旦LOWにしてからHIGHにします。
5ピンをHIGHにしてから500ms程度経ってから `System.use?('WiFi')` を呼び出して `true` が返れば `WiFi` クラスが使用可能です。
`WiFi.setMode(1)` でステーションモードに設定し、`WiFi.connect()` にSSIDとパスワードを指定して呼び出すと、接続が開始されます。10秒程度待ってから `WiFi.ipconfig` を呼び出すと、IPアドレスが返ってくるはずです。接続が完了していない場合、IPアドレスとして 0.0.0.0 が返ってきます。

```ruby
!mruby

SSID = '**************'
PASSWD = '*************'

# Reset ESP8266
pinMode(5, OUTPUT)
digitalWrite(5, LOW)  # Disable
delay 500
digitalWrite(5, HIGH) # Enable
delay 500

if (System.use?('WiFi')) then
  WiFi.setMode(1)  # Station mode
  WiFi.connect(SSID, PASSWD)

  s = WiFi.ipconfig
  puts s
  delay(5000)
  s = WiFi.ipconfig
  puts s
  delay(5000)
  s = WiFi.ipconfig
  puts s
  delay(5000)
  s = WiFi.ipconfig
  puts s
end
```

(実行例、あとで書く)


## Ambientで測定データを可視化する

[Ambient](https://ambidata.io/)というIoTデータ可視化サービスがあります。このサービスを使うと、GR-CITRUSで取得した各種データをグラフ等を使って可視化することが可能です。


### Ambientにデータを送信してみる

まずは公式ドキュメントの[チュートリアル](https://ambidata.io/docs/)に従ってユーザー登録などを行いましょう。
次にチャネルを作成すると、チャネルIDとライトキーが発行されます。Ambientにデータを送信するにはこの2つの情報が必要になります。

Ambientでは、いくつかの環境用に公式ライブラリが用意されていますが、GR-CITRUS用のライブラリは用意されていませんので、[python ライブラリ](https://github.com/AmbientDataInc/ambient-python-lib)などを参考に自分でデータ送信用のコードを書くことになります。幸い、以下のように簡単なコードで実装可能です。

```ruby
class Ambient
  def initialize(channelId, writeKey)
    @channelId = channelId
    @writeKey = writeKey
  end

  def send(hash)
    arr = []
    hash.each {|k, v|
      arr << "\"#{k}\":#{v}"
    }
    body = "{\"writeKey\":\"#{@writeKey}\",\"data\":[{#{arr.join(',')}}]}"
    url = "ambidata.io/api/v2/channels/#{@channelId}/dataarray"
    ret = WiFi.httpPost(url, ['Content-Type: application/json'], body)
  end
nd
```

この `Ambient` クラスを使えば、以下のようなコードでデータを送信できます。(Wi-Fiは別途初期化してあるものとします。)

```ruby
CHANNELID = '*****'
WRITEKEY = '****************'

@ambi = Ambient.new(CHANNELID, WRITEKEY)
temp = 26.97    # 気温
hum = 47.406    # 湿度
pres = 1000.48  # 気圧
di = 74.02      # 不快指数
@ambi.send({"d1" => temp, "d2" => hum, "d3" => pres, "d4" => di})
```

ここでは、データ1として気温、データ2として湿度、データ3として気圧、データ4として不快指数を送信しています。タイムスタンプは設定せず、Ambient側で設定させるようにしています。


### AmibentにBME680の測定データを送信する

前章のBME680を使った環境メーターと組み合わせて、測定データを10分ごとにAmbientに送信するようにしてみました。ソースコード全体は [`envmeter-bme680-ambient.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/envmeter-bme680-ambient.rb) から取得できます。
Wi-Fiの初期化には時間が掛かるため、3段階に分けて初期化を行うようにしています。Wi-Fiを初期化している間もLCDには測定値が表示されます。さらに、起動後20秒経ってから初めてデータを送信するようにしてあります。

これを動かすと、例えば以下のようにグラフ化が可能です。

(グラフの例、あとで書く)


## サーバーとして動かす

(未定)

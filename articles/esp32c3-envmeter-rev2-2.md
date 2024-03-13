---
title: "ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その2)"
emoji: "🎚️"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680", "pcbway"]
published: false
---

## 概要

本記事は、以下の記事の続きとなります。

* [ESP32-C3とBME680でIoT環境メーターを作る](https://zenn.dev/k_takata/articles/esp32c3-envmeter)
* [ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その1)](https://zenn.dev/k_takata/articles/esp32c3-envmeter-rev2-1)

今回は、その1で発注した基板の受領、組み立て、ソフトウェアの作成について記載します。


## 基板到着

元々PCBWayでの基板の製造が3 ~ 4日、OCSでの送付に2 ~ 3日の予定でしたが、実際には3/4に製造開始して、3日後の3/7に完了、同日中にOCSに引き渡され、4日後の3/11に一度配達されたものの自分が不在だったために、3/12に再配達となりました。結局、再配達を除けばちょうど1週間で基板を受け取ることができました。

見慣れたJLCPCBのネコポス用の箱に比べると、かなり大きな箱で届きました。

https://twitter.com/k_takata/status/1767352744569111038

左が今回PCBWayで作ったRev. 2、右が前回JLCPCBで作ったRev. 1です。写真では見にくいですが、今回のRev. 2はRev. 1に比べると明らかにつやがあります。
PCBWayでは、黒と黒(つや消し)が明確に分かれているだけあって、つや消しの黒がよい場合は明示的につや消しを選ぶ必要があります。（価格は上がりますが。）


### 受領確認

PCBWayの注文リストを見ると、基板が到着したのに配達状態のままになっており、完了状態に移行しないのを不思議に思っていたのですが、よく見ると「受け確認」と書かれたボタンがあることに気づきました。ボタンを押すと「受け確認?」とだけ聞かれて意味が分かりませんでしたが、どうやらこれが受領確認のボタンだったようです。（英語ページでは "Confirm received" でした。）
ボタンは「受領確認」、質問メッセージは「製品を受け取りましたか？」だったら分かりやすいと思いました。日本語訳の改善に期待したいです。
このボタンを押すと、無事完了状態になりました。

![confirm received](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/confirm-received.png)

さらに、受領確認を行ったことで、[PCBWayリワード画面](https://member.pcbway.jp/specials/rewards)でポイントとBeansが加算されていることが確認できました。
1回の注文ごとに10ポイント、さらに$1の支払いにつき1ポイントで、合計28ポイント加算されていました。


### フィードバックを残す

「受け確認」ボタンの下にあった「フィードバックを残す」ボタンを押すと、以下のような画面になりました。

![feedback](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/feedback.png)

（英語で）50語以上のフィードバックを送るとクーポンをもらえるチャンスがあるようです。暇な時にでもフィードバックを送ってみたいと思います。

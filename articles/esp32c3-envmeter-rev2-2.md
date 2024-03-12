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

今回は、その1で発注した基板を組み立てて、ソフトウェアを作成します。


## 基板到着

元々基板の製造が3 ~ 4日、送付に2 ~ 3日の予定でしたが、実際には3/4に製造開始して、3日後の3/7に完了、同日中にOCSに引き渡され、4日後の3/11に1回配達されたものの自分が不在だったために、3/12に再配達となりました。結局、再配達を除けばちょうど1週間で基板を受け取ることができました。

見慣れたJLCPCBのネコポス用の箱に比べると、かなり大きな箱で届きました。

https://twitter.com/k_takata/status/1767351146279297070

左が今回PCBWayで作ったRev. 2、右が前回JLCPCBで作ったRev. 1です。写真では見にくいですが、今回のRev. 2はRev. 1に比べると明らかにつやがあります。
つや消しの黒がよい場合は、PCBWayでは明示的につや消しを選ぶ必要があります。（価格は上がりますが。）

https://twitter.com/k_takata/status/1767352744569111038

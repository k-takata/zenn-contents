---
title: "ESP32-C3とBME680でIoT環境メーター/スマートリモコンを作る (その1)"
emoji: "🎚️"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["電子工作", "esp32", "esp32c3", "bme680", "pcbway"]
published: false
---

## 概要

前回作成した[IoT環境メーター](https://zenn.dev/k_takata/articles/esp32c3-envmeter)を改良し、スマートリモコン機能を追加したRev. 2を設計・発注しました。
スマートリモコン機能は、以下の記事で紹介した内容を[WA-MIKAN](https://akizukidenshi.com/catalog/g/g111218/) (ESP8266)から[ESP32-C3](https://akizukidenshi.com/catalog/g/g117493/)に移植したものとなっており、Slack経由でエアコンを制御できるというものです。

* [WA-MIKANを単体で使う（その1）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/10_wa_mikan_only1)
* [WA-MIKANを単体で使う（その2）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/11_wa_mikan_only2)


## ハードウェア

前回同様に、[PCB_envmeter_esp32c3](https://github.com/k-takata/PCB_envmeter_esp32c3)で回路図と基板を公開しています。

前回のRev. 1と異なる点は、赤外線送受信部が追加されていることです。赤外線送受信部の回路設計については、[WA-MIKANを単体で使う（その1）](https://zenn.dev/k_takata/books/d5c77046e634bb/viewer/10_wa_mikan_only1)に記載しています。赤外線受信部のピンはIO14の代わりにGPIO10を、赤外線送信部のピンはIO12の代わりにGPIO6を使用しています。
(スマートリモコン機能を使わない場合は、これらの部品を取り付けず、Rev. 1と同じ使い方をすることもできます。)

また、今回は[PCBWay](https://www.pcbway.com/)様からのスポンサーの申し出があったため、PCBWay様に基板を発注しています。


## PCBWayでの発注

今まで基板はJLCPCBに発注していましたが、今回初めてPCBWayに発注することになりましたので、いろいろと比較してみたいと思います。

### ガーバーファイルの作成

KiCadでPCBWay用のガーバーファイルを作成するには、専用のプラグインを使うのが楽です。
KiCadのプラグイン＆コンテンツ マネージャーで "PCBWay" で検索すると、2つのプラグインが見つかります。

![KiCad plugins](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/kicad-plugins.png)

* [PCBWay Plug-in for KiCad](https://www.pcbway.com/blog/News/PCBWay_Plug_In_for_KiCad_3ea6219c.html)  
  ボタン一発で、PCBWay向けのガーバーファイルの作成から、ブラウザで注文画面を開いてガーバーファイルのアップロードまでできてしまいます。ただし、注文画面は[英語画面](https://www.pcbway.com/orderonline.aspx)が開くようになっており、[日本語画面](https://www.pcbway.jp/orderonline.aspx)は開けません。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの直下に作成されます。
* [PCBWay Fabrication Toolkit](https://www.pcbway.com/blog/News/PCBWay_Fabrication_Toolkit_for_Kicad_23c41e77.html)  
  ボタン一発でPCBWay向けのガーバーファイルの作成ができます。  
  ガーバーファイルは、KiCadプロジェクトディレクトリの下に `pcbway_production` というディレクトリが作成され、その下に作成されます。

ボタン一発で注文画面まで行ける方がよく、英語も苦にならない場合は、前者がよいでしょう。
一方、発注前にガーバーファイルの内容を確認したい場合や、日本語画面から注文したい場合は後者がよいでしょう。


### 注文画面

![PCBWay order](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/order.png)
*PCBWayの注文画面*

スタンダード基板の注文画面で選択できる項目について、PCBWayとJLCPCBで違う点を挙げてみました。

* 5枚と10枚で値段が変わらない
  - PCBWay: 5 ~ 10枚で$5.00
  - JLCPCB: 5枚で通常価格$4.00のところ割引で$2.00、10枚で$5.00
* 基板の層の選択肢が多い
  - PCBWay: 1 ~ 14層 (ただし4層以上は追加料金)
  - JLCPCB: 1 ~ 4層 (4層でも50x50mm以内なら2層までと同一価格)
* FR4-TG(ガラス転移温度)の選択ができる
  - PCBWay: TG 130-140, TG 150-160が同一価格で選択可能。追加料金で他も選択可能。
  - JLCPCB: TG 135-140のみ
* PCBの厚さの選択肢が多い
  - PCBWay: 0.2 ~ 3.2mm
  - JLCPCB: 0.4 ~ 2.0mm
* レジストの選択肢が多い
  - PCBWay: 黒(つや消し)、緑(つや消し)が選べる。ただし、つや消しと紫は追加料金。
  - JLCPCB: 紫を含め、同一料金。
* シルクの選択肢が多い
  - PCBWay: 白、黒、黄色、なしから選択可能。標準以外の組み合わせは追加料金。
  - JLCPCB: レジストの色により白か黒か自動決定。
* 表面処理の選択肢が多い
  - PCBWay: 10種類以上。PCBWayの裁量でHASLをENIGに変更する可能性があることに同意というチェックがある。
  - JLCPCB: HASL、無鉛HASL、ENIG
* 銅箔の選択肢が多い
  - PCBWay: 1 ~ 13 oz
  - JLCPCB: 1 ~ 2 oz
* 発注番号の位置指定
  - PCBWay: `WayWayWay`
  - JLCPCB: `JLCJLCJLCJLC`

全般的に、スタンダード基板で選択できる幅はPCBWayの方が多いです。価格はオプションにもよりますが、ほぼ同じか若干JCLPCBの方が安いように見えます。

注文画面の操作性の違いについては以下の点が挙げられます。

* リセットボタンがある  
  JLCPCBとは異なり、リセットボタンがあるので設定を最初からやり直したいときに便利です。
* ガーバーファイルのアップロード方法  
  JLCPCBとは異なり、「クイックオーダー基板」のリンクをクリックすることでガーバーファイルがアップロードできるようになり、一手間多いです。


### 住所入力

pcbway.jpであれば、住所入力欄の説明もほぼ日本語になっており、それほど苦になりません。
Cityの訳が市や都市ではなく都会になっているのは惜しいです。「都会」欄に市名を入力すると、郵便番号も一覧になって出てくるのですが、郵便番号の絞り込みはできないようで、数十項目の中から目的の郵便番号を選択するのは少し面倒でした。
![address input](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/address-input.png)


### 送料

JLCPCBでは、OCS NEPを選択すると1ドル程度、OCS Expressを選択すると2ドル程度(どちらも6 ~ 8営業日)になりますが、PCBWayには同様のOCSの格安サービスはないようです。

![JLCPCB shipping](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shipping-jlcpcb.png)
*JLCPCBの配送方法*

一方PCBWayでは、担当者にOCSを選択することを強く推奨されました。今回は2 ~ 3営業日で$13.04と表示されました。OCSよりも安価な業者もいくつかありますが、配送期間とのバランスを考えると推奨通りOCSが一番よさそうです。

![PCBWay shipping](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shipping.png)
*PCBWayの配送方法*


### 支払い

今回はPCBWay様のご厚意により、基板製作費と送料を負担していただけることになりました。
注文画面からガーバーファイルをアップロードし、レジスト色と注文番号の位置指定のオプションを選んでカートに追加すると、製造番号が発行されましたので、それを担当者に連絡したところ、製作費とOCS前提の送料が自分のアカウントの残高として追加されました。
![message](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/message.png)

支払い時に、配送業者としてOCSを選択し、クーポン選択欄でAccount Balance (アカウント残高)にチェックを入れたところ、無事$0.0で発注ができました。
![payment](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/payment.png)

支払い方法として、PayPalやクレジットカードを使う場合は、PayPal feeやBank feeとして1ドル程度取られるようでした。


### 生産追跡

PCBWayも(JLCPCBなどと同様に)生産状況を確認することができます。今回は3/4の昼頃に発注して、3/7の未明に完成しました。黒のレジストを選択したため、元々3 ~ 4日の予定でしたが、思ったより早く完成しました。
![production tracking](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/production-tracking.png)

生産追跡画面で再生ボタンを押すと、各工程のビデオを見ることができるはずなのですが、pcbway.jpではビデオの画面が生産追跡画面の後ろに表示されてしまい、見れませんでした。pcbway.comでは正しく表示されるので、改善をお願いしたいです。


### 共有プロジェクト

PCBWayで面白いと思ったサービスが、[共有プロジェクト](https://www.pcbway.com/project/shareproject/)です。
自分の作成した基板をオープンソースハードウェアとして公開する場合、それをPCBWayの共有プロジェクトとして登録することができます。あるいは、他の人が設計した基板で気に入ったものがあれば、それを発注することができます。
誰かが自分のプロジェクトを気に入って基板を発注した場合、その金額の10%が自分に支払われるとのことです。

![shared projects](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/shared-projects.png)

注文管理ページの "Share&Sell" ボタンを押せば、発注済みのプロジェクトを共有することができます。また、[Edit Your Project - PCBWay Community](https://www.pcbway.com/project/shareproject/techshare.aspx)から新規にプロジェクトを共有することもできます。


### ポイント

もう一つ面白いと思ったのが、[PCBWayリワード](https://member.pcbway.jp/specials/rewards)という仕組みです。
PCBWayポイントを貯めることで、1 ~ 5%の割引などの特典を受け取ることができるようです。
また、PCBWay Beans (豆)を貯めることで、ギフトやクーポンを引き換えることができるようです。

![rewards](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/rewards.png)


### クーポン

PCBWayの初回登録時には、$5以上の注文で$5の割引となるクーポンがもらえます。
それに加えて、いくつかクーポンをもらうことができます。画面の左側を見ると、いくつかクーポンのマークがついている項目があります。

![coupon](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/coupon.png)

* 紹介プログラム  
  友達を紹介するとクーポンがもらえます。
* 共有プロジェクト  
  (クーポンマークがついていますが、どういう条件でもらえるのか分かりません。)
* アカウント設定  
  プロフィールを設定すると記入した項目数に応じて最大で$35のクーポンがもらえます。
* A5  
  アンケートに回答すると抽選で最大$100のクーポンがもらえます。

今回、「アカウント設定」→「Edit Profile」から電話番号と誕生日以外すべて（アイコンも含む）記入したところ、$34のクーポンをもらえました。（ただ、使える条件がそこそこ厳しいので、そのまま失効しそうです。）
Edit Profileで入力した情報の一部（少なくとも名前とアイコン）は、公開されますので注意してください。

![coupon by profile](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/pcbway/coupon-by-prof.png)


## 続き

今回は、IoT環境メーター/スマートリモコン Rev. 2の設計とPCBWayでの発注までをまとめました。
続きは別記事としてまとめたいと思います。

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

---
title: "開発環境の整備"
---

[GR-CITRUSの公式ページ](https://www.renesas.com/jp/ja/products/gadget-renesas/boards/gr-citrus)では、Rubyでプログラミングをするために[Rubicというアプリを使った方法](https://www.renesas.com/jp/ja/products/gadget-renesas/boards/gr-citrus/project-rubic-ruby)を紹介しています。
しかし残念ながら、2023年8月現在、Rubicは最新のChromeに対応しておらずインストールすることができません。また、同じ作者によるVS Code向けの拡張としてのRubicもありますが、こちらもやはり最新のVS Codeには対応しておらずインストールできません。

このため、GR-CITRUSリリース当時のように手軽にRubyを使ってボードを動かすという訳にはいきませんが、以下の手順を踏むことで引き続きRubyでプログラミングができます。

1. mrubyをコンパイルし、mrbcコマンドを用意する。
2. RubyでGR-CITRUS用のプログラムを書く。
3. mrbcコマンドで2.のプログラムをmrbファイルにコンパイルする。
4. シリアル端末ソフトを使い、3.のmrbファイルをGR-CITRUSに転送・実行する。

ここではWindows上での開発環境の整備方法を説明していきます。


## 必要なソフトウェア

WindowsでGR-CITRUSとmrubyを使うためには以下のソフトウェアが必要です。

* [Tera Term](https://ttssh2.osdn.jp/)
* Visual C++ (2015, 2017, 2019, 2022のいずれか)
* Ruby 3.x ([RubyInstaller](https://rubyinstaller.org/))
* GNU Bison (Yacc) (CygwinやMSYS2に入っているものでも良い)


## GR-CITRUS Rubyファームウェアの更新

まずは、GR-CITRUSのファームウェアを最新の物に更新しておきます。

GR-CITRUSファームウェアリポジトリの[Releases](https://github.com/wakayamarb/wrbb-v2lib-firm/releases)ページに行き、最新のファームウェアをダウンロードします。
2023年8月現在の最新版はv2.50です。Assetsからdata.zipをダウンロードし、解凍すると `citrus_sketch.bin` というファイルがいくつか出てきます。`release.json` にはそれぞれにどのようなmrbgemsが組み込まれているかが書かれています。特に目的が無ければ「最小構成版」を選んでおけば良いでしょう。

GR-CITRUSボードとPCをUSBケーブルで接続すると、ボードがシリアルポートとして認識されます。Tera Term等のシリアル端末ソフトを接続するとRubyファームウェアが起動していることが確認できます。

この際、シリアル端末ソフトの設定は以下のようにしておきます。

| 項目 | 設定値 |
|------|--------|
| スピード | 115200 |
| データ | 8 bit |
| パリティ | none |
| ストップビット | 1 bit |
| フロー制御 | none |
| 改行コード (受信) | CR |
| 改行コード (送信) | CR |


ここでボード上のリセットボタンを押すと、今度はボードがUSBメモリとして認識されます。先ほどダウンロードした `citrus_sketch.bin` をここにコピーしてしばらく待つと、自動でボードの再起動が掛かり、再びボードがシリアルポートとして認識されます。

Tera Term上でEnterを押すと、以下のような表示が出ます。

```
EEPROM FileWriter Ver. 1.79.v2
 Command List
 L:List Filename..........>L [ENTER]
 W:Write File.............>W Filename Size [ENTER]
 G:Get File...............>G Filename [ENTER]
 F:Get File B2A...........>F Filename [ENTER]
 D:Delete File............>D Filename [ENTER]
 Z:Delete All Files.......>Z [ENTER]
 A:List FAT...............>A [ENTER]
 R:Run File...............>R Filename [ENTER]
 N:Run File Ignore break..>N Filename [ENTER]
 X:Execute File...........>X Filename Size [ENTER]
 S:List Sector............>S Number [ENTER]
 .:Repeat.................>. [ENTER]
 Q:Quit...................>Q [ENTER]
 E:System Reset...........>E [ENTER]
 M:Drive Mount............>M [ENTER]
 U:Write File B2A.........>U Filename Size [ENTER]
 T:'>'Auto Print Switch...>T [ENTER]
 C:License................>C [ENTER]

WAKAYAMA.RB Board Ver.CITRUS-2.50(2018/6/7)f4(256KB), mruby 1.4.0 (H [ENTER])
>
```


ファームウェアのバージョンが2.50であり、mrubyのバージョンが1.4.0であることが確認できます。


## mrubyのコンパイル

前述の通り、GR-CITRUSのRubyファームウェアで使われているmrubyのバージョンが1.4.0であることが分かりましたので、このバージョンのmrubyを用意します。

まずは、mruby 1.4.0のソースコードを取得します。Git for Windows (Git Bash)を起動し、以下のコマンドを入力します。(行頭の `$` はプロンプトですので入力は不要です。)

```shell-session
$ git clone https//github.com/mruby/mruby.git
$ cd mruby
$ git checkout 1.4.0
```

mrubyをコンパイルするにはRubyが必要ですが、Ruby 3.0から関数の引数の扱いが変わったため、このままでは `Rakefile` でエラーが出てしまいます。
[以下のパッチ](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/Rakefile.diff)のように、`{ :verbose => $verbose }` をすべて `:verbose => $verbose` に置き換えます。

```diff:Rakefile.diff
--- a/Rakefile
+++ b/Rakefile
@@ -37,15 +37,15 @@ load "#{MRUBY_ROOT}/tasks/gitlab.rake"
 task :default => :all
 
 bin_path = ENV['INSTALL_DIR'] || "#{MRUBY_ROOT}/bin"
-FileUtils.mkdir_p bin_path, { :verbose => $verbose }
+FileUtils.mkdir_p bin_path, :verbose => $verbose
 
 depfiles = MRuby.targets['host'].bins.map do |bin|
   install_path = MRuby.targets['host'].exefile("#{bin_path}/#{bin}")
   source_path = MRuby.targets['host'].exefile("#{MRuby.targets['host'].build_dir}/bin/#{bin}")
 
   file install_path => source_path do |t|
-    FileUtils.rm_f t.name, { :verbose => $verbose }
-    FileUtils.cp t.prerequisites.first, t.name, { :verbose => $verbose }
+    FileUtils.rm_f t.name, :verbose => $verbose
+    FileUtils.cp t.prerequisites.first, t.name, :verbose => $verbose
   end
 
   install_path
@@ -78,8 +78,8 @@ MRuby.each_target do |target|
         install_path = MRuby.targets['host'].exefile("#{bin_path}/#{bin}")
 
         file install_path => exec do |t|
-          FileUtils.rm_f t.name, { :verbose => $verbose }
-          FileUtils.cp t.prerequisites.first, t.name, { :verbose => $verbose }
+          FileUtils.rm_f t.name, :verbose => $verbose
+          FileUtils.cp t.prerequisites.first, t.name, :verbose => $verbose
         end
         depfiles += [ install_path ]
       elsif target == MRuby.targets['host-debug']
@@ -87,8 +87,8 @@ MRuby.each_target do |target|
           install_path = MRuby.targets['host-debug'].exefile("#{bin_path}/#{bin}")
 
           file install_path => exec do |t|
-            FileUtils.rm_f t.name, { :verbose => $verbose }
-            FileUtils.cp t.prerequisites.first, t.name, { :verbose => $verbose }
+            FileUtils.rm_f t.name, :verbose => $verbose
+            FileUtils.cp t.prerequisites.first, t.name, :verbose => $verbose
           end
           depfiles += [ install_path ]
         end
@@ -127,16 +127,16 @@ end
 desc "clean all built and in-repo installed artifacts"
 task :clean do
   MRuby.each_target do |t|
-    FileUtils.rm_rf t.build_dir, { :verbose => $verbose }
+    FileUtils.rm_rf t.build_dir, :verbose => $verbose
   end
-  FileUtils.rm_f depfiles, { :verbose => $verbose }
+  FileUtils.rm_f depfiles, :verbose => $verbose
   puts "Cleaned up target build folder"
 end
 
 desc "clean everything!"
 task :deep_clean => ["clean"] do
   MRuby.each_target do |t|
-    FileUtils.rm_rf t.gem_clone_dir, { :verbose => $verbose }
+    FileUtils.rm_rf t.gem_clone_dir, :verbose => $verbose
   end
   puts "Cleaned up mrbgems build folder"
 end
```

次にスタートメニューから "Developer Command Prompt for VS 2022" を選び、Visual C++ のコマンドプロンプトを起動します。
mrubyのディレクトリ(以下の例では `\path\to\mruby`)に移動し、`ruby minirake` コマンドを実行します。もしruby.exeとbison.exeにパスが通っていないようであれば、パスを通しておきます。(where.exeコマンドでパスが通っているか確認できます。)

```
> cd \path\to\mruby
> PATH %PATH%;C:\Ruby31-x64\bin;C:\msys64\usr\bin
> where.exe ruby
C:\Ruby31-x64\bin\ruby.exe

> where.exe bison
C:\msys64\usr\bin\bison.exe

> ruby minirake
```

問題なく終われば、`bin` ディレクトリ内に `mrbc.exe` などができますので、このディレクトリにパスを通しておきます。コマンドプロンプト上で一時的にパスを通すには以下のようにします。

```
> path %PATH%;\path\to\mruby\bin
```


## 初めてのLチカ

電子工作界隈ではマイコンを動かすときに始めてやることはLEDを点滅させること(Lチカ)だと言われています。早速やってみましょう。

まずは以下の内容を [`main.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/main.rb) として保存します。

```ruby:main.rb
#!mruby

10.times do
  led
  delay 250
end
```

`led` 関数は、引数無しで実行した場合、現在LEDが点いていれば消灯し、消えていれば点灯します。
`delay` 関数はミリ秒単位で待ちます。
つまり、このプログラムはLEDを0.5秒間隔で5回点滅させるものになります。

次にmrbcコマンドを使ってmrbファイルにコンパイルします。

```
> mrbc main.rb
```

`main.mrb` ファイルができているのが確認できます。

```
> dir main.*
 ドライブ C のボリューム ラベルは Windows です
 ボリューム シリアル番号は FA35-D0E9 です

 C:\work\gr-citrus\sample のディレクトリ

2023/08/20  11:37               150 main.mrb
2023/08/20  11:36                42 main.rb
               2 個のファイル                 192 バイト
               0 個のディレクトリ  242,486,083,584 バイトの空き領域
```

`main.mrb` のサイズは150バイトでした。

次にTera Termを開きます。EEPROM FileWriterのプロンプト(`>`)が表示されている状態で、`W main.mrb 150` と入力し、Enterを押します。(`150`の部分は `main.mrb` のファイルサイズに合わせる必要があります。) すると60秒間のカウントダウンが始まりますので、その間にTera Termのメニューから「ファイル」→「ファイル送信」を選びます。ファイル送信のダイアログボックスが表示されたら、「オプション」の「バイナリ」にチェックを入れてから、先ほどの `main.mrb` を選択して「開く」を押します。

```
WAKAYAMA.RB Board Ver.CITRUS-2.50(2018/6/7)f4(256KB), mruby 1.4.0 (H [ENTER])
>W main.mrb 150

Waiting  60 59 58 57 56 55 54 53 52 51 50 49
main.mrb(150) Saving..

WAKAYAMA.RB Board Ver.CITRUS-2.50(2018/6/7)f4(256KB), mruby 1.4.0 (H [ENTER])
>
```

`R` [ENTER] を押すと `main.mrb` の実行が始まり、LEDが5回点滅したあと、EEPROM FileWriterのプロンプトに戻ってきます。

:::message
1秒ごとに表示される `>` が邪魔なときは、`T` [ENTER] を押すことで自動表示を止めることができます。再度 `T` [ENTER] を押すと元に戻ります。
:::


## もっと簡単に実行する

ここまでで、Rubyでプログラムを書いてGR-CITRUSボードで実行することができるようになりました。しかし、mrbファイルを書き込む際にはファイルサイズを手動で指定する必要があり、実行するのが非常に面倒です。

Tera Termマクロを使って、もっと簡単にmrbファイルを書き込んで実行できるようにしてみましょう。


以下の [`runfile-main.ttl`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/runfile-main.ttl) ファイルを、`main.rb` や `main.mrb` と同じディレクトリに保存しましょう。

```:runfile-mail.ttl
connect '/C=6'
if result<>2 then
	messagebox 'No connection.' 'Error'
	exit
endif

filename = 'main.mrb'
getdir dir
makepath filename dir filename
filestat filename size
basename fname filename
sprintf2 str 'X %s %d' fname size
sendln str
wait 'Waiting'
sendfile filename 1
```

1行目の `/C=6` の部分は "COM6" ポートに接続することを意味しています。自身の環境に合わせて書き換えてください。
7行目の `'main.mrb'` は、書き込み・実行するファイル名です。別のファイルを実行したい場合は(マクロのファイル名とともに)書き換えてください。

Tera Termのメニューから「コントロール」→「マクロ」を選び、先ほどの `runfile-main.ttl` を選択すると、同じディレクトリにある `main.mrb` の書き込みと実行が行われます。

以下の [`runfile.ttl`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/runfile.ttl) を使えば、mrbファイルは固定ではなく、選択画面が表示され、選んだファイルを実行できます。必要に応じて使い分けてください。

```:runfile.ttl
connect '/C=6'
if result<>2 then
	messagebox 'No connection.' 'Error'
	exit
endif

filenamebox 'Select mrb file for execution' 0
if result<>0 then
	filestat inputstr size
	basename fname inputstr
	sprintf2 str 'X %s %d' fname size
	sendln str
	wait 'Waiting'
	sendfile inputstr 1
endif
```

:::message alert
`runfile-main.ttl` や `runfile.ttl` はエラーチェックが甘いため、たまに書き込みが上手くいかないことがあります。(例えばEEPROM FileWriterのプロンプトに何かを入力した状態でマクロを実行するなど) その場合は、メニューから「コントロール」→「マクロウィンドウの表示」を選び、「終了」ボタンを押してマクロの実行を中断してから、再度実行してみてください。
:::


## シリアル通信でHello World

Lチカの次は、シリアル端末に文字列を表示してみましょう。

(m)rubyでは `puts` 関数を使うことで文字列を表示できますが、GR-CITRUSでも同様に `puts` 関数で文字列を表示できます。以下の [`hello.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/hello.rb) を試してみましょう。

```ruby:hello.rb
#!mruby

puts "Hello World!"
```

このコードを `mrbc hello.rb` でコンパイルすると `hello.mrb` ができますので、それを実行すると、以下のようにシリアル端末(Tera Term)上に "Hello World!" と表示されます。

```
>X hello.mrb 102

Waiting  60 59
hello.mrb(102) Saving..
Hello World!
```

Rubyファームウェアv2.50までの `puts` 関数は、mrubyの標準の `puts` 関数とは異なり、2つ以上の引数を渡すことができません。
以下の [`hello-error.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/hello-error.rb) を見てみましょう。

```ruby:hello-error.rb
#!mruby

board = "GR-CITRUS"
puts "Hello", board
```

`mrbc` コマンドに `-g` オプションを付けて `mrbc -g hello-error.rb` でコンパイルすると、エラーが起きたときにエラーの箇所が表示されるようになります。これを実行すると以下のようになり、`hello-error.rb` の4行目でエラーになっていることが分かります。

```
>X hello-error.mrb 193

Waiting  60 59
hello-error.mrb(193) Saving..
hello-error.rb:4: wrong number of arguments (ArgumentError)
```

2つ以上の引数を表示したい場合は、[`hello-strinterp.rb`](https://github.com/k-takata/zenn-contents/tree/master/books/d5c77046e634bb/src/hello-strinterp.rb) のように代わりに式展開(string interpolation)が使えます。

```ruby:hello-strinterp.rb
#!mruby

board = "GR-CITRUS"
puts "Hello\r\n#{board}"
```

このコードを実行すると、"Hello" と "GR-CITRUS" が1行ずつ表示されます。

```
>X hello-strinterp.mrb 147

Waiting  60 59
hello-strinterp.mrb(147) Saving..
Hello
GR-CITRUS
```

:::message
Rubyファームウェアの最新ソースコード(2023年8月16日以降)を取得し、自分でコンパイルするとmruby標準の `puts` と同様に2つ以上の引数を渡して表示することができるようになります。
:::


## 他の方法

ここまでコマンドラインベースの使い方を紹介してきましたが、他には[Crione](https://github.com/ogom/crione)というGR-CITRUS向けGUIクライアントがあります。
ビルド済みパッケージはmacOS用しかありませんですが、[Windowsで使えるようにする方法](https://qiita.com/takjn/items/5b5e1f7a690e67871ce8)もあるようです。

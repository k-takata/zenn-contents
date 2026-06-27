---
title: "Vimのインストーラーを作り直してみた"
emoji: "📦"
type: "tech" # tech: 技術記事 / idea: アイデア
topics: ["Vim", "NSIS", "インストーラー"]
published: true
published_at: 2026-06-29 00:00
---

:::message
この記事は[Vim駅伝](https://vim-jp.org/ekiden/)の2026-06-29の記事です。
Vim駅伝は常に参加者を募集しています。詳しくは[こちらのページ](https://vim-jp.org/ekiden/about/)をご覧ください。
:::

## 概要

かつて、[新しいWindows用Vimのインストーラーを作っている話 #Vim - Qiita](https://qiita.com/k-takata/items/343c5ee6abbe34e85571)で書いたようにVimの新しいインストーラーを作ったわけですが、それから8年が経ち、いろいろと不満な点が出てきました。そこで今回、インストーラーを再度作り直すことにしました。


## 機能

今回対応した機能や改善は以下の通りです。

* 全ユーザーに対するインストール (per-machine) に加え、現在のユーザーに対するインストール (per-user) に対応
* フォルダー構成の変更
* コマンドラインサポートの改善
* コマンドラインオプション対応
* winget対応の改善

これらの対応にあたり、インストーラーシステム自体を[NSIS](https://nsis.sourceforge.io/)から他のもの (例えばMSI) に移行することも検討しましたが、今回はそのままNSISを使うこととしました。

これらを以下のプルリクエストで実装しました。
https://github.com/vim/vim-win32-installer/pull/457


### 現在のユーザーに対するインストール

従来のインストーラーは、全ユーザーに対するインストールのみに対応しており、インストールには管理者権限が必須でした。これに対しては以前から何度も指摘があり、これを改善することが今回の最大の目的でした。

今回は、[NsisMultiUser](https://github.com/Drizin/NsisMultiUser)というプラグインを使用して、全ユーザーに対するインストールと現在のユーザーに対するインストールに対応することにしました。これを使うことで次のような動作を行うことができます。

* UACによる昇格をせずに、通常権限でインストーラーを実行開始
* インストーラーの画面上で、全ユーザーに対するインストールか現在のユーザーに対するインストールかを選択
  - 全ユーザーが選択された場合、UACによる昇格後、インストールを続行
    (デフォルトインストール先: `C:\Program Files\Vim`)
  - 現在のユーザーが選択された場合、UACによる昇格をせずにインストールを続行
    (デフォルトインストール先: `C:\Users\<ユーザー名>\AppData\Local\Programs\Vim`)

NSIS本体には、[MultiUser](https://nsis.sourceforge.io/Docs/MultiUser/Readme.html)という類似のプラグインが含まれていますが、このような動的なUAC昇格はできないため、NsisMultiUserを使うこととしました。

実行時の画面は以下のようになっています。インストール対象として全ユーザーを選択したときだけ、「次へ」ボタンの横に盾のアイコンが表示され、UACの昇格が必要になります。

![インストール対象の選択画面](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/vim-rework-installer/select-user.png)

NsisMultiUserには日本語メッセージが含まれていなかったため、PRを送って取り込んでもらいました。

https://github.com/Drizin/NsisMultiUser/pull/35

なお、従来のインストーラーでは、レジストリの更新やショートカットアイコンの登録は `install.exe` を介して行っていましたが、今回は `install.exe` への依存を廃し[^1]、NSIS単体で登録を行うように変更しています。

[^1]: 前回の改修時は[二重管理を避けたい](https://qiita.com/k-takata/items/343c5ee6abbe34e85571#%E5%8F%96%E3%82%8A%E8%BE%BC%E3%81%BE%E3%82%8C%E3%81%AA%E3%81%8B%E3%81%A3%E3%81%9F%E5%8E%9F%E5%9B%A0)という理由で、`install.exe` に依存させていましたが、今回、現在のユーザーに対するインストールの対応とフォルダー構成の変更の影響で `install.exe` の更新が難しくなったため、`install.exe` は更新しないことにしました。


### フォルダー構成の変更

`gvim.exe` を格納するフォルダーを、バージョン番号を含まない名前にしてほしいとの要望があったため、フォルダー構成を変更することにしました。

変更前:
```
C:\Program Files\
└── Vim\                     ($VIM)
    ├── _vimrc
    ├── vim92\               ($VIMRUNTIME)
    │   ├── GvimExt32\
    │   ├── GvimExt64\
    │   ├── autoload\
    │   ├── ...
    │   ├── LICENSE.txt
    │   ├── README.txt
    │   ├── gvim.exe
    │   ├── uninstall-gui.exe
    │   └── vim.exe
    └── vimfiles\
```

変更後:
```
C:\Program Files\
└── Vim\                     ($VIM)
    ├── _vimrc
    ├── lang\
    │   ├── LICENSE.??.txt
    │   └── README.??.txt
    ├── runtime\             ($VIMRUNTIME)
    │   ├── GvimExt32\
    │   ├── GvimExt64\
    │   ├── autoload\
    │   └── ...
    ├── LICENSE.txt
    ├── README.txt
    ├── gvim.exe
    ├── uninstall-gui.exe
    ├── vim.exe
    ├── ...
    └── vimfiles\
```

変更前は、`gvim.exe` は `C:\Program Files\Vim\vim92\` に格納されていましたが、1階層上の `C:\Program Files\Vim\` に格納するように変更し、ランタイムファイルの格納先も `vim92` から `runtime` に変更しました。

元々、バージョン番号がフォルダー名に含まれていたのは、複数の異なるバージョンを同時にインストールできるようにするための仕組みですが、現在のインストーラーはそもそも異なるバージョンを同時にインストールできるようにはなっていません。（古いバージョンはアンインストールされるようになっています。） そのため、ランタイムディレクトリーの名前からバージョン番号を除いても問題ないと判断しました。


#### ZIP形式について

[vim-win32-installer](https://github.com/vim/vim-win32-installer)では、NSISインストーラーとZIP形式の配布を行っていますが、ユーザーへの影響を考慮し、ZIP形式は従来通りのフォルダー構成を維持しています。
ZIP形式を使った場合は、同梱の `install.exe` を使って対話形式でインストールできるようになっていますが、こちらは従来通り管理者権限が必要となっており、現在のユーザーに対するインストールはできません。


### コマンドラインサポートの改善

従来のインストーラーには、コマンドラインからVimを使いやすくするという目的で、`gvim.bat` や `vim.bat` というバッチファイルを `C:\Windows` に作成するという機能がありました。これを使うと、PATHを変更することなく、コマンドラインから `gvim` や `vim` というコマンドが使えるようになります。
ただ、この機能にはバッチファイル特有の使いづらさ[^2]があったため、この機能は廃止し、別の機能を2つ用意することとしました。

[^2]: コマンドラインの解析に癖があるため複雑なオプションをうまく渡せない、バッチファイルから呼び出すときに `call` コマンドを使わないと処理が戻ってこない、GUIアプリから `gvim.bat` を呼び出すとコマンドプロンプトが開いてしまう、など。


#### Vim launcher

1つ目は、Vim launcherという、Vimを起動するための小さな実行ファイルを `C:\Windows` に配置するという機能です。
従来のバッチファイルとほぼ同じ機能ですが、本物の実行ファイルになっているので、バッチファイル特有の罠などはありません。
何らかの理由で、Vimのインストール先をPATHに追加したくない場合は、この方法が使えます。

`C:\Windows` にファイルを配置することから、この機能は全ユーザーに対するインストール時のみ使用可能です。

![Vim launcher](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/vim-rework-installer/launcher.png)


#### PATHへの追加

2つ目は、インストーラーでVimのインストール先をPATHに追加できるようにするというものです。
コマンドラインからVimを呼び出しやすくする方法としては、この方法が最も素直と言えるでしょう。
全ユーザーに対するインストール時にはシステムの環境変数を変更し、現在のユーザーに対するインストール時にはユーザーの環境変数を変更するようになっています。

この機能の実装には、[EnVar plug-in](https://nsis.sourceforge.io/EnVar_plug-in)[^3]を使用しています。

[^3]: NSISの標準機能には、[一定の長さを超える環境変数を扱えないという大問題](https://nsis.sourceforge.io/Setting_Environment_Variables)がありますが、EnVar plug-inを使うことでこの問題を回避できます。


### コマンドラインオプション対応

サイレントインストールでインストールする際に、インストールするコンポーネントを選択できるようにしてほしいという要望があったため、それに対応しました。

インストーラーのコマンドラインオプションとして `/?` を指定して実行すると、以下のようなヘルプが表示されます。

![ヘルプ](https://raw.githubusercontent.com/k-takata/zenn-contents/master/articles/images/vim-rework-installer/help.png)

`/S` と `/D=path` は、NSISインストーラーの標準のオプションですが、`/S` を使ってサイレントインストールを行うには、`/allusers` または `/currentuser` のどちらかのオプションを指定して、全ユーザーに対するインストールか現在のユーザーに対するインストールかを明示する必要があります。

インストールするコンポーネントは、`/コンポーネント名={1 または 0}` で指定します。`1` ならインストールし、`0` ならインストールしません。
例えば、デスクトップにショートカットを作成しないなら `/desktop=0` を指定します。
デフォルトは、`/console=1 /launcher=0 /addpath=0 /desktop=1 /startmenu=1 /editwith=1 /vimrc=1 /pluginhome=1 /pluginvim=0 /nls=1` となっています。

上記ヘルプの最後の3項目は `_vimrc` 作成時の設定です。こちらも `/設定名=選択` で指定します。
デフォルトは、`/compat=all /keymap=default /mouse=default` となっています。

一度インストールしたあと、新しいバージョンを上書きインストールする際には、前回の選択がデフォルト値として引き継がれます。


### winget対応の改善

上記の改善により、winget経由のインストールでも現在のユーザーに対するインストールができるようになりました。
デフォルトは現在のユーザーになっており、`--scope` オプションを使うことで、全ユーザーに対するインストールに切り替えることができます。

例:

```
winget install vim.vim.nightly --scope machine
```

`--custom` オプションのあとに、前節のコマンドラインオプションを指定すれば、インストールするコンポーネントを指定できます。複数のオプションを指定する場合は、全体をダブルクォートで囲みます。
例えば、明示的に現在のユーザーに対してインストールし、デスクトップアイコンは作成せず、Vimのインストール先をPATHに追加するには次のようにします。

```
winget install vim.vim.nightly --scope user --custom "/desktop=0 /addpath=1"
```


## 補足事項

### MSI/MSIX形式

今回の改修に先立ち、[vim-msi](https://github.com/jcasale/vim-msi)で開発されているMSI形式のインストーラーを使えるか試してみました。これは、[WiX Toolset](https://www.firegiant.com/wixtoolset/)を使用してインストーラーを作成するようになっています。

調査の結果、以下のような課題が見つかったため、今回はMSIへの移行は見送りました。

* per-machine と per-user インストールへの対応
  技術的には可能なはずだが、vim-msi自体は現時点では対応していない。
  (AIを使った作りかけブランチ: [dual-purpose](https://github.com/k-takata/vim-msi/tree/dual-purpose))
* 多言語への対応
  メッセージを翻訳することは可能だが、WiX Toolsetは多言語に対応したインストーラーを作成することは現時点ではできない[^4]。各言語に個別対応したインストーラーを作成することになってしまう。(WiXで作成した個別インストーラーを別ツールで統合することは可能？[^5])
  (AIを使った作りかけブランチ: [agents/wix-toolset-multilingual-ui](https://github.com/k-takata/vim-msi/tree/agents/wix-toolset-multilingual-ui))
* 32bit版/64bit版GvimExtの同時インストール
  現状のインストーラーのように、`C:\Program Files` 以下に両方を同時インストールするのは難しそう。`C:\Program Files` と `C:\Program Files (x86)` に分けてインストールするのは可能と思われるが、管理が煩雑。(将来的に32bit版GvimExtのインストールはやめてもよいかもしれないが)

さらに最近はMSIXという新しい形式もありますが、こちらはGvimExtのようなシェルエクステンションのインストールには使えないということですので、こちらも採用は見送りました。

[^4]: [WIP: add support for multilingual MSI packages · Issue #7544 · wixtoolset/issues](https://github.com/wixtoolset/issues/issues/7544)
[^5]: [多言語化されたWindows Installerを 作成する #WiXToolset - Qiita](https://qiita.com/hanashima/items/b2a2c4ac5b65ca6f4544)


### AIレビュー

今回のPRでは、GitHub Copilotによるレビューも活用しています。自分では実装はほぼ完璧だと思った段階でレビューを掛けてみたのですが、14件もの指摘が出ました。

|分類                             |件数|
|---------------------------------|----|
|バグとなりうる重要な指摘         |5件 |
|typoや英文の表記に関する指摘     |4件 |
|今回は必須ではないが採用したもの |4件 |
|不要で採用しなかったもの         |1件 |

重要な指摘が5件もあり、それを含めて13件を採用しました。
ただし、AIの修正提案のうち2件はミスがあり、`endif` が消えてしまっていました。1件はマージ前に気づきましたが、1件は気づかずマージしてしまい、コンパイルが通らずしばらく悩みました。

いくつか問題はあったものの、予想以上に的確な指摘で驚きました。


### アイコンファイルサイズの削減

Vim launcherは、実行ファイルのサイズを極限まで小さくしてみようと思い、Cランタイムを使用せずに作成しています。これにより、実行ファイルのサイズはわずか35KBほどになっています。しかし、実はこのうちアイコンが27KBほどを占めています。

アイコンファイルのサイズを削減できないものかと考え、Copilotを使って、[optico](https://github.com/k-takata/optico)というツールを作成してみました。これにより1KB弱の削減に成功しました。


### リポジトリの移動

従来のインストーラーのソースコードは、[vim/vim](https://github.com/vim/vim)リポジトリで管理されていましたが、今回、[vim/vim-win32-installer](https://github.com/vim/vim-win32-installer)リポジトリに移動しました。これは、インストーラーのソースコードは、インストーラーパッケージ作成のためのCIのコードとまとめて管理した方が手間が少ないだろうという考えからです。


## まとめ

現在のユーザーに対するインストールなどに対応したVimの新インストーラーを紹介しました。
今回の新NSISインストーラーは[v9.2.0699](https://github.com/vim/vim-win32-installer/releases/tag/v9.2.0699)から採用されています。（ただし、コマンドラインオプションのサポートは[v9.2.0735](https://github.com/vim/vim-win32-installer/releases/tag/v9.2.0735)から）

もし何かありましたら、[vim-win32-installer](https://github.com/vim/vim-win32-installer)にissueを立てるなどして連絡をお願いします。

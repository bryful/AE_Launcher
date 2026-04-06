# AE_Launcher

C++で作って軽量・高速のAfter effects バージョン選択ランチャーです。<br>
.aepをダブルクリックすると使用するAfter Effectsのバージョンが選べます。<br>
<br>
[https://github.com/bryful/AepSelector](https://github.com/bryful/AepSelector) と同じものです。<br>
こちらはC++でコーディングされていて軽量・高速です。<br>


3種類ありますが、基本的に同じものです。

# 使い方
拡張子登録を行う。<br>

1. 適当な場所にAE_Launcher.exeをおく(C:\bin\AE_Launcher\AE_Launcher.exe を推奨)
2. 拡張子.aepのファイルを右クリックしてプロパティ。
3. プログラム：変更ボタンでAE_Launcher.exeを選択（PCでアプリを選択する）
4. 適用ボタンで終了

これで使えるはずです。おまけでコンソールで

```
AE_Launcher -inst
```
でも拡張子登録できます。<br>
```
AE_Launcher -cs
```
でデスクトップにショートカットを作成できます。<br>
一応**AE_Launcher.cmd**をダブルクリックで上記のコマンドを実行できます。

# 3パターンあります

## Imgui版
最初に作ったものです。実際にはwin32Api+Imguiになります。
起動は早いですが、コードがやや複雑。あとUIデザインがほぼ選べませんがありとあらゆる物がそろってるのでその点は便利そう。

## DxLib版
DxLibはボタンが作れないので適当にpngを表示させてます。
DxLibは起動がめちゃ遅いです。ウィンドウが開いてからUIが表示されるのにかなり時間かかります。
このアプリでは最初visibleをOFFにしてUIの初期化が終わってから表示させてます。

## win32api版
参考用にwin32apiを作りました。流石に容量が一番小さいです。動作も一番早いです。
ライブラリがないのでその初期化分だけ起動が速いです。意外とコードがわかりやすい。コード量は一番多いですが。
多分使うにはこれが一番かも。

# 使用ライブラリ

[Dear ImGui](https://github.com/ocornut/imgui)を使用しています。<br>
[DxLib](https://dxlib.xsrv.jp/)を使用しています。<br>


## Dependency
Visual studio 2026 C#<br>
Gemini + Github copilot<br>

## License
This software is released under the MIT License, see LICENSE

## Authors

bry-ful(Hiroshi Furuhashi)<br>
twitter:[bryful](https://twitter.com/bryful)<br>


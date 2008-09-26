README-jis.txt

Error-correcing Multicast on Open Nodes (EMON)とは，各種パーツ(プロセス)
を組み合わせることで簡単にエラー訂正付きの映像・音声が，ユニキャスト・マ
ルチキャストで送信できるシステムです．

============================================================
コンパイル方法と単純な利用方法の説明は
この文章(README-jis.txt) にあります。

インターネット電話(NOTASIP方式)等の応用例の説明は
test/README-jis.txt にあります。

各パーツ(各プログラム)の説明は 
doc/specification.txt にあります。

サウンドカードの動作確認情報は
doc/dspcap.txt にあります。
============================================================

0. 準備

   コンパイル方法
	% tar zxvf emon-20020219.tar.gz
	% cd emon-20020219/src
	% ./configure
	% make depend
	% make

   注意:
	開発中で動作未確認のプログラムも含まれています．
	動作可:
		jpegcapt, fecenc, rtpenc, rtpsend, 
		jpegplay, fecdec, rtpdec, rtprecv,
		audiocapt, audiocaptplay, audioplay,
		filerepeat, pipesave, pipeerr,
	動作未確認:
		udpsend, udprecv

   その他の情報
	動作確認済み OS
		FreeBSD 4.3R / FreeBSD 4.2R

	必要なライブラリ
		libjpeg
		SDL-1.1.6 以上 (http://www.libsdl.org/)

	サンプルの動画像ファイルの置き場所
		http://www.ibcast.net/emon/sample.jpgs (約7MB)

	映像の取り込みに必要なハードウェア
		/dev/bktr0 で扱えるキャプチャカード
	音声の取り込みに必要なハードウェア
		/dev/dsp0 で扱えるサウンドカード


1．ファイルから映像を表示する

   サンプルの動画像ファイル(sample.jpgs)を再生するには
   下の例のように，jpegplay の標準入力にデータを読み込ませて下さい．

   % jpegplay < sample.jpgs

   繰り返して再生させたい場合は，filerepeat を使って

   % filerepeat sample.jpgs | jpegplay

   とします．

   httpで受信しながら再生する場合は,
   curl(http://curl.sourceforge.net/) あるいは
   fetch(FreeBSD 4.1以降) を使って

   % curl http://www.ibcast.net/emon/sample.jpgs | jpegplay
   % fetch -o - http://www.ibcast.net/emon/sample.jpgs | jpegplay

   とします. この時に画像を保存しながら再生する場合は、

   % curl http://..../sample.jpgs | tee sample.jpgs | jpegplay
   % fetch -o - http://..../sample.jpgs | tee sample.jpgs | jpegplay

   とします.

2．終了するには？

   Ctrl + C で終了するはずですが，終了できない場合があります．
   その場合は Ctrl + \ を押してみて下さい．
   これでも終了できなければ，kill でプロセスを止めて下さい．


3．キャプチャするには

   /dev/bktr0 が使えるようになっている環境で，下の例のようにすると
   映像をキャプチャしながら表示します．

   % jpegcapt | jpegplay

   キャプチャカードに複数の映像入力ラインが有る場合，
   どのラインから映像を取り込むかは，-l オプションで指定できます．
   -l の後ろには数字(0-5)か 'C' か 'S' を指定できます．
   C, S はそれぞれ Composite, S-Video のつもりですが，
   全てのデバイスで期待通りに動作するかは確認していません．
   C, S で正しく指定できない場合は 0〜5 の数字を使って下さい．

   注意:
   キャプチャしながら再生を行うと，マシンパワーが足りなくて，
   コマ落ちすることがあります．


4．キャプチャしたデータをファイルに保存

   キャプチャした映像データをファイルに保存しておくには，
   下の例のように jpegcapt の標準出力をそのままファイルに落として下さい．

   % jpegcapt > data.jpgs

   補足1:
   出来たファイルは，1．の説明に使ったサンプルファイルと同じフォーマットに
   なっているので，1．で説明した方法…
   % jpegplay < data.jpgs
   …で，再生できます．

   補足2:
   % jpegcapt
   のように標準出力を端末(tty)に向けたまま実行した場合は jpegcapt は動作
   しません．画面にゴミをまき散らさないためです．
   他のプログラムも同様に，標準出力を端末(tty)に向けたままでは，動作しな
   いものがあります．



5．FEC 付きデータを扱うには

  キャプチャしたデータに FEC (Forward Error Correction) 符号を付けるには
  fecenc，FEC 付きのデータから元のデータを取り出すには fecdec を使います．
  使い方の例は下のようになります．

  % jpegcapt | fecenc > data.fec
  % fecdec < data.fec | jpegplay

  上記二つの行を一度に処理して

  % jpegcapt | fecenc | fecdec | jpegplay

  ということも可能です．


6．ネットワークに流すには

   データをネットワークに流すには rtpsend，受信するには rtprecv を使います．
   送受信する IP アドレスは -A，ポート番号は -P で指定します．
   ユニキャストアドレスはもちろん，マルチキャストアドレスを
   指定しても動作します．

   送信側の例
   % jpegcapt | fecenc | rtpsend -A224.x.y.z -P10000

   受信側の例
   % rtprecv -A224.x.y.z -P10000 | fecdec | jpegplay



7．キャプチャや再生速度を変更する

   速度を制御する数値は，clock と freq の二つがあり，
   (clock / freq) が「1秒あたりのフレーム数」に相当します．
   初期値は clock = 44100, freq = 1470 で，clock / freq = 30 です．
   clock = 30, freq = 1 としても，同様の速度で動作します．

   clock や freq の値を指定する方法は，
    A. 環境変数で指定する方法
    B. 起動時のオプションの指定する方法
   の二つがあります．

   A. の場合は，EMON_CLOCK, EMON_FREQ という環境変数に数値を設定して下さい．
   例:(秒間10フレーム)

	% export EMON_CLOCK=10
	% export EMON_FREQ=1
	% jpegcapt | jpegplay

   B. の場合は，プログラム起動時に -C, -F というオプションで指定して下さい．
   例:(秒間10フレーム)

	% jpegcapt -C10 -F1 | jpegplay -C10 -F1



8．最適化・高速化について

    カーネルについて
    FreeBSD の GENERIC カーネルでは，20msec ごとにしかプロセス切り替えを
    してくれないようです．1/30sec = 33.3..msec ごとに画像のキャプチャや
    表示をするには，これでは無理があります．
    プロセス切り替えのタイミングをより正確にするためには，カーネルの 
    config ファイルに，

    options HZ=1000

    のような設定を追加して，カーネルをコンパイルし直して下さい．
    これで，1/1000 = 1msec ごとに切り替えられるはずです．


    libjpeg について
    MMX 命令用に書かれた libjpeg を使うことで JPEG のデコード処理が高速
    化されます．
    http://sourceforge.net/projects/mjpeg/


    コンパイラについて
    Pentium専用コンパイラ pgcc を用いることで，全体の処理が高速化されま
    す．


9．今後の展望

    音声と映像の同期再生

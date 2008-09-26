test/README-jis.txt

この文章はEMONを用いた
インターネット電話(NOTASIP方式)等の応用例を説明します。


この文書で説明する応用例の一覧です。
(カッコの中は test/ にあるShell-script名)

 1. オーディオディバイステスト
	ループバック		(audioloopback)
	DTMF			(dtmfpad)
        ループバック+DTMF	(dtmftest)

 2. RTPマイク			(rtpmic)
 3. RTPスピーカ			(rtpspkr)

 4. NOTASIP電話
	発信			(notasipcall)
	発信(DTMF有り)		(notasipcall2)
	着信			(notasipwait)

これらを利用する前に、
あらかじめ ../src で EMONのプログラムをコンパイルしておいて下さい。

以下で各スクリプトの説明をします。
各スクリプトを実行すると、
"= pipeline start ="
と
"= pipeline end ="
の間にEMONのプログラムをどのように組み合わせているのかが表示されます。


1. オーディオディバイステスト

  audioloopback は、
  /dev/dsp で音声を録音し、すぐに再生します。
  サウンドカードがFull-duplexに対応しているのか?
  指定のサウンドフォーマットに対応しているのか?
  といったことを調べることができます。

  % audioloopback -h 

  と実行すると、ヘルプが表示されます。

  % audioloopback -r /dev/dsp0 -p /dev/dsp1 -rate 44100 -c pcmu

  と実行すると、
  /dev/dsp0 で音声を44.1KHz16bit1chで録音しmu-Lawでエンコードします。
  そして、mu-Lawでデコードし、/dev/dsp1 で再生します。
  再生ディバイス(-r) と録音ディバイス(-p)は同じでも構いませんが、
  指定したディバイスがFull-duplexに対応している必要があります。

  注意:
	Full-duplex対応のサウンドカードでも
	ドライバがFull-duplexに対応していない場合があります。
	例えば、/dev/dsp0 がHalf-duplexにしか対応していないのに
	/dev/dsp1 を利用できない場合があります。

	ドライバによっては、44.1KHz16bitの再生以外は
	不具合が生じる場合があります。
	例えば、遅延が大きくなる、録音ができない、
	モノラル再生ができない、などの不具合が生じます。

	ドライバが提供するサウンドカードの情報は
	
	% cat /dev/sndstat

	によって表示されます。

	Half-duplexにしか対応していないサウンドカードの
 	場合でも、サウンドカードを2枚差すことで実行できます。
	

  dtmfpad は、
  test/dtmf にある "0"〜"1"と"#"と"*"の8kHz16bit1chでサンプリングした
  DTMF音を標準出力に出力します。

  % dtmppad > /dev/dspW

  と実行すると、入力待ち受け状態になり、

  0120-12345678

  と入力すると、DTMF音が/dev/dspWに出力されます。
  
  注意:
	DTMF音が鳴りはじめるまでに1秒程度かかる場合、
	そのディバイスを用いて電話をすると遅延が大きくなる可能性があります。

	/dev/dspWは8kHz16bit1chの音声を再生できる必要があります。


  dtmftestは
  audioloopback と dtmfpadを同時に実行したように動作します。
  つまり、数字の入力が無い時は audioloopback の動作をし、
  数字の入力があった時だけ dtmfpadの動作をします。  


2. RTPマイク
  rtpmicは
  /dev/dspから音声を録音し、RTPで指定のホストへ送信します。
  
  % rtpmic

  と実行すると、送信先のホストの指定を求められます。


3. RTPスピーカ
  rtpspkrは
  指定のホストでRTPの音声ストリームを受信し、/dev/dspで再生します。

  % rtpspkr
  
  と実行すると、受信するホストの指定を求められます。


4. NOTASIP電話
  notasipcallは
  NOTASIP電話に発信します。

  % notasipcall -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  と実行すると、アドレス:133.x.y.z、UDPポート:10000のNOTASIP電話へ発信します。
  電話セッションが確立されると、
  /dev/dsp0 で録音し、pcmuでエンコードした音声を通話相手に送信します。
  通話相手から受信した音声データはpcmuでデコードし、
  /dev/dsp1 で再生します。

  % notasipcall -r rtp://10.0.0.1:10002 -p rtp://10.0.0.2:10002 \
     -c pcmu 133.x.y.z 10000

  と実行すると、
  10.0.0.1へ音声を送信する"2.RTPマイク"をマイクとして利用し、
  10.0.0.2で音声を受信する"3.RTPスピーカ"をスピーカとして利用して
  アドレス:133.x.y.z、UDPポート:10000のNOTASIP電話へ発信します。


  notasipcall の代わりに notasipcall2を用いると、
  数字を入力することで DTMF 音を送信できます。
  notasipcall2は "rtp://" に対応していません。
  

  notasipwaitは
  NOTASIP電話の着信待ちをします。

  % notasipwait -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  と実行すると アドレス:133.x.y.z、UDPポート:10000のNOTASIP電話
  として着信待ちをします。
  電話セッションが確立されると、
  /dev/dsp0 で録音し、pcmuでエンコードした音声を通話相手に送信します。
  通話相手から受信した音声データはpcmuでデコードし、
  /dev/dsp1 で再生します。
  この電話への発信は

  % notasipcall -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  等と実行してください。


以上です。

test/README-jis.txt

$B$3$NJ8>O$O(BEMON$B$rMQ$$$?(B
$B%$%s%?!<%M%C%HEEOC(B(NOTASIP$BJ}<0(B)$BEy$N1~MQNc$r@bL@$7$^$9!#(B


$B$3$NJ8=q$G@bL@$9$k1~MQNc$N0lMw$G$9!#(B
($B%+%C%3$NCf$O(B test/ $B$K$"$k(BShell-script$BL>(B)

 1. $B%*!<%G%#%*%G%#%P%$%9%F%9%H(B
	$B%k!<%W%P%C%/(B		(audioloopback)
	DTMF			(dtmfpad)
        $B%k!<%W%P%C%/(B+DTMF	(dtmftest)

 2. RTP$B%^%$%/(B			(rtpmic)
 3. RTP$B%9%T!<%+(B			(rtpspkr)

 4. NOTASIP$BEEOC(B
	$BH/?.(B			(notasipcall)
	$BH/?.(B(DTMF$BM-$j(B)		(notasipcall2)
	$BCe?.(B			(notasipwait)

$B$3$l$i$rMxMQ$9$kA0$K!"(B
$B$"$i$+$8$a(B ../src $B$G(B EMON$B$N%W%m%0%i%`$r%3%s%Q%$%k$7$F$*$$$F2<$5$$!#(B

$B0J2<$G3F%9%/%j%W%H$N@bL@$r$7$^$9!#(B
$B3F%9%/%j%W%H$r<B9T$9$k$H!"(B
"= pipeline start ="
$B$H(B
"= pipeline end ="
$B$N4V$K(BEMON$B$N%W%m%0%i%`$r$I$N$h$&$KAH$_9g$o$;$F$$$k$N$+$,I=<($5$l$^$9!#(B


1. $B%*!<%G%#%*%G%#%P%$%9%F%9%H(B

  audioloopback $B$O!"(B
  /dev/dsp $B$G2;@<$rO?2;$7!"$9$0$K:F@8$7$^$9!#(B
  $B%5%&%s%I%+!<%I$,(BFull-duplex$B$KBP1~$7$F$$$k$N$+(B?
  $B;XDj$N%5%&%s%I%U%)!<%^%C%H$KBP1~$7$F$$$k$N$+(B?
  $B$H$$$C$?$3$H$rD4$Y$k$3$H$,$G$-$^$9!#(B

  % audioloopback -h 

  $B$H<B9T$9$k$H!"%X%k%W$,I=<($5$l$^$9!#(B

  % audioloopback -r /dev/dsp0 -p /dev/dsp1 -rate 44100 -c pcmu

  $B$H<B9T$9$k$H!"(B
  /dev/dsp0 $B$G2;@<$r(B44.1KHz16bit1ch$B$GO?2;$7(Bmu-Law$B$G%(%s%3!<%I$7$^$9!#(B
  $B$=$7$F!"(Bmu-Law$B$G%G%3!<%I$7!"(B/dev/dsp1 $B$G:F@8$7$^$9!#(B
  $B:F@8%G%#%P%$%9(B(-r) $B$HO?2;%G%#%P%$%9(B(-p)$B$OF1$8$G$b9=$$$^$;$s$,!"(B
  $B;XDj$7$?%G%#%P%$%9$,(BFull-duplex$B$KBP1~$7$F$$$kI,MW$,$"$j$^$9!#(B

  $BCm0U(B:
	Full-duplex$BBP1~$N%5%&%s%I%+!<%I$G$b(B
	$B%I%i%$%P$,(BFull-duplex$B$KBP1~$7$F$$$J$$>l9g$,$"$j$^$9!#(B
	$BNc$($P!"(B/dev/dsp0 $B$,(BHalf-duplex$B$K$7$+BP1~$7$F$$$J$$$N$K(B
	/dev/dsp1 $B$rMxMQ$G$-$J$$>l9g$,$"$j$^$9!#(B

	$B%I%i%$%P$K$h$C$F$O!"(B44.1KHz16bit$B$N:F@80J30$O(B
	$BIT6q9g$,@8$8$k>l9g$,$"$j$^$9!#(B
	$BNc$($P!"CY1d$,Bg$-$/$J$k!"O?2;$,$G$-$J$$!"(B
	$B%b%N%i%k:F@8$,$G$-$J$$!"$J$I$NIT6q9g$,@8$8$^$9!#(B

	$B%I%i%$%P$,Ds6!$9$k%5%&%s%I%+!<%I$N>pJs$O(B
	
	% cat /dev/sndstat

	$B$K$h$C$FI=<($5$l$^$9!#(B

	Half-duplex$B$K$7$+BP1~$7$F$$$J$$%5%&%s%I%+!<%I$N(B
 	$B>l9g$G$b!"%5%&%s%I%+!<%I$r(B2$BKg:9$9$3$H$G<B9T$G$-$^$9!#(B
	

  dtmfpad $B$O!"(B
  test/dtmf $B$K$"$k(B "0"$B!A(B"1"$B$H(B"#"$B$H(B"*"$B$N(B8kHz16bit1ch$B$G%5%s%W%j%s%0$7$?(B
  DTMF$B2;$rI8=`=PNO$K=PNO$7$^$9!#(B

  % dtmppad > /dev/dspW

  $B$H<B9T$9$k$H!"F~NOBT$A<u$1>uBV$K$J$j!"(B

  0120-12345678

  $B$HF~NO$9$k$H!"(BDTMF$B2;$,(B/dev/dspW$B$K=PNO$5$l$^$9!#(B
  
  $BCm0U(B:
	DTMF$B2;$,LD$j$O$8$a$k$^$G$K(B1$BICDxEY$+$+$k>l9g!"(B
	$B$=$N%G%#%P%$%9$rMQ$$$FEEOC$r$9$k$HCY1d$,Bg$-$/$J$k2DG=@-$,$"$j$^$9!#(B

	/dev/dspW$B$O(B8kHz16bit1ch$B$N2;@<$r:F@8$G$-$kI,MW$,$"$j$^$9!#(B


  dtmftest$B$O(B
  audioloopback $B$H(B dtmfpad$B$rF1;~$K<B9T$7$?$h$&$KF0:n$7$^$9!#(B
  $B$D$^$j!"?t;z$NF~NO$,L5$$;~$O(B audioloopback $B$NF0:n$r$7!"(B
  $B?t;z$NF~NO$,$"$C$?;~$@$1(B dtmfpad$B$NF0:n$r$7$^$9!#(B  


2. RTP$B%^%$%/(B
  rtpmic$B$O(B
  /dev/dsp$B$+$i2;@<$rO?2;$7!"(BRTP$B$G;XDj$N%[%9%H$XAw?.$7$^$9!#(B
  
  % rtpmic

  $B$H<B9T$9$k$H!"Aw?.@h$N%[%9%H$N;XDj$r5a$a$i$l$^$9!#(B


3. RTP$B%9%T!<%+(B
  rtpspkr$B$O(B
  $B;XDj$N%[%9%H$G(BRTP$B$N2;@<%9%H%j!<%`$r<u?.$7!"(B/dev/dsp$B$G:F@8$7$^$9!#(B

  % rtpspkr
  
  $B$H<B9T$9$k$H!"<u?.$9$k%[%9%H$N;XDj$r5a$a$i$l$^$9!#(B


4. NOTASIP$BEEOC(B
  notasipcall$B$O(B
  NOTASIP$BEEOC$KH/?.$7$^$9!#(B

  % notasipcall -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  $B$H<B9T$9$k$H!"%"%I%l%9(B:133.x.y.z$B!"(BUDP$B%]!<%H(B:10000$B$N(BNOTASIP$BEEOC$XH/?.$7$^$9!#(B
  $BEEOC%;%C%7%g%s$,3NN)$5$l$k$H!"(B
  /dev/dsp0 $B$GO?2;$7!"(Bpcmu$B$G%(%s%3!<%I$7$?2;@<$rDLOCAj<j$KAw?.$7$^$9!#(B
  $BDLOCAj<j$+$i<u?.$7$?2;@<%G!<%?$O(Bpcmu$B$G%G%3!<%I$7!"(B
  /dev/dsp1 $B$G:F@8$7$^$9!#(B

  % notasipcall -r rtp://10.0.0.1:10002 -p rtp://10.0.0.2:10002 \
     -c pcmu 133.x.y.z 10000

  $B$H<B9T$9$k$H!"(B
  10.0.0.1$B$X2;@<$rAw?.$9$k(B"2.RTP$B%^%$%/(B"$B$r%^%$%/$H$7$FMxMQ$7!"(B
  10.0.0.2$B$G2;@<$r<u?.$9$k(B"3.RTP$B%9%T!<%+(B"$B$r%9%T!<%+$H$7$FMxMQ$7$F(B
  $B%"%I%l%9(B:133.x.y.z$B!"(BUDP$B%]!<%H(B:10000$B$N(BNOTASIP$BEEOC$XH/?.$7$^$9!#(B


  notasipcall $B$NBe$o$j$K(B notasipcall2$B$rMQ$$$k$H!"(B
  $B?t;z$rF~NO$9$k$3$H$G(B DTMF $B2;$rAw?.$G$-$^$9!#(B
  notasipcall2$B$O(B "rtp://" $B$KBP1~$7$F$$$^$;$s!#(B
  

  notasipwait$B$O(B
  NOTASIP$BEEOC$NCe?.BT$A$r$7$^$9!#(B

  % notasipwait -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  $B$H<B9T$9$k$H(B $B%"%I%l%9(B:133.x.y.z$B!"(BUDP$B%]!<%H(B:10000$B$N(BNOTASIP$BEEOC(B
  $B$H$7$FCe?.BT$A$r$7$^$9!#(B
  $BEEOC%;%C%7%g%s$,3NN)$5$l$k$H!"(B
  /dev/dsp0 $B$GO?2;$7!"(Bpcmu$B$G%(%s%3!<%I$7$?2;@<$rDLOCAj<j$KAw?.$7$^$9!#(B
  $BDLOCAj<j$+$i<u?.$7$?2;@<%G!<%?$O(Bpcmu$B$G%G%3!<%I$7!"(B
  /dev/dsp1 $B$G:F@8$7$^$9!#(B
  $B$3$NEEOC$X$NH/?.$O(B

  % notasipcall -r /dev/dsp0 -p /dev/dsp1 -c pcmu 133.x.y.z 10000

  $BEy$H<B9T$7$F$/$@$5$$!#(B


$B0J>e$G$9!#(B

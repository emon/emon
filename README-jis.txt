README-jis.txt

Error-correcing Multicast on Open Nodes (EMON)$B$H$O!$3F<o%Q!<%D(B($B%W%m%;%9(B)
$B$rAH$_9g$o$;$k$3$H$G4JC1$K%(%i!<D{@5IU$-$N1GA|!&2;@<$,!$%f%K%-%c%9%H!&%^(B
$B%k%A%-%c%9%H$GAw?.$G$-$k%7%9%F%`$G$9!%(B

============================================================
$B%3%s%Q%$%kJ}K!$HC1=c$JMxMQJ}K!$N@bL@$O(B
$B$3$NJ8>O(B(README-jis.txt) $B$K$"$j$^$9!#(B

$B%$%s%?!<%M%C%HEEOC(B(NOTASIP$BJ}<0(B)$BEy$N1~MQNc$N@bL@$O(B
test/README-jis.txt $B$K$"$j$^$9!#(B

$B3F%Q!<%D(B($B3F%W%m%0%i%`(B)$B$N@bL@$O(B 
doc/specification.txt $B$K$"$j$^$9!#(B

$B%5%&%s%I%+!<%I$NF0:n3NG'>pJs$O(B
doc/dspcap.txt $B$K$"$j$^$9!#(B
============================================================

0. $B=`Hw(B

   $B%3%s%Q%$%kJ}K!(B
	% tar zxvf emon-20020219.tar.gz
	% cd emon-20020219/src
	% ./configure
	% make depend
	% make

   $BCm0U(B:
	$B3+H/Cf$GF0:nL$3NG'$N%W%m%0%i%`$b4^$^$l$F$$$^$9!%(B
	$BF0:n2D(B:
		jpegcapt, fecenc, rtpenc, rtpsend, 
		jpegplay, fecdec, rtpdec, rtprecv,
		audiocapt, audiocaptplay, audioplay,
		filerepeat, pipesave, pipeerr,
	$BF0:nL$3NG'(B:
		udpsend, udprecv

   $B$=$NB>$N>pJs(B
	$BF0:n3NG':Q$_(B OS
		FreeBSD 4.3R / FreeBSD 4.2R

	$BI,MW$J%i%$%V%i%j(B
		libjpeg
		SDL-1.1.6 $B0J>e(B (http://www.libsdl.org/)

	$B%5%s%W%k$NF02hA|%U%!%$%k$NCV$->l=j(B
		http://www.ibcast.net/emon/sample.jpgs ($BLs(B7MB)

	$B1GA|$N<h$j9~$_$KI,MW$J%O!<%I%&%'%"(B
		/dev/bktr0 $B$G07$($k%-%c%W%A%c%+!<%I(B
	$B2;@<$N<h$j9~$_$KI,MW$J%O!<%I%&%'%"(B
		/dev/dsp0 $B$G07$($k%5%&%s%I%+!<%I(B


1$B!%%U%!%$%k$+$i1GA|$rI=<($9$k(B

   $B%5%s%W%k$NF02hA|%U%!%$%k(B(sample.jpgs)$B$r:F@8$9$k$K$O(B
   $B2<$NNc$N$h$&$K!$(Bjpegplay $B$NI8=`F~NO$K%G!<%?$rFI$_9~$^$;$F2<$5$$!%(B

   % jpegplay < sample.jpgs

   $B7+$jJV$7$F:F@8$5$;$?$$>l9g$O!$(Bfilerepeat $B$r;H$C$F(B

   % filerepeat sample.jpgs | jpegplay

   $B$H$7$^$9!%(B

   http$B$G<u?.$7$J$,$i:F@8$9$k>l9g$O(B,
   curl(http://curl.sourceforge.net/) $B$"$k$$$O(B
   fetch(FreeBSD 4.1$B0J9_(B) $B$r;H$C$F(B

   % curl http://www.ibcast.net/emon/sample.jpgs | jpegplay
   % fetch -o - http://www.ibcast.net/emon/sample.jpgs | jpegplay

   $B$H$7$^$9(B. $B$3$N;~$K2hA|$rJ]B8$7$J$,$i:F@8$9$k>l9g$O!"(B

   % curl http://..../sample.jpgs | tee sample.jpgs | jpegplay
   % fetch -o - http://..../sample.jpgs | tee sample.jpgs | jpegplay

   $B$H$7$^$9(B.

2$B!%=*N;$9$k$K$O!)(B

   Ctrl + C $B$G=*N;$9$k$O$:$G$9$,!$=*N;$G$-$J$$>l9g$,$"$j$^$9!%(B
   $B$=$N>l9g$O(B Ctrl + \ $B$r2!$7$F$_$F2<$5$$!%(B
   $B$3$l$G$b=*N;$G$-$J$1$l$P!$(Bkill $B$G%W%m%;%9$r;_$a$F2<$5$$!%(B


3$B!%%-%c%W%A%c$9$k$K$O(B

   /dev/bktr0 $B$,;H$($k$h$&$K$J$C$F$$$k4D6-$G!$2<$NNc$N$h$&$K$9$k$H(B
   $B1GA|$r%-%c%W%A%c$7$J$,$iI=<($7$^$9!%(B

   % jpegcapt | jpegplay

   $B%-%c%W%A%c%+!<%I$KJ#?t$N1GA|F~NO%i%$%s$,M-$k>l9g!$(B
   $B$I$N%i%$%s$+$i1GA|$r<h$j9~$`$+$O!$(B-l $B%*%W%7%g%s$G;XDj$G$-$^$9!%(B
   -l $B$N8e$m$K$O?t;z(B(0-5)$B$+(B 'C' $B$+(B 'S' $B$r;XDj$G$-$^$9!%(B
   C, S $B$O$=$l$>$l(B Composite, S-Video $B$N$D$b$j$G$9$,!$(B
   $BA4$F$N%G%P%$%9$G4|BTDL$j$KF0:n$9$k$+$O3NG'$7$F$$$^$;$s!%(B
   C, S $B$G@5$7$/;XDj$G$-$J$$>l9g$O(B 0$B!A(B5 $B$N?t;z$r;H$C$F2<$5$$!%(B

   $BCm0U(B:
   $B%-%c%W%A%c$7$J$,$i:F@8$r9T$&$H!$%^%7%s%Q%o!<$,B-$j$J$/$F!$(B
   $B%3%^Mn$A$9$k$3$H$,$"$j$^$9!%(B


4$B!%%-%c%W%A%c$7$?%G!<%?$r%U%!%$%k$KJ]B8(B

   $B%-%c%W%A%c$7$?1GA|%G!<%?$r%U%!%$%k$KJ]B8$7$F$*$/$K$O!$(B
   $B2<$NNc$N$h$&$K(B jpegcapt $B$NI8=`=PNO$r$=$N$^$^%U%!%$%k$KMn$H$7$F2<$5$$!%(B

   % jpegcapt > data.jpgs

   $BJdB-(B1:
   $B=PMh$?%U%!%$%k$O!$(B1$B!%$N@bL@$K;H$C$?%5%s%W%k%U%!%$%k$HF1$8%U%)!<%^%C%H$K(B
   $B$J$C$F$$$k$N$G!$(B1$B!%$G@bL@$7$?J}K!!D(B
   % jpegplay < data.jpgs
   $B!D$G!$:F@8$G$-$^$9!%(B

   $BJdB-(B2:
   % jpegcapt
   $B$N$h$&$KI8=`=PNO$rC<Kv(B(tty)$B$K8~$1$?$^$^<B9T$7$?>l9g$O(B jpegcapt $B$OF0:n(B
   $B$7$^$;$s!%2hLL$K%4%_$r$^$-;6$i$5$J$$$?$a$G$9!%(B
   $BB>$N%W%m%0%i%`$bF1MM$K!$I8=`=PNO$rC<Kv(B(tty)$B$K8~$1$?$^$^$G$O!$F0:n$7$J(B
   $B$$$b$N$,$"$j$^$9!%(B



5$B!%(BFEC $BIU$-%G!<%?$r07$&$K$O(B

  $B%-%c%W%A%c$7$?%G!<%?$K(B FEC (Forward Error Correction) $BId9f$rIU$1$k$K$O(B
  fecenc$B!$(BFEC $BIU$-$N%G!<%?$+$i85$N%G!<%?$r<h$j=P$9$K$O(B fecdec $B$r;H$$$^$9!%(B
  $B;H$$J}$NNc$O2<$N$h$&$K$J$j$^$9!%(B

  % jpegcapt | fecenc > data.fec
  % fecdec < data.fec | jpegplay

  $B>e5-Fs$D$N9T$r0lEY$K=hM}$7$F(B

  % jpegcapt | fecenc | fecdec | jpegplay

  $B$H$$$&$3$H$b2DG=$G$9!%(B


6$B!%%M%C%H%o!<%/$KN.$9$K$O(B

   $B%G!<%?$r%M%C%H%o!<%/$KN.$9$K$O(B rtpsend$B!$<u?.$9$k$K$O(B rtprecv $B$r;H$$$^$9!%(B
   $BAw<u?.$9$k(B IP $B%"%I%l%9$O(B -A$B!$%]!<%HHV9f$O(B -P $B$G;XDj$7$^$9!%(B
   $B%f%K%-%c%9%H%"%I%l%9$O$b$A$m$s!$%^%k%A%-%c%9%H%"%I%l%9$r(B
   $B;XDj$7$F$bF0:n$7$^$9!%(B

   $BAw?.B&$NNc(B
   % jpegcapt | fecenc | rtpsend -A224.x.y.z -P10000

   $B<u?.B&$NNc(B
   % rtprecv -A224.x.y.z -P10000 | fecdec | jpegplay



7$B!%%-%c%W%A%c$d:F@8B.EY$rJQ99$9$k(B

   $BB.EY$r@)8f$9$k?tCM$O!$(Bclock $B$H(B freq $B$NFs$D$,$"$j!$(B
   (clock / freq) $B$,!V(B1$BIC$"$?$j$N%U%l!<%`?t!W$KAjEv$7$^$9!%(B
   $B=i4|CM$O(B clock = 44100, freq = 1470 $B$G!$(Bclock / freq = 30 $B$G$9!%(B
   clock = 30, freq = 1 $B$H$7$F$b!$F1MM$NB.EY$GF0:n$7$^$9!%(B

   clock $B$d(B freq $B$NCM$r;XDj$9$kJ}K!$O!$(B
    A. $B4D6-JQ?t$G;XDj$9$kJ}K!(B
    B. $B5/F0;~$N%*%W%7%g%s$N;XDj$9$kJ}K!(B
   $B$NFs$D$,$"$j$^$9!%(B

   A. $B$N>l9g$O!$(BEMON_CLOCK, EMON_FREQ $B$H$$$&4D6-JQ?t$K?tCM$r@_Dj$7$F2<$5$$!%(B
   $BNc(B:($BIC4V(B10$B%U%l!<%`(B)

	% export EMON_CLOCK=10
	% export EMON_FREQ=1
	% jpegcapt | jpegplay

   B. $B$N>l9g$O!$%W%m%0%i%`5/F0;~$K(B -C, -F $B$H$$$&%*%W%7%g%s$G;XDj$7$F2<$5$$!%(B
   $BNc(B:($BIC4V(B10$B%U%l!<%`(B)

	% jpegcapt -C10 -F1 | jpegplay -C10 -F1



8$B!%:GE,2=!&9bB.2=$K$D$$$F(B

    $B%+!<%M%k$K$D$$$F(B
    FreeBSD $B$N(B GENERIC $B%+!<%M%k$G$O!$(B20msec $B$4$H$K$7$+%W%m%;%9@Z$jBX$($r(B
    $B$7$F$/$l$J$$$h$&$G$9!%(B1/30sec = 33.3..msec $B$4$H$K2hA|$N%-%c%W%A%c$d(B
    $BI=<($r$9$k$K$O!$$3$l$G$OL5M}$,$"$j$^$9!%(B
    $B%W%m%;%9@Z$jBX$($N%?%$%_%s%0$r$h$j@53N$K$9$k$?$a$K$O!$%+!<%M%k$N(B 
    config $B%U%!%$%k$K!$(B

    options HZ=1000

    $B$N$h$&$J@_Dj$rDI2C$7$F!$%+!<%M%k$r%3%s%Q%$%k$7D>$7$F2<$5$$!%(B
    $B$3$l$G!$(B1/1000 = 1msec $B$4$H$K@Z$jBX$($i$l$k$O$:$G$9!%(B


    libjpeg $B$K$D$$$F(B
    MMX $BL?NaMQ$K=q$+$l$?(B libjpeg $B$r;H$&$3$H$G(B JPEG $B$N%G%3!<%I=hM}$,9bB.(B
    $B2=$5$l$^$9!%(B
    http://sourceforge.net/projects/mjpeg/


    $B%3%s%Q%$%i$K$D$$$F(B
    Pentium$B@lMQ%3%s%Q%$%i(B pgcc $B$rMQ$$$k$3$H$G!$A4BN$N=hM}$,9bB.2=$5$l$^(B
    $B$9!%(B


9$B!%:#8e$NE8K>(B

    $B2;@<$H1GA|$NF14|:F@8(B


# 
#  Error-correcting Multicast on Open Nodes (EMON)
#
#  This software is placed in public domain.
#

#
#  author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
#             KOMURA Takaaki <komura@astem.or.jp>	
#             FUJIKAWA Kenji <fujikawa@real-internet.org>
#  started:   2001/05/17
#
#  $Id: Makefile,v 1.1 2008/09/26 15:10:34 emon Exp $
#

CC=gcc

CFLAGS=-Wall
CFLAGS+=-DDEBUG -DNO_PRINT -I/usr/local/include/SDL -I/usr/local/include -D_GNU_SOURCE=1 -D_REENTRANT


CFLAGS+=	-g -O2  -DHAVE_CONFIG_H
LDFLAGS=	 

JPEGLIB=-L/usr/local/lib -ljpeg -L./jpegbuf -ljpegbuf
INCLUDES=-I/usr/local/include -I./jpegbuf

TARGETS=fecenc rtpenc udpsend \
	udprecv rtpdec fecdec jpegplay \
	rtpsend rtprecv \
	\
	jpegcat pipeerr pipesave pipeinfo filerepeat jpegsave\
	\
	audiocapt audioplay audiocaptplay \
	jpegcapt \
	\
	ntspwait ntspcall

all: $(TARGETS)

getaddrinfo.o: compat/getaddrinfo.c
getnameinfo.o: compat/getnameinfo.c
net_addrcmp.o: compat/net_addrcmp.c

#
#  autoconf
#
AUTOCONF_ORIG=	configure.in acconfig.h Makefile.in
AUTOCONF_AGEN=	configure config.h.in
AUTOCONF_CONF=	config.h config.cache config.log config.status Makefile
AUTOCONF_TEMP=	confdefs.h

$(AUTOCONF_AGEN): $(AUTOCONF_ORIG)
	-chmod +w $(AUTOCONF_AGEN)
	autoconf
	autoheader
	chmod -w $(AUTOCONF_AGEN)

clean:
	rm -f $(TARGETS) *.o *~ *.core
	(cd jpegbuf; make clean)

config-clean:
	rm -f Makefile Makefile.bak config.cache config.h config.log config.status

depend:
	makedepend -- $(CFLAGS) -- *.c

fecenc: fecenc.o pipe_hd.o rs_emon.o libfec.o timeval.o debug.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC) -lm

rtpenc: rtpenc.o rtp_hd.o debug.o timeval.o pipe_hd.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

udpsend: udpsend.o debug.o timeval.o pipe_hd.o sendsock_setup.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)


rtpsend.o: rtpenc.c
	$(CC) -o $(.TARGET) $(CFLAGS) -DRTPSEND $(CMPFLAGS) $(INCLUDES) -c rtpenc.c

rtpsend: rtpsend.o rtp_hd.o debug.o timeval.o pipe_hd.o sendsock_setup.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

rtprecv.o: rtpdec.c
	$(CC) -o $(.TARGET) $(CFLAGS) -DRTPRECV $(CMPFLAGS) $(INCLUDES) -c rtpdec.c

rtprecv: rtprecv.o rtp_hd.o debug.o timeval.o pipe_hd.o multicast.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)


udprecv: udprecv.o debug.o pipe_hd.o multicast.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

rtpdec: rtpdec.o rtp_hd.o debug.o timeval.o pipe_hd.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

fecdec: fecdec.o pipe_hd.o rs_emon.o libfec.o fec_decode.o timeval.o debug.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC) -lm

jpegplay: jpegplay.o decoder_buf.o pipe_hd.o debug.o timeval.o decoder_buf.o
	(cd jpegbuf; make)
	$(CC) -o $(.TARGET) $(LDFLAGS) -L/usr/local/lib -Wl,-rpath,/usr/local/lib -lSDL -pthread $(.ALLSRC) \
	$(JPEGLIB)


jpegcat: jpegcat.o pipe_hd.o debug.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

pipesave: pipesave.o pipe_hd.o debug.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

jpegsave: jpegsave.o pipe_hd.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

pipeerr: pipeerr.o pipe_hd.o 
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

pipeinfo: pipeinfo.o pipe_hd.o debug.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

filerepeat: filerepeat.o pipe_hd.o debug.o timeval.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

audiocapt: audiocapt.o debug.o pipe_hd.o aucodec.o aucodec_l8l16.o aucodec_dvi4.o aucodec_pcmu.o time.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

audioplay: audioplay.o debug.o pipe_hd.o aucodec.o aucodec_l8l16.o aucodec_dvi4.o aucodec_pcmu.o timeval.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

audiocaptplay: audiocaptplay.o debug.o 
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

jpegcapt: jpegcapt.o debug.o pipe_hd.o capture.o timeval.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC) $(JPEGLIB)

ntspwait: ntspwait.o ntsputil.o net_addrcmp.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

ntspcall: ntspcall.o ntsputil.o net_addrcmp.o
	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC)

.c.o:
	$(CC) -o $(.TARGET) $(CFLAGS) $(CMPFLAGS) $(INCLUDES) -c $(.IMPSRC)

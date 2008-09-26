# Generated automatically from Makefile.in by configure.

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
CFLAGS+=-DDEBUG -DNO_PRINT 


CFLAGS+=	-g -O2  -DHAVE_CONFIG_H
LDFLAGS=	 

JPEGLIB=-L/usr/local/lib -ljpeg -L./jpegbuf -ljpegbuf
INCLUDES=-I/usr/local/include -I./jpegbuf

jpegcapt_vaio: jpegcapt_vaio.o debug.o pipe_hd.o timeval.o \
	mchip.o pci_netbsd.o sonypi.o ptable.o yuv.o

	$(CC) -o $(.TARGET) $(LDFLAGS) $(.ALLSRC) $(JPEGLIB) \
	-R/usr/pkg/lib -R/usr/X11R6/lib -li386

.c.o:
	$(CC) -o $(.TARGET) $(CFLAGS) $(CMPFLAGS) $(INCLUDES) -c $(.IMPSRC)


link:
	ln -s ../*.h .
	ln -s ../*.o .
	ln -s ../jpegbuf .

rmlink:
	find . -type l -delete

rmobj:
	rm -f *.o jpegcapt_vaio

clean:	rmlink rmobj

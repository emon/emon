CC=	gcc
GOAL=	libjpegbuf.a
OBJS=	bufio.o jbufdst.o jbufsrc.o jmemsrc.o
CFLAGS=	-g -Wall -O2
INCLUDES=-I. -I/usr/local/include

LIB=	-ljpeg
LIBDIR=	-L/usr/local/lib
RANLIB=	ranlib
AR=	ar -rv
RM=	rm

all:	$(GOAL)

$(GOAL): $(OBJS)
	$(AR) $(GOAL) $(OBJS)
	$(RANLIB) $(GOAL)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c
clean:
	$(RM) -f $(GOAL) $(OBJS) *core

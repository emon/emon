#ifndef _JPEGBUF_BUFIO_H_
#define _JPEGBUF_BUFIO_H_

typedef struct buf {
	int writecnt;
	int readcnt;
	char *buf;
} buf_t;

void comp_buf_init(buf_t *, char *);
void decomp_buf_init(buf_t *, char *, int);

int write_buf(char *, buf_t *, int);
int read_buf(char *, buf_t *, int);

#endif /* _JPEGBUF_BUFIO_H_*/

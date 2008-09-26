#include <string.h>
#include "bufio.h"

void comp_buf_init(buf_t *buf ,char *dst)
{
	buf->writecnt=0;
	buf->readcnt=0;
	buf->buf = dst;
}

void decomp_buf_init(buf_t *buf, char *src, int len)
{
	buf->readcnt=0;
	buf->writecnt = len;
	buf->buf = src;
}

int write_buf(char *src, buf_t *buf, int size)
{
	bcopy(src, buf->buf + buf->writecnt, size);
	buf->writecnt += size;
  
	return size;
}

int read_buf(char *dst, buf_t *buf, int size)
{
	int rcnt=0;
	int datacnt;

	datacnt = buf->writecnt - buf->readcnt;
  
	if (datacnt > 0){
		if (size <= datacnt)
			rcnt = size;
		else 
			rcnt = datacnt;

		bcopy(buf->buf + buf->readcnt, dst, rcnt);
		buf->readcnt += rcnt;

	} else
		rcnt = 0;

	return rcnt;
}




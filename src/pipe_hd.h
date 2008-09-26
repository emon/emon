/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            FUJIKAWA Kenji <fujikawa@real-internet.org>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: pipe_hd.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _pipe_hd_h_
#define _pipe_hd_h_

/*
 * EMON Message Format
 * 
 * Message Header
 * 
 * 0              7 8            15 16           23 24           31
 * +-------+-+-----+---------------+---------------+---------------+
 * |  Ver  |M| Pad |                 Message Length                |
 * +-------+-+-----+---------------+---------------+---------------+
 * |                           Timestamp                           |
 * +---------------+---------------+---------------+---------------+
 * 
 * Ver              4-bit Protocol version number = 1.
 * 
 * M                Marker bit.
 * 
 * Messsage Length  24-bit unsigned integer. Length of the Message Payload.
 * 
 * Timestamp        32-bit unsigned integer. RTP timestamp.
 * 
 */

#include <sys/types.h>
#include <netinet/in.h>

typedef struct {
	u_int32_t vmlen;
	u_int32_t ts;
} pipe_hd_t;

#define PIPE_HEADER_LEN sizeof(pipe_hd_t)
#define PIPE_ERROR ((ssize_t)-1)
#define PIPE_END ((ssize_t)-2)
#define END_FLAG 0x00ffffff

void            pipe_set_version(void *hd, u_int8_t version);
void            pipe_set_marker(void *hd, u_int8_t marker);
#define         pipe_set_length(hd, len) \
	((u_int32_t*)(hd))[0] = (((u_int32_t*)(hd))[0] & htonl(0xff000000)) \
                               | htonl((len) & 0x00ffffff)
#define         pipe_set_timestamp(hd, ts) \
	((u_int32_t*)(hd))[1] = htonl(ts)

u_int8_t        pipe_get_version(void *hd);
u_int8_t        pipe_get_marker(void *hd);
#define         pipe_get_length(hd) \
	(u_int32_t)(ntohl(((u_int32_t*)(hd))[0]) & 0x00ffffff)
#define         pipe_get_timestamp(hd) \
	(u_int32_t)ntohl(((u_int32_t*)(hd))[1])

typedef void* PIPE_CONTEXT;

ssize_t         pipe_blocked_read_block(int fd, void *hd, void *payload, int maxlen);
ssize_t         pipe_blocked_read_message(int fd, void *hd, void *payload, int maxlen);
ssize_t         pipe_blocked_read_message_ex(int fd, void *hd, void *payload, int *eras_pos, int *no_eras, int num_packet, int plen);

PIPE_CONTEXT    pipe_context_init(int fd, int num_packet, int packet_len);
ssize_t         pipe_blocked_read_packet_ex(PIPE_CONTEXT p, void *hd, void *payload);
void            pipe_context_uninit(PIPE_CONTEXT p);

ssize_t         pipe_blocked_write_block(int fd, void *hd, void *payload);

int fd_can_read(int fd, int usec);
int fd_can_read_blocked(int fd);
int fd_can_write(int fd, int usec);
/* keep reading nbytes with ignoring EOF */
ssize_t fd_read_ignoreEOF(int d, void *buf, size_t nbytes);
/* lseek for fd which we will append data to. */
ssize_t fd_lseek_ignoreEOF(int d, size_t nbytes);

void            pipe_exit_req();
#endif				/* _pipe_hd_h_ */

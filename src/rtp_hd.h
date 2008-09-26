/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            FUJIKAWA Kenji <fujikawa@real-internet.org>
 * started:   2001/06/16
 *
 * $Id: rtp_hd.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _rtp_hd_h_
#define _rtp_hd_h_

#include <sys/types.h>

/*
 * RTP data header (RFC1889)
 */
typedef struct {
	u_int8_t  vpxc;		/* protocol version 2bit*/
#define VERSION (2<<6)
				/* padding flag */
				/* header extension flag */
				/* CSRC count */
	u_int8_t  mp;		/* marker bit */
				/* payload type */
	u_int16_t seq;		/* sequence number */
	u_int32_t ts;		/* timestamp */
	u_int32_t ssrc;		/* synchronization source */
} rtp_hd_t;

#define RTP_HEADER_LEN sizeof(rtp_hd_t)
#define RTP_HD_MACROS

#ifdef DEBUG
void            debug_rtp_header_print(void *hdr);
#endif

#ifndef RTP_HD_MACROS

void            rtp_reset(void *hd, size_t len);
void            rtp_set_version(void *hd, u_int8_t version);
void            rtp_set_extension(void *hd, u_int8_t extension);
void            rtp_set_ccount(void *hd, u_int8_t ccount);
void            rtp_set_marker(void *hd, u_int8_t marker);
void            rtp_set_ptype(void *hd, u_int8_t ptype); /* payload type */
void            rtp_set_seqnum(void *hd, u_int16_t seqnum);
void            rtp_set_timestamp(void *hd, u_int32_t timestamp);
void            rtp_set_ssrc(void *hd, u_int32_t ssrs);
/* Synchronization Source Identifier */
void            rtp_set_csrc(void *hd, u_int csrc, u_int32_t num);

u_int8_t        rtp_get_version(void *hd);
u_int8_t        rtp_get_extension(void *hd);
u_int8_t        rtp_get_ccount(void *hd);
u_int8_t        rtp_get_marker(void *hd);
u_int8_t        rtp_get_ptype(void *hd);
u_int16_t       rtp_get_seqnum(void *hd);
u_int32_t       rtp_get_timestamp(void *hd);
u_int32_t       rtp_get_ssrc(void *hd);
u_int32_t       rtp_get_csrc(void *hd, size_t num);
void           *rtp_get_payload_p(void *hd);

#else /* if RTP_HD_MACROS */

#include <strings.h>		/* for bzero */
#define rtp_reset(hd,len)	bzero(hd, len);
#define rtp_set_version(hd, version) \
	{ 	u_int8_t       *h = (u_int8_t *) (hd); \
		u_int8_t        v = (u_int8_t) ((version << 6) & 0xc0);\
		h[0] = (h[0] & 0x3f) | v;\
	}
#define rtp_set_extension(hd, extension) \
	{	u_int8_t       *h = (u_int8_t *) (hd);\
		u_int8_t        e = (u_int8_t) ((extension << 4) & 0x10);\
		h[0] = (h[0] & 0xef) | e;\
	}
#define rtp_set_ccount(hd, ccount) \
	{	u_int8_t       *h = (u_int8_t *) (hd);\
		u_int8_t        cc = (u_int8_t) (ccount & 0x0f);\
		h[0] = (h[0] & 0xf0) | cc;\
	}
#define rtp_set_marker(hd, marker) \
	{	u_int8_t       *h = (u_int8_t *) (hd);\
		u_int8_t        m = (u_int8_t) ((marker << 7) & 0x80);\
		h[1] = (h[1] & 0x7f) | m;\
	}
#define rtp_set_ptype(hd, ptype) \
	{	u_int8_t       *h = (u_int8_t *) (hd);\
		u_int8_t        pt = (u_int8_t) (ptype & 0x7f);\
		h[1] = (h[1] & 0x80) | pt;\
	}
#define rtp_set_seqnum(hd, seqnum) \
	{	u_int16_t      *h = (u_int16_t *) (hd);\
		h[1] = htons(seqnum);\
	}
#define rtp_set_timestamp(hd, timestamp) \
	{	u_int32_t      *h = (u_int32_t *) (hd);\
		h[1] = htonl(timestamp);\
	}
#define rtp_set_ssrc(hd, ssrs) \
	{	u_int32_t      *h = (u_int32_t *) (hd);\
		h[2] = htonl(ssrs);\
	}
#define rtp_set_csrc(hd, csrc, num) \
	{	u_int32_t      *h = (u_int32_t *) (hd);\
		h[num + 3] = htonl(csrc);\
	}

#define rtp_get_version(hd) \
	(u_int8_t)((((u_int8_t*)(hd))[0] & 0xc0) >> 6)
#define rtp_get_extension(hd) \
	(u_int8_t)((((u_int8_t*)(hd))[0] & 0x10) >> 4)
#define rtp_get_ccount(hd) \
	(u_int8_t)(((u_int8_t*)(hd))[0] & 0x0f)
#define rtp_get_marker(hd) \
	(u_int8_t)((((u_int8_t*)(hd))[1] & 0x80) >> 7)
#define rtp_get_ptype(hd) \
	(u_int8_t)(((u_int8_t*)(hd))[1] & 0x7f)
#define rtp_get_seqnum(hd) \
	(u_int16_t)ntohs(((u_int16_t *) (hd))[1])
#define rtp_get_timestamp(hd) \
	(u_int32_t)ntohl(((u_int32_t*)(hd))[1])
#define rtp_get_ssrc(hd) \
	(u_int32_t)ntohl(((u_int32_t*)(hd))[2])
#define rtp_get_csrc(hd, num) \
	(u_int32_t)ntohl(((u_int32_t*)(hd))[num + 3])
#define rtp_get_payload_p(hd) \
	(void*)(((u_int32_t*)(hd)) + 3 + rtp_get_ccount((hd)))

#endif /* ifndef RTP_HD_MACROS */
#endif /* ifndef _rtp_hd_h_ */

/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: rtp_hd.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <strings.h>		/* for bzero */
#include "rtp_hd.h"
#include "debug.h"

#ifdef DEBUG
void
debug_rtp_header_print(void *hdr)
{
	printf("\nTS=%d, SQ=%d, M=%d, PT=%d",
	       rtp_get_timestamp(hdr),
	       rtp_get_seqnum(hdr),
	       rtp_get_marker(hdr),
	       rtp_get_ptype(hdr));
	return;
}
#endif


#ifndef RTP_HD_MACROS

void
rtp_reset(void *hd, size_t len)
{
	bzero(hd, len);
}


void
rtp_set_version(void *hd, u_int8_t version)
{
	u_int8_t       *h = (u_int8_t *) hd;
	u_int8_t        v = (u_int8_t) ((version << 6) & 0xc0);

	h[0] = (h[0] & 0x3f) | v;
}


void
rtp_set_extension(void *hd, u_int8_t extension)
{
	u_int8_t       *h = (u_int8_t *) hd;
	u_int8_t        e = (u_int8_t) ((extension << 4) & 0x10);

	h[0] = (h[0] & 0xef) | e;
}


void
rtp_set_ccount(void *hd, u_int8_t ccount)
{
	u_int8_t       *h = (u_int8_t *) hd;
	u_int8_t        cc = (u_int8_t) (ccount & 0x0f);

	h[0] = (h[0] & 0xf0) | cc;
}


void
rtp_set_marker(void *hd, u_int8_t marker)
{
	u_int8_t       *h = (u_int8_t *) hd;
	u_int8_t        m = (u_int8_t) ((marker << 7) & 0x80);

	h[1] = (h[1] & 0x7f) | m;
}


void
rtp_set_ptype(void *hd, u_int8_t ptype)
{				/* payload type */
	u_int8_t       *h = (u_int8_t *) hd;
	u_int8_t        pt = (u_int8_t) (ptype & 0x7f);

	h[1] = (h[1] & 0x80) | pt;
}


void
rtp_set_seqnum(void *hd, u_int16_t seqnum)
{
	u_int16_t      *h = (u_int16_t *) hd;

	h[1] = htons(seqnum);
}


void
rtp_set_timestamp(void *hd, u_int32_t timestamp)
{
	u_int32_t      *h = (u_int32_t *) hd;

	h[1] = htonl(timestamp);
}


void
rtp_set_ssrc(void *hd, u_int32_t ssrs)
{				/* Synchronization Source Identifier */
	u_int32_t      *h = (u_int32_t *) hd;

	h[2] = htonl(ssrs);
}


void
rtp_set_csrc(void *hd, u_int32_t csrc, size_t num)
/* Contributing Source Identifier, num: 0 = 1st-CCRS */
{
	u_int32_t      *h = (u_int32_t *) hd;

	h[num + 3] = htonl(csrc);
}


/* ---------------------------------------------------------------------- */

u_int8_t
rtp_get_version(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;

	return (h[0] & 0xc0) >> 6;
}

u_int8_t
rtp_get_extension(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;

	return (h[0] & 0x10) >> 4;
}


u_int8_t
rtp_get_ccount(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;

	return (h[0] & 0x0f);
}


u_int8_t
rtp_get_marker(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;

	return (h[1] & 0x80) >> 7;
}


u_int8_t
rtp_get_ptype(void *hd)
{
	u_int8_t       *h = (u_int8_t *) hd;

	return h[1] & 0x7f;
}


u_int16_t
rtp_get_seqnum(void *hd)
{
	u_int16_t      *h = (u_int16_t *) hd;

	return ntohs(h[1]);
}



u_int32_t
rtp_get_timestamp(void *hd)
{
	u_int32_t      *h = (u_int32_t *) hd;

	return ntohl(h[1]);
}


u_int32_t
rtp_get_ssrc(void *hd)
{
	u_int32_t      *h = (u_int32_t *) hd;

	return ntohl(h[2]);
}


u_int32_t
rtp_get_csrc(void *hd, size_t num)
/* Contributing Source Identifier, num: 0 = 1st-CCRS */
{
	u_int32_t      *h = (u_int32_t *) hd;

	return ntohl(h[num + 3]);
}

void           *
rtp_get_payload_p(void *hd)
{
	u_int32_t      *h = (u_int32_t *) hd;

	return (h + 3 + rtp_get_ccount(hd));
}

#endif /* ifndef RTP_HD_MACROS */

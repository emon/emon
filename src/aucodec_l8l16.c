/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): ?
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/09/25
 *
 * $Id: aucodec_l8l16.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */


/*

RFC1890

4.4.7 L8

   L8 denotes linear audio data, using 8-bits of precision with an
   offset of 128, that is, the most negative signal is encoded as zero.

4.4.8 L16

   L16 denotes uncompressed audio data, using 16-bit signed
   representation with 65535 equally divided steps between minimum and
   maximum signal level, ranging from -32768 to 32767. The value is
   represented in two's complement notation and network byte order.

*/

#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "aucodec.h"
#include "debug.h"

#define ENDIAN_LITTLE


#if 0
typedef enum{
	AUCODEC_DSPFMT_U8    = 1<<0,
	AUCODEC_DSPFMT_S16LE = 1<<1,
	AUCODEC_DSPFMT_S16BE = 1<<2,
	AUCODEC_DSPFMT_S8    = 1<<3,
	AUCODEC_DSPFMT_U16LE = 1<<4,
	AUCODEC_DSPFMT_U16BE = 1<<5,
}aucodec_dspfmt_t;
#endif

static void      aucodec_l8_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len);
static void      aucodec_l8_enc_deinit
                        (aucodec_enc_t        *enc);
static void      aucodec_l8_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst);
static void      aucodec_l8_dec_deinit
                        (aucodec_dec_t        *dec);

static void      aucodec_l16_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len);
static void      aucodec_l16_enc_deinit
                        (aucodec_enc_t        *enc);
static void      aucodec_l16_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst);
static void      aucodec_l16_dec_deinit
                        (aucodec_dec_t        *dec);


u_int32_t aucodec_l8_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc){
	aucodec_enc_t *enc;
	enc=malloc(sizeof(aucodec_enc_t));
	if(enc==NULL){
		return 1;
	}
	*ret_enc=enc;
	if(dspfmt_avail & AUCODEC_DSPFMT_U8){
		enc->enc=&aucodec_l8_enc;
		enc->deinit=&aucodec_l8_enc_deinit;
		enc->codeblk_maxsize=pcmblk_len*ch;
		enc->pcmblk_size=pcmblk_len*ch;
		*ret_dspfmt=AUCODEC_DSPFMT_U8;
		return 0;
	}
	free(enc);
	return 2;
}

static void      aucodec_l8_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len){
	if(src!=dst){
		memcpy(dst,src,enc->pcmblk_size);
	}
	*ret_dst_len=enc->pcmblk_size;
	return;
}
static void      aucodec_l8_enc_deinit
                        (aucodec_enc_t        *enc){
	free(enc);
	return;
}

u_int32_t aucodec_l8_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec){
	aucodec_dec_t *dec;
	dec=malloc(sizeof(aucodec_dec_t));
	if(dec==NULL){
		return 1;
	}
	*ret_dec=dec;
	if(dspfmt_avail & AUCODEC_DSPFMT_U8){
		dec->dec=aucodec_l8_dec;
		dec->deinit=aucodec_l8_dec_deinit;
		dec->codeblk_maxsize=pcmblk_len*ch;
		dec->pcmblk_size=pcmblk_len*ch;
		*ret_dspfmt=AUCODEC_DSPFMT_U8;
		return 0;
	}
	free(dec);
	return 2;
}

static void      aucodec_l8_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst){
	if(src!=dst){
		memcpy(dst,src,src_len);
	}
	return;
}
static void      aucodec_l8_dec_deinit
                        (aucodec_dec_t        *dec){
	free(dec);
}


/*

       L16

*/
u_int32_t aucodec_l16_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc){
	aucodec_enc_t *enc;
	enc=malloc(sizeof(aucodec_enc_t));
	if(enc==NULL){
		return 1;
	}
	*ret_enc=enc;
	if(dspfmt_avail & AUCODEC_DSPFMT_S16HE){
		enc->enc=aucodec_l16_enc;
		enc->deinit=aucodec_l16_enc_deinit;
		enc->codeblk_maxsize=2*ch*pcmblk_len;
		enc->pcmblk_size=2*ch*pcmblk_len;
		*ret_dspfmt=AUCODEC_DSPFMT_S16HE;
		return 0;
	}
	free(enc);
	return 2;
}

static void      aucodec_l16_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len){
#ifdef ENDIAN_LITTLE
	int16_t   *s;
	int16_t   *d;
	int16_t   *slastp1;

	s       = (int16_t *)src;
	d       = (int16_t *)dst;
	slastp1 = s+(enc->pcmblk_size/2);
	while (slastp1 != s) {
		(*d) = htons(*s);
		s++;
		d++;
	}
	*ret_dst_len=enc->pcmblk_size;
	return;
#else
	if(src!=dst){
		memcpy(dst,src,enc->pcmblk_size);
	}
	*ret_dst_len=enc->pcmblk_size;
	return;
#endif
}

static void      aucodec_l16_enc_deinit
                        (aucodec_enc_t        *enc){
	free(enc);
	return;
}

u_int32_t aucodec_l16_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec){
	aucodec_dec_t *dec;
	dec=malloc(sizeof(aucodec_dec_t));
	if(dec==NULL){
		return 1;
	}
	*ret_dec=dec;
	if(dspfmt_avail & AUCODEC_DSPFMT_S16HE){
		dec->dec=aucodec_l16_dec;
		dec->deinit=aucodec_l16_dec_deinit;
		dec->codeblk_maxsize=2*ch*pcmblk_len;
		dec->pcmblk_size=2*ch*pcmblk_len;
		*ret_dspfmt=AUCODEC_DSPFMT_S16HE;
		return 0;
	}
	free(dec);
	return 2;
}

static void      aucodec_l16_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst){
#ifdef ENDIAN_LITTLE
	int16_t   *s;
	int16_t   *d;
	int16_t   *slastp1;

	s       = (int16_t *)src;
	d       = (int16_t *)dst;
	slastp1 = s+(src_len/2);
	while (slastp1 != s) {
		(*d) = ntohs(*s);
		s++;
		d++;
	}
	return;
#else
	if(src!=dst){
		memcpy(dst,src,src_len);
	}
	return;
#endif
}
static void      aucodec_l16_dec_deinit
                        (aucodec_dec_t        *dec){
	free(dec);
}


/* EOF */

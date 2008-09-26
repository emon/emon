/* TODO channel > 1 */

/*
 * Internet-Telephone Sample Implementation
 *
 * iphoned/adpcm.c  - DVI4 (ADPCM) codec for RTP
 *
 * $IPhone: adpcm.c,v 1.2 2000/09/07 04:04:35 cas Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "aucodec.h"
#include "debug.h"

struct rtp_dvi4_hdr {
        int16_t         rdh_predict;    /* predicted value of first sample
                                           from the previous block (L16 format) 
*/
        u_int8_t        rdh_index;      /* current index into stepsize table */
        u_int8_t        rdh_reserved;   /* set to zero by sender, ignored by rec
eiver */
};

typedef struct _aucodec_dvi4_enc_t {
	aucodec_enc_t        aucodec_enc;
	struct rtp_dvi4_hdr  dvi4_last_hdr;
}aucodec_dvi4_enc_t;

#define DVI4_HEADER_SIZE (4)

/*
 * tables
 */

static int index_table[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsize_table[89] = {
     7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
   130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
   876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
  2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
  5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


static void      aucodec_dvi4_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len);
static void      aucodec_dvi4_enc_deinit
                        (aucodec_enc_t        *enc);
static void      aucodec_dvi4_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst);
static void      aucodec_dvi4_dec_deinit
                        (aucodec_dec_t        *dec);

u_int32_t aucodec_dvi4_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc){
	aucodec_dvi4_enc_t *enc;
	enc=malloc(sizeof(aucodec_dvi4_enc_t));
	if(enc==NULL){
		return 1;
	}
	*ret_enc=(aucodec_enc_t*)enc;
	enc->dvi4_last_hdr.rdh_predict=0;
	enc->dvi4_last_hdr.rdh_index=0;
	if(dspfmt_avail & AUCODEC_DSPFMT_S16HE){
		enc->aucodec_enc.enc=aucodec_dvi4_enc;
		enc->aucodec_enc.deinit=aucodec_dvi4_enc_deinit;
		enc->aucodec_enc.codeblk_maxsize=(ch*pcmblk_len+1)/2+DVI4_HEADER_SIZE;
		enc->aucodec_enc.pcmblk_size=2*ch*pcmblk_len;
		*ret_dspfmt=AUCODEC_DSPFMT_S16HE;
		return 0;
	}
	free(enc);
	return 2;
}

static void      aucodec_dvi4_enc
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len){
	long predict;
	int index;
	int step;
	int valbuf = 0;
	aucodec_dvi4_enc_t *enc_dvi4;

	struct rtp_dvi4_hdr *hp;
	signed char * dp;	/* destination pointer */
	int n;
	int sample;
	int16_t    *sp;
	sp=src;

	enc_dvi4=(aucodec_dvi4_enc_t*)enc;
	sample=enc_dvi4->aucodec_enc.pcmblk_size/2;
	*ret_dst_len=enc_dvi4->aucodec_enc.codeblk_maxsize;
	/* store last header into the packet */
	hp = (struct rtp_dvi4_hdr *)dst;
	hp->rdh_predict = htons(enc_dvi4->dvi4_last_hdr.rdh_predict);
	hp->rdh_index = enc_dvi4->dvi4_last_hdr.rdh_index;
	hp->rdh_reserved = 0;


	/* call previous state values */
	predict = enc_dvi4->dvi4_last_hdr.rdh_predict;
	index = enc_dvi4->dvi4_last_hdr.rdh_index;
	step = stepsize_table[index];

	/* encode */
	dp = (signed char *)(dst + sizeof(struct rtp_dvi4_hdr));
	for (n = 0; n < sample; n++) {
		int delta;
		int pred_diff;
		long diff;
		int sign;

		/* calculate difference from previous value */
		diff = *sp - predict;
		sign = (diff >> 28) & 8;	/* extract sign bit */
		if (sign)	diff = -diff;

		/* divide and clamp */
		delta = 0;
		pred_diff = step >> 3;

		if (diff >= step) {
			delta = 4;
			diff -= step;
			pred_diff += step;
		}
		step >>= 1;
		if (diff >= step) {
			delta |= 2;
			diff -= step;
			pred_diff += step;
		}
		step >>= 1;
		if (diff >= step) {
			delta |= 1;
			pred_diff += step;
		}

		/* update previous value */
		if (sign)	predict -= pred_diff;
		else		predict += pred_diff;

		/* clamp previous value to 16 bits */
		if (predict > 32767)		predict = 32767;
		else if (predict < -32768)	predict = -32768;

		/* assemble value, update index and step values */
		delta |= sign;

		index += index_table[delta];
		if (index < 0)	index = 0;
		if (index > 88)	index = 88;
		step = stepsize_table[index];

		/* output value */
		if ((n & 1) == 0)	valbuf = delta;
		else			*dp++ = valbuf | delta << 4;

#if 0
		/* next */
		sp += 2;	/* skip another channel */
#else
		sp += 1;	/* skip another channel */
#endif

	}
	enc_dvi4->dvi4_last_hdr.rdh_predict=predict;
	enc_dvi4->dvi4_last_hdr.rdh_index=index;
#if 0
	/* returns byte length of payload */
	return ((unsigned char *)dp - dst);
#else
	return ;
#endif

}

static void      aucodec_dvi4_enc_deinit
                        (aucodec_enc_t        *enc){
	free(enc);
	return;
}

u_int32_t aucodec_dvi4_dec_init
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
		dec->dec=aucodec_dvi4_dec;
		dec->deinit=aucodec_dvi4_dec_deinit;
		dec->codeblk_maxsize=(ch*pcmblk_len+1)/2+DVI4_HEADER_SIZE;
		dec->pcmblk_size=2*ch*pcmblk_len;
		*ret_dspfmt=AUCODEC_DSPFMT_S16HE;
		return 0;
	}
	free(dec);
	return 2;
}

static void      aucodec_dvi4_dec
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst){
	long predict;
	int index;
	int step;
	int valbuf = 0;

	int samples;

	const struct rtp_dvi4_hdr *hp;
	const signed char *sp;
	int n;
	int16_t           *dp;

	dp=dst;
	/* compute sample count */
	if (src_len < sizeof(struct rtp_dvi4_hdr)){
#if 0
		return -1;
#else
		return ;
#endif 

	}
	samples = (src_len - sizeof(struct rtp_dvi4_hdr));
	samples *=2;

	/* retrieve header value from the packet */
	hp = (const struct rtp_dvi4_hdr *)src;
	predict = (signed short)ntohs(hp->rdh_predict);
	index = hp->rdh_index;
	step = stepsize_table[index];

	/* decode */
	sp = (const signed char *)(src + sizeof(struct rtp_dvi4_hdr));
	for (n = 0; n < samples; n++) {
		int delta;
		int sign;
		int pred_diff;

		/* get the delta value */
		if ((n & 1) == 0) {
			valbuf = *sp++;
			delta = valbuf & 0x0f;
		} else {
			delta = (valbuf >> 4) & 0x0f;
		}

		/* find new index value (for later) */
		index += index_table[delta];
		if (index < 0)	index = 0;
		if (index > 88)	index = 88;

		/* separate sign and magnitude */
		sign = delta & 8;
		delta = delta & 7;

		/* compute difference and new predicted value */
		pred_diff = step >> 3;
		if (delta & 4)	pred_diff += step;
		if (delta & 2)	pred_diff += step >> 1;
		if (delta & 1)	pred_diff += step >> 2;

		if (sign)	predict -= pred_diff;
		else		predict += pred_diff;

		/* clamp output value */
		if (predict > 32767)		predict = 32767;
		else if (predict < -32768)	predict = -32768;

		/* update step value */
		step = stepsize_table[index];

		/* output value */
//		*dp++ = predict;	/* for left */
		*dp++ = predict;	/* for right */
	}
}
static void      aucodec_dvi4_dec_deinit
                        (aucodec_dec_t        *dec){
	free(dec);
}


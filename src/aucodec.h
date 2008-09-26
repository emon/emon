/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): ?
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/09/24
 *
 * $Id: aucodec.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _AUCODEC_H_
#define _AUCODEC_H_

#include <sys/types.h>

#define AUCODEC_DSP_OSS
#define ENDIAN_LITTLE

#ifdef AUCODEC_DSP_OSS
#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif
#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#endif
#ifdef HAVE_SOUNDCARD_H
#include <soundcard.h>
#endif
typedef enum{
	AUCODEC_DSPFMT_U8    = 1<<0,
	AUCODEC_DSPFMT_S16LE = 1<<1,
	AUCODEC_DSPFMT_S16BE = 1<<2,
	AUCODEC_DSPFMT_S8    = 1<<3,
	AUCODEC_DSPFMT_U16LE = 1<<4,
	AUCODEC_DSPFMT_U16BE = 1<<5,
}aucodec_dspfmt_t;

aucodec_dspfmt_t aucodec_dspfmt_oss2dspfmt(int ossmask);
int              aucodec_dspfmt_dspfmt2oss(aucodec_dspfmt_t dspfmt);
#endif

#ifdef ENDIAN_LITTLE
#define AUCODEC_DSPFMT_S16HE (AUCODEC_DSPFMT_S16LE)
#define AUCODEC_DSPFMT_U16HE (AUCODEC_DSPFMT_U16LE)
#else
#define AUCODEC_DSPFMT_S16HE (AUCODEC_DSPFMT_S16BE)
#define AUCODEC_DSPFMT_U16HE (AUCODEC_DSPFMT_U16BE)
#endif


typedef struct _aucodec_enc_t aucodec_enc_t;
typedef struct _aucodec_dec_t aucodec_dec_t;

typedef u_int32_t (*aucodec_enc_init_f) 
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc);
/*pcmblk_len == PCM samples per emon block*/

typedef void      (*aucodec_enc_f)
			(aucodec_enc_t        *enc,
			 void                 *src,
			 void                 *dst,
			 u_int32_t            *ret_dst_len);

typedef void      (*aucodec_enc_deinit_f)
			(aucodec_enc_t        *enc);

typedef u_int32_t (*aucodec_dec_init_f)
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec);

typedef void      (*aucodec_dec_f)
			(aucodec_dec_t        *dec,
			 void                 *src,
			 u_int32_t             src_len,
			 void                 *dst);

typedef void      (*aucodec_dec_deinit_f)
			(aucodec_dec_t        *dec);


struct _aucodec_enc_t {
	aucodec_enc_f         enc;
	aucodec_enc_deinit_f  deinit;
	u_int32_t             codeblk_maxsize;
        u_int32_t             pcmblk_size;
};
/* codeblk_maxsize == the maximam of encoded code size per pcmblk */
/* pcmblk_size     == size per emon block */
struct _aucodec_dec_t {
	aucodec_dec_f         dec;
	aucodec_dec_deinit_f  deinit;
	u_int32_t             codeblk_maxsize;
        u_int32_t             pcmblk_size;
};

u_int32_t aucodec_l8_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc);

u_int32_t aucodec_l8_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec);


u_int32_t aucodec_l16_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc);

u_int32_t aucodec_l16_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec);

u_int32_t aucodec_pcmu_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc);

u_int32_t aucodec_pcmu_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec);
u_int32_t aucodec_dvi4_enc_init
			(aucodec_dspfmt_t   dspfmt_avail, 
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len, 
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_enc_t    **ret_enc);

u_int32_t aucodec_dvi4_dec_init
			(aucodec_dspfmt_t   dspfmt_avail,
			 u_int32_t          ch,
			 u_int32_t          pcmblk_len,
			 aucodec_dspfmt_t  *ret_dspfmt,
			 aucodec_dec_t    **ret_dec);

/* pcmu pcma l16 l8 ... */
aucodec_enc_init_f aucodec_enc_str2init(char *str);
aucodec_dec_init_f aucodec_dec_str2init(char *str);
char* aucodec_enc_init2str(aucodec_enc_init_f init);
char* aucodec_dec_init2str(aucodec_dec_init_f init);

/* idx == 0,1,.... */
/* char* aucodec_enc_idx2name(u_int32_t idx); */
/* char* aucodec_dec_idx2name(u_int32_t idx); */


#endif /* _AUCODEC_H_ */

/* EOF */

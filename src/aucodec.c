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
 * $Id: aucodec.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/types.h>
#include <string.h>

#include "aucodec.h"

#ifdef AUCODEC_DSP_OSS
aucodec_dspfmt_t aucodec_dspfmt_oss2dspfmt(int ossmask){
	aucodec_dspfmt_t dspmask;
	dspmask=0;
	if(ossmask&AFMT_U8)      dspmask|=AUCODEC_DSPFMT_U8;
	if(ossmask&AFMT_S16_LE)  dspmask|=AUCODEC_DSPFMT_S16LE;
	if(ossmask&AFMT_S16_BE)  dspmask|=AUCODEC_DSPFMT_S16BE;
	if(ossmask&AFMT_S8)      dspmask|=AUCODEC_DSPFMT_S8;
	if(ossmask&AFMT_U16_LE)  dspmask|=AUCODEC_DSPFMT_U16LE;
	if(ossmask&AFMT_U16_BE)  dspmask|=AUCODEC_DSPFMT_U16BE;
	return dspmask;
}	

int              aucodec_dspfmt_dspfmt2oss(aucodec_dspfmt_t dspfmt){
	int ossfmt;
	ossfmt=0;
	if(dspfmt==AUCODEC_DSPFMT_U8)    ossfmt=AFMT_U8;
	if(dspfmt==AUCODEC_DSPFMT_S16LE) ossfmt=AFMT_S16_LE;
	if(dspfmt==AUCODEC_DSPFMT_S16BE) ossfmt=AFMT_S16_BE;
	if(dspfmt==AUCODEC_DSPFMT_S8)    ossfmt=AFMT_S8;
	if(dspfmt==AUCODEC_DSPFMT_U16LE) ossfmt=AFMT_U16_LE;
	if(dspfmt==AUCODEC_DSPFMT_U16BE) ossfmt=AFMT_U16_BE;
	return ossfmt;
}
#endif

typedef struct _l_codec_name_t {
	char  *s;
	void  *c;
}l_codec_name_t;


static l_codec_name_t encoder[]={
	{"L8",aucodec_l8_enc_init},
	{"L16",aucodec_l16_enc_init},
/* 	{"PCMA",aucodec_pcma_enc_init}, */
 	{"PCMU",aucodec_pcmu_enc_init}, 
 	{"DVI4",aucodec_dvi4_enc_init}, 
	{NULL,NULL}};
static l_codec_name_t decoder[]={
	{"L8",aucodec_l8_dec_init},
	{"L16",aucodec_l16_dec_init},
/* 	{"PCMA",aucodec_pcma_dec_init}, */
 	{"PCMU",aucodec_pcmu_dec_init}, 
 	{"DVI4",aucodec_dvi4_dec_init}, 
	{NULL,NULL}};

static void* str2init(l_codec_name_t *codecs,char *key){
	while(codecs->c!=NULL){
		if(strcasecmp(codecs->s,key)==0)
			return codecs->c;
		codecs++;
	}
	return NULL;
}
static char* init2str(l_codec_name_t *codecs,void  *ptr){
	while(codecs->c!=NULL){
		if(codecs->c==ptr)
			return codecs->s;
		codecs++;
	}
	return NULL;
}
aucodec_enc_init_f aucodec_enc_str2init(char *str){
	return str2init(encoder,str);
}
aucodec_dec_init_f aucodec_dec_str2init(char *str){
	return str2init(decoder,str);
}

char* aucodec_enc_init2str(aucodec_enc_init_f init){
	return init2str(encoder,init);	
}
char* aucodec_dec_init2str(aucodec_dec_init_f init){
	return init2str(decoder,init);	
}
/* EOF */

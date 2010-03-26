#ifndef _def_value_h
#define _def_value_h

#define STDIN 0
#define STDOUT 1

#define DEF_EMON_FEC_TYPE	"RS"	/* -T : FEC type */
#define DEF_EMON_CLOCK		44100	/* -C : baseclock for RTP timestamp */
#define DEF_EMON_FREQ		1470	/* -R : interval of FEC process */
#define DEF_EMON_PLEN		1200	/* -L : packet length */
#define DEF_EMON_RS_M		4	/* -M : parameter for Reed Solomon */
#define DEF_EMON_RS_N		15	/* -N : same above */
#define DEF_EMON_RS_F		3	/* -F : same above */
#define DEF_EMON_PT	    	96	/* -p : RTP payload type */

#define DEF_RTPSEND_ADDR	"127.0.0.1"	/* -A : send IP addr */
#define DEF_RTPSEND_PORT	10002	/* -P : send IP port */
#define DEF_RTPSEND_TTL		64	/* -t : multicast TTL */
#define DEF_RTPSEND_IF		NULL	/* -I : multicast interface */

#define DEF_RTPRECV_ADDR	NULL	/* -A : recieve IP addr */
#define DEF_RTPRECV_PORT	10002	/* -P : recieve IP port */
#define DEF_RTPRECV_IF		NULL	/* -I : multicast interface */

#define DEF_JPEGCAPT_WIDTH	320
#define DEF_JPEGCAPT_HEIGHT	240
#define DEF_JPEGCAPT_DEV	"/dev/bktr0"
#define DEF_JPEGCAPT_LINE	CAPTURE_INPUT_DEV0
#define DEF_JPEGCAPT_FIELD	0 	/* 0 = both fields */
#define DEF_JPEGCAPT_QUALITY	75
#define DEF_JPEGCAPT_DSIZE	14400	/* 1200bytes * 12packets */

/*#define	DEF_JPEGPLAY_DSIZE      14400*/
#define	DEF_JPEGPLAY_DSIZE      (1024*200)
#define DEF_JPEGPLAY_DELAY_LIMIT 200    /* msec */

#endif				/* _def_value_h */

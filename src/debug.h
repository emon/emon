/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: debug.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _debug_h_
#define _debug_h_

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG			/* this work only by gcc */

#define d_printf(format, args...)	\
	fprintf(stderr, format, ## args)
#define d1_printf(format, args...)	\
	if (debug_level >=1) fprintf(stderr, format, ## args)
#define d2_printf(format, args...)	\
	if (debug_level >=2) fprintf(stderr, format, ## args)
#define d3_printf(format, args...)	\
	if (debug_level >=3) fprintf(stderr, format, ## args)

//__PRETTY_FUNCTION__,
#define debug_printf(lev,format, args...)\
        while(debug_level>=lev){ \
        fprintf(stderr,"(%s:%3d)",__FILE__, __LINE__); \
        fprintf(stderr,format,## args); \
	break;} 
#define debug_printf0(format, args...)\
        debug_printf(0,format,## args)
#define debug_printf1(format, args...)\
        debug_printf(1,format,## args)
#define debug_printf2(format, args...)\
        debug_printf(2,format,## args)
#define debug_printf3(format, args...)\
        debug_printf(3,format,## args)
#define debug_printf4(format, args...)\
        debug_printf(4,format,## args)
#else				/* ifdef DEBUG */

#define d_printf(format, args...)	/* null */
#define d1_printf(format, args...)	/* null */
#define d2_printf(format, args...)	/* null */
#define d3_printf(format, args...)	/* null */

#define debug_printf0(format, args...)	/* null */
#define debug_printf1(format, args...)	/* null */
#define debug_printf2(format, args...)	/* null */
#define debug_printf3(format, args...)	/* null */
#define debug_printf4(format, args...)	/* null */
#define debug_printf(lev,format, args...)	/* null */

#endif				/* ifdef DEBUG */


#define e_printf(format, args...)	fprintf(stderr, format, ## args)
extern int      debug_level;

void            debug_data_print(void *dat, int nbytes);

#endif				/* _debug_h_ */

/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: timeval.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _timeval_h_
#define _timeval_h_

#include <sys/time.h>
#include <sys/types.h>


				/* return (A +/- B) */
int             timeval_add_usec(struct timeval * tv, long usec);
int             timeval_add_msec(struct timeval * tv, long msec);
int             timeval_sub_usec(struct timeval * tv, long usec);

				/*  return (tv1 - tv2) */
long            timeval_diff_usec(struct timeval * tv1, struct timeval * tv2);
long            timeval_diff_msec(struct timeval * tv1, struct timeval * tv2);
struct timeval  timeval_sub_timeval(struct timeval *tv1, struct timeval *tv2);

		/* return {-1:(tv1<tv2), 0:(tv1=tv2), 1:(tv1>tv2)} */
int             timeval_comp(const struct timeval *tv1, const struct timeval *tv2);

				/* A +- B => A */
void            timeval_add(struct timeval *tv1, const struct timeval *tv2);
void		timeval_sub(struct timeval *tv1, const struct timeval *tv2);

void            timeval_average(struct timeval *tv, int n);
struct timeval  timeval_average_timeval(struct timeval *tv, int n);

u_int32_t       timeval_2ts(const struct timeval *tv,const u_int32_t base);
u_int64_t       timeval_sub_ts(struct timeval *tv1, struct timeval *tv2
			       ,u_int32_t base);

#endif

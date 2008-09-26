/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: timeval.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "timeval.h"

int
timeval_add_usec(struct timeval * tv, long usec)
{
	int             carry = 0;

	tv->tv_usec += usec;
	while (tv->tv_usec > (1000 * 1000)) {
		carry++;
		tv->tv_sec += 1;
		tv->tv_usec -= (1000 * 1000);
	}

	return carry;
}


int
timeval_add_msec(struct timeval * tv, long msec)
{
	int             carry = 0;

	tv->tv_sec += msec / 1000;
	tv->tv_usec += (msec % 1000) * 1000;
	while (tv->tv_usec > 1000 * 1000) {
		tv->tv_usec -= (1000 * 1000);
		tv->tv_sec += 1;
	}
	return carry;
}


void
timeval_add(struct timeval * tv1, const struct timeval * tv2)
{
	tv1->tv_usec += tv2->tv_usec;
	tv1->tv_sec += tv2->tv_sec;
	while (tv1->tv_usec > (1000 * 1000)) {
		tv1->tv_usec -= (1000 * 1000);
		tv1->tv_sec += 1;
	}
	return;
}



int
timeval_sub_usec(struct timeval * tv, long usec)
{
	int             carry = 0;

	tv->tv_usec -= usec;
	while (tv->tv_usec < 0) {
		carry--;
		tv->tv_sec -= 1;
		tv->tv_usec += (1000 * 1000);
	}

	return carry;
}


long
timeval_diff_usec(struct timeval * tv1, struct timeval * tv2)
{
	long            diff_usec;
	long            diff_sec;

	diff_sec = tv1->tv_sec - tv2->tv_sec;

	if (-100 <= diff_sec && diff_sec <= 100) {
		diff_usec = diff_sec * 1000 * 1000;
		diff_usec += (tv1->tv_usec - tv2->tv_usec);
	} else if (diff_sec < -100) {
		diff_usec = -100 * 1000 * 1000;
	} else {		/* diff_sec > 100 */
		diff_usec = 100 * 1000 * 1000;
	}
	return diff_usec;
}


long
timeval_diff_msec(struct timeval * tv1, struct timeval * tv2)
{
	long            diff_msec;
	long            diff_usec;
	long            diff_sec;

	diff_sec = tv1->tv_sec - tv2->tv_sec;
	diff_usec = tv1->tv_usec - tv2->tv_usec;
	diff_msec = (diff_sec * 1000) + (diff_usec / 1000);

	return diff_msec;
}


struct timeval
timeval_sub_timeval(struct timeval *tv1, struct timeval *tv2)
{
	struct timeval tmp;

	tmp.tv_sec = tv1->tv_sec;
	tmp.tv_usec= tv1->tv_usec;

	timeval_sub(&tmp, tv2);
	return tmp;
}



int
timeval_comp(const struct timeval *tv1, const struct timeval *tv2)
/* return value
 * -1 : tv1 < tv2
 *  0 : tv1 = tv2
 *  1 : tv1 > tv2
 */
{
	if (tv1->tv_sec > tv2->tv_sec){
		return 1;
	} else if (tv1->tv_sec < tv2->tv_sec) {
		return -1;
	} else {		/* tv1->tv_sec == tv2->tv_sec */
		if (tv1->tv_usec > tv2->tv_usec){
			return 1;
		} else if (tv1->tv_usec < tv2->tv_usec) {
			return -1;
		} else {
			return 0;
		}
	}
}


/* tv1 = tv1 - tv2 */
void
timeval_sub(struct timeval *tv1, const struct timeval *tv2)
{
	tv1->tv_sec -= tv2->tv_sec;
	tv1->tv_usec -= tv2->tv_usec;
	while (tv1->tv_usec < 0){
		tv1->tv_sec -= 1;
		tv1->tv_usec += 1000*1000;
	}
	return;
}


void
timeval_avarage(struct timeval *tv, int n)
{
	int mod;

	if (n == 0){
		return;
	}
	
	mod = tv->tv_sec % n;
	tv->tv_sec /= n;
	tv->tv_usec += (mod * 1000*1000);
	tv->tv_usec /= n;
}


struct timeval
timeval_average_timeval(struct timeval *tv, int n)
{
	struct timeval ret;
	int mod;
	
	if (n == 0){
		ret.tv_sec = 0;
		ret.tv_usec = 0;
		return ret;
	}
	
	ret.tv_sec = tv->tv_sec / n;
	mod = tv->tv_sec % n;

	ret.tv_usec = (tv->tv_usec + (mod * 1000*1000)) / n;
	
	return ret;
}


u_int32_t
timeval_2ts(const struct timeval *tv,const u_int32_t base)
{
	u_int32_t usec,sec;

	usec  = tv->tv_usec /1000;
	usec *= base;
	usec/= 1000;
  
	sec  = tv->tv_sec * base;
	return (sec + usec) ;//& ((1<<32)-1);
}


u_int64_t
timeval_sub_ts(struct timeval *tv1, struct timeval *tv2 ,u_int32_t base)
{
	u_int64_t difusec,difsec;

		//  assert(timeval_comp(&tv1,&tv2)>=0);
	difusec  =1000*1000 + tv1->tv_usec;
	difusec -=tv2->tv_usec;
	difusec *=base;
	difusec /=1000*1000;

	difsec   =tv1->tv_sec - 1;
	difsec  -=tv2->tv_sec;
	difsec  *= base;

	return (difusec + difsec);
}

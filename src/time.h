/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp> 
 * started:   2002/01/24
 *
 * $Id: time.h,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#ifndef _EMON_time_h_
#define _EMON_time_h_

#include <sys/time.h>
#include <sys/types.h>

typedef         struct _time_tv_t time_tv_t;
typedef         struct _time_clkofemon_t time_clkofemon_t;

/* 
   time_tv_t is a time whose the freq of clock is 1micro sec;
   if time_tv_t value == {-1,100},it means -0.999900 micro sec;
 */
struct _time_tv_t{
	long sec;
	long usec;	/* 0<= usec < 1000*1000 */
};

struct _time_clkofemon_t{
	time_tv_t day2emon;
};


u_int32_t       time_32to32(const u_int32_t src,const u_int32_t srchz
			    ,const u_int32_t dsthz);
u_int32_t       time_tvto32(const time_tv_t src,const u_int32_t dsthz);
time_tv_t       time_32totv(const u_int32_t src,const u_int32_t srchz);
time_tv_t       time_32totimeofday(const u_int32_t src,const u_int32_t srchz
				   ,const time_tv_t hint);

/* 
   ret=time_addtv(a,b) 
   ret==a+b;
*/
time_tv_t       time_addtv (const time_tv_t a,const time_tv_t b);
/* 
   ret=time_subtv(a,b) 
   ret==a-b;
   ret->sec<0 ?a<b:a>=b
*/
time_tv_t       time_subtv(const time_tv_t a,const time_tv_t b);
/* 
   ret=time_abstv(a) 
   ret==a.sec>=0 ? a: -a;
*/

time_tv_t       time_abstv (const time_tv_t a);
/* 
   if (a<b) time_cmptv(a,b)<0
   if (a==b)time_cmptv(a,b)==0
   if (a>b) time_cmptv(a,b)>0
*/
int             time_cmptv(const time_tv_t a,const time_tv_t b);
void            time_htontv(u_int8_t *dst,const time_tv_t src);
time_tv_t       time_ntohtv(u_int8_t *src);

time_tv_t       time_gettimeofday();

void            time_resetclkofemon(time_clkofemon_t *clkofemon);
u_int32_t       time_updateclkofemon(int fd,time_clkofemon_t *clkofemon);
time_tv_t       time_gettimeofemon(time_clkofemon_t clkofemon);

#endif

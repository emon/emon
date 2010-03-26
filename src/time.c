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
 * $Id: time.c,v 1.1.1.1 2002/08/10 18:53:22 emon Exp $
 */

#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "time.h"

#include "pipe_hd.h"
#include "debug.h"


u_int32_t       time_32to32(const u_int32_t src,const u_int32_t srchz
			    ,const u_int32_t dsthz){
	u_int64_t dst;

	dst =(u_int64_t)src;
	dst*=(u_int64_t)dsthz;
	dst+=(u_int64_t)(srchz/2); /* for round off */
	dst/=(u_int64_t)srchz;
	return ((u_int32_t)dst);
}

u_int32_t       time_tvto32(const time_tv_t src,const u_int32_t dsthz){
	u_int64_t usec;
	u_int32_t sec;
	
	usec  = src.usec;
	usec *= (u_int64_t)dsthz;
	usec += (u_int64_t)(1000*1000/2); /* for round off */
	usec /= (u_int64_t)(1000*1000);
	sec   = src.sec * dsthz;
	return (sec + (u_int32_t)usec) ;
}

time_tv_t time_32totv(const u_int32_t src,const u_int32_t srchz){
	time_tv_t tv;
	u_int64_t usec;

	tv.sec  =src/srchz;
	usec    =src%srchz;
	usec   *=(u_int64_t)1000*1000;
	usec   +=(u_int64_t)(srchz/2);/* for round off */
	usec   /=(u_int64_t)srchz;
	tv.usec =usec;
//	printf ("time_32totv sec%ld usec%ld \n",tv.sec,tv.usec);
	return tv;
}

time_tv_t       time_32totimeofday(const u_int32_t src,const u_int32_t srchz
				   ,const time_tv_t hint){
	u_int32_t base;
	int32_t   diff;
	time_tv_t tv;
	
	tv.sec=hint.sec;
	tv.usec=0;
	while(1){
		base=tv.sec*srchz;
		diff=src-base;
		if(diff>=0){
			tv=time_addtv(time_32totv(diff,srchz),tv);
			break;
		}
		/* 
		   assert(diff<0) .
		   "diff+1" avoid the calc which when diff==(-1)*1<<31,
		   "(-1)*diff". this result in "diff".
		   so. (-1) is NOT executed.
		*/
//		printf("time_32totimeofday diff %d ,base %ld\n",diff,tv.sec);
		tv.sec-=(u_int32_t)(-1*(diff+1))/srchz +1;
//		printf("time_32totimeofday ==>base %ld\n",tv.sec);
	}
	return tv;
}
/* 
   ret=time_addtv(a,b) 
   ret==a+b;
*/
time_tv_t       time_addtv (const time_tv_t a,const time_tv_t b){
	time_tv_t tv;
	tv.sec =a.sec +b.sec;
	tv.usec=a.usec+b.usec;
	if(tv.usec>=1000*1000){
		tv.sec++;
		tv.usec-=(1000*1000);
	}
	return tv;
}
/* 
   ret=time_subtv(a,b) 
   ret==a-b;
   ret->sec<0 ?a<b:a>=b
*/
time_tv_t       time_subtv(const time_tv_t a,const time_tv_t b){
	time_tv_t tv;
	tv.sec=a.sec-b.sec;
	tv.usec=a.usec-b.usec;
	if(tv.usec<0){
		tv.sec--;
		tv.usec+=(1000*1000);
	}
	return tv;
}
/* 
   ret=time_abstv(a) 
   ret==a.sec>=0 ? a: -a;
*/

time_tv_t       time_abstv (const time_tv_t a){
	time_tv_t tv;
	if(a.sec>0){
		return a;
	}
	tv.sec =(-1)*a.sec - 1;
	tv.usec=1000*1000-a.usec;
	return tv;
}
/* 
   if (a<b) time_cmptv(a,b)<0
   if (a==b)time_cmptv(a,b)==0
   if (a>b) time_cmptv(a,b)>0
*/
int             time_cmptv(const time_tv_t a,const time_tv_t b){
	time_tv_t tv;
	tv=time_subtv(a,b);
	if(tv.sec>0){
		return tv.sec;
	}else if(tv.sec==0 && tv.usec==0){
		return 0;
	}else{
		/* tv.sec <=0 */
		return (tv.sec-1);
	}
}
void            time_htontv(u_int8_t *dst,const time_tv_t src){
	u_int32_t       *sec,*usec;
	sec =(u_int32_t*)dst;
	usec=(u_int32_t*)(dst+sizeof(u_int32_t));
	*sec =htonl(src.sec);
	*usec=htonl(src.usec);
	return;
}
time_tv_t       time_ntohtv(u_int8_t *src){
	time_tv_t tv;
	u_int32_t       *sec,*usec;
	sec = (u_int32_t*)src;
	usec=(u_int32_t*)(src+sizeof(u_int32_t));
	tv.sec =ntohl(*sec);
	tv.usec=ntohl(*usec);
	return tv;
}

time_tv_t       time_gettimeofday(){
	struct timeval stv;
	time_tv_t ret;

	gettimeofday(&stv,NULL);
	ret.sec=stv.tv_sec;
	ret.usec=stv.tv_usec;
	return ret;
}


void            time_resetclkofemon(time_clkofemon_t *clkofemon){
	clkofemon->day2emon.sec=0;
	clkofemon->day2emon.usec=0;
	return;
}
u_int32_t       time_updateclkofemon(int fd,time_clkofemon_t *clkofemon){
	u_int8_t  buf[sizeof(time_tv_t)];
	time_tv_t timeofemon;
	time_tv_t timeofday;
	int ret;
	int cnt;

	timeofday=time_gettimeofday();
	if(fd==-1){
		return 1;
	}
	cnt=0;
	while(1){
		u_int8_t        p_header[PIPE_HEADER_LEN];
		ssize_t         io_ret;

 		ret=fd_can_read(fd,0); 
 		if(!ret){ 
 			break; 
 		} 
 		cnt++; 
		debug_printf3("read clock from fd\n");
		io_ret = pipe_blocked_read_block(fd,p_header,
						 buf,sizeof(time_tv_t));
		break;
	}
	if(cnt>0){
		timeofemon=time_ntohtv(buf);
		d3_printf("ntoh(tv1)={%ld,%ld}\n"
			  ,timeofemon.sec,timeofemon.usec);
		clkofemon->day2emon=time_subtv(timeofemon,timeofday);
	}
	return cnt;
}
time_tv_t        time_gettimeofemon(time_clkofemon_t clkofemon){
	time_tv_t timeofday;

	d3_printf("timer diff={%ld,%ld}\n"
		 ,clkofemon.day2emon.sec
		 ,clkofemon.day2emon.usec);
	timeofday=time_gettimeofday();
	return(time_addtv(clkofemon.day2emon,timeofday));
}



/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): Yasutaka Atarashi <atarashi@mm.media.kyoto-u.ac.jp>
 *            KASAMATSU Ken-ichi <kasamatu@lab1.kuis.kyoto-u.ac.jp>
 * started:   2001/06/16
 *
 * $Id: udpsend.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <wlresvapi.h>

#include "def_value.h"
#include "sendsock_setup.h"
#include "rtp_hd.h"		/* RTP_HEADER_LEN */
#include "pipe_hd.h"
#include "timeval.h"
#include "debug.h"

#define	MAX_SFD	30

static struct opt_values {	/* set by getopt */
	int             send_err_rate;
	int             send_double;	/* 1-3, this is not flag */
	int             rs_N;
	int             clock, freq, plen;
	int             sfd;	/* send socket */
	int		msfd[MAX_SFD]; /* multiple send sockets */
	int		nsfd;	/* number of msfd */

	int		lambdano_enable; /* reserve lambdano or not */
	int		lambdano; /* lambdano to resere */
}               OPT;

struct timeval  total_start, total_end;
int             display_time_all_packet = 0;	/* flag */
int             total_send = 0;
int             total_drop = 0;

/*
  target="1.2.3.4%fxp0/1000" ->  addr="1.2.3.4", port=1000, ifname="fxp0"
  target="::1%fxp0/10000"    ->  addr="::1", port=10000, ifname="fxp0"
 */
int
parse_ip_if_port(char *target, char **addr, int *port, char **ifname)
{
	char *p = target;

	*addr = target;

	for (; *p != NULL; p++){
		if (*p == '%'){
			*p = '\0';
			*ifname = (p + 1);
		}
		if (*p == '/'){
			*p = '\0';
			*port = atoi(p + 1);
		}
	}
	return 0;
}


int
opt_etc(int argc, char *argv[])
{
	int             ch;	/* for getopt */
	char           *temp;

	char           *addr;	/* IP address or domainname */
	int             port;
	char           *src_addr;
	int             src_port;
	u_char          ttl;
	char           *ifname;
	char		ipver;
	char           *mdst[MAX_SFD];
	int		ndst = 0;
	
	char           *opts = "C:R:L:N:A:P:a:p:V:t:I:D:d:e:s123l:";
	char           *helpmes =
	"usage : %s [-C <clock>] [-R <freq>] [-L <packet len>]\n"
	" [-N <n>] [-t <ttl>] [-I <interface>]\n"
	" [-A <IP addr>] [-P <port>] [-a <src IP addr>] [-p <src port>]\n"
	" [-V <IP-ver>] [-D <debug level>] [-e <error rate>] [-123s]\n"
	" [-d <dst IP><%%interface></port> [-l lambdano] [-d ..]]\n"
	" options\n"
	" <general>\n"
	"  C <n>   : clock base\n"
	"  R <n>   : frequency\n"
	"  L <n>   : length of a element\n"
	" <Read Solomon>\n"
	"  N <n>   : # of total packets\n"
	" <network>\n"
	"  d <s>   : <ip>[%%interface]</port> (multiple -d permitted)\n"
        "  A <s>   : IP address\n"
	"  P <n>   : port #\n"
	"  a <s>   : src IP address\n"
	"  p <n>   : src port #\n"
	"  t <n>   : multicast ttl\n"
	"  I <s>   : multicast interface\n"
	"  V <n>   : IP version # (4 or 6)\n"
	" <debug>\n"
	"  D <n>   : debug level\n"
	"  e <n>   : error rate (100 -> 1/100, 1000 -> 1/1000 ...)\n"
	"  1/2/3   : packet rate normal/double/triple\n"
	"  s       : diplay time of all packets\n"
	"  l <n>   : lambdano to reserve\n"
	               ;
	/* set default value */
	addr = DEF_RTPSEND_ADDR;
	port = DEF_RTPSEND_PORT;
	src_addr = NULL;
	src_port = -1;
	ttl = DEF_RTPSEND_TTL;
	ifname = DEF_RTPSEND_IF;
	ipver = 0;

	OPT.clock = DEF_EMON_CLOCK;
	OPT.freq = DEF_EMON_FREQ;
	OPT.plen = DEF_EMON_PLEN;
	OPT.rs_N = DEF_EMON_RS_N;

	OPT.send_err_rate = 0;
	OPT.send_double = 1;

	/* get environment variables */

	if ((temp = getenv("EMON_CLOCK")) != NULL)
		OPT.clock = atoi(temp);
	if ((temp = getenv("EMON_FREQ")) != NULL)
		OPT.freq = atoi(temp);
	if ((temp = getenv("EMON_PLEN")) != NULL)
		OPT.plen = atoi(temp);
	if ((temp = getenv("EMON_RS_N")) != NULL)
		OPT.rs_N = atoi(temp);
	if ((temp = getenv("RTPSEND_ADDR")) != NULL)
		addr = temp;
	if ((temp = getenv("RTPSEND_PORT")) != NULL)
		port = atoi(temp);
	if ((temp = getenv("RTPSEND_SRCADDR")) != NULL)
		src_addr = temp;
	if ((temp = getenv("RTPSEND_SRCPORT")) != NULL)
		src_port = atoi(temp);
	if ((temp = getenv("RTPSEND_TTL")) != NULL)
		ttl = atoi(temp);
	if ((temp = getenv("RTPSEND_IF")) != NULL)
		ifname = temp;

	while ((ch = getopt(argc, argv, opts)) != -1) {
		switch (ch) {
		case 'C':
			OPT.clock = atoi(optarg);
			break;
		case 'R':
			OPT.freq = atoi(optarg);
			break;
		case 'L':
			OPT.plen = atoi(optarg);
			break;
		case 'N':
			OPT.rs_N = atoi(optarg);
			break;
		case 'A':	/* IP address */
			addr = optarg;
			break;
		case 'P':	/* port# */
			port = atoi(optarg);
			break;
		case 'a':	/* IP address */
			src_addr = optarg;
			break;
		case 'p':	/* port# */
			src_port = atoi(optarg);
			break;
		case 'd':
			if (ndst >= MAX_SFD){
				e_printf("too many(%d) destnations are "
					 "specified\n", ndst+1);
				exit (-1);
			}
			mdst[ndst++] = optarg;
			break;
		case 't':
			ttl = atoi(optarg);
			break;
		case 'I':
			ifname = optarg;
			break;
		case 'V':
			ipver = atoi(optarg);
			break;
		case 'D':
			debug_level = atoi(optarg);
			break;
		case 'e':
			OPT.send_err_rate = atoi(optarg);
			break;
		case 's':
			display_time_all_packet = 1;
			break;
		case '1':
		case '2':
		case '3':
			OPT.send_double = ch - '0';
			break;
		case 'l':
			OPT.lambdano = atoi(optarg);
			OPT.lambdano_enable = 1;
			break;
		default:
			fprintf(stderr, helpmes, argv[0]);
			return -1;
		}
	}
	OPT.sfd = sendsock_setup(addr, port, ifname,
				  src_addr, src_port, ttl, ipver);
	d_printf("connect addr=%s port=%d\n", addr, port);

	if (OPT.lambdano_enable) {
		struct wl_resv resv;
		struct wl_reserve_status status;
		socklen_t status_len;
		int ret;

		resv.wlr_flags = WL_RESV_FLAG_SEND;
		resv.wlr_lambda_no = OPT.lambdano;
		ret = setsockopt (OPT.sfd, IPPROTO_IP, WL_ADD_RESV, &resv,
				  sizeof (resv));
		if (ret == -1) {
			d_printf ("setsockopt WL_ADD_RESV error");
			exit (-1);
		}

		status_len = sizeof (status);
		ret = getsockopt (OPT.sfd, IPPROTO_IP, WL_RESERVE_STATUS,
				  &status, &status_len);
		if (ret == -1) {
			d_printf ("getsockopt WL_RESERVE_STATUS error");
			exit (-1);
		}

		if (status.status != WL_RESV_STATUS_RESERVED){
			d_printf ("can't reserve lambdano");
			exit (-1);
		}
	}

	OPT.nsfd = ndst;
	while (--ndst >= 0){
		if (parse_ip_if_port(mdst[ndst], &addr, &port, &ifname)){
			d_printf("parse error %dth -d option\n", ndst+1);
			exit (-1);
		}
		d_printf("connect[%d] addr=%s port=%d\n", ndst, addr, port);
		OPT.msfd[ndst]=sendsock_setup(addr, port, ifname,
					      src_addr, src_port, ttl, ipver);
	}

	return 0;
}

int
next_mediadata_get(char *p_hd, char *payload)
{
	int             len;
	memset(payload, 0, RTP_HEADER_LEN+OPT.plen);
#if 0
	len=pipe_blocked_read_block(STDIN, p_hd, payload, PIPE_HEADER_LEN+OPT.plen);
#else
	len=pipe_blocked_read_block(STDIN, p_hd, payload, RTP_HEADER_LEN+OPT.plen);
#endif
	if(len==PIPE_ERROR) e_printf("\nnext_mediadata_get failed");
	return len;
}

void
wait_proper_time(int count)
{
	const int       packet_rate = OPT.clock * OPT.rs_N / OPT.freq;
	struct timeval  now, proper;
	long            diff_usec;

	proper.tv_sec = count / packet_rate;
	proper.tv_usec =
		(count % packet_rate) * 1000 * 1000 / packet_rate;
	timeval_add(&proper, &total_start);
	gettimeofday(&now, NULL);
	if(display_time_all_packet) {
		d_printf("[%ld.%06ld]", proper.tv_sec, proper.tv_usec);
		d_printf("[%ld.%06ld]", now.tv_sec, now.tv_usec);
	}
	diff_usec = timeval_diff_usec(&proper, &now);
	if (diff_usec > 0) {
		d1_printf(" wait %ld usec", diff_usec);
		usleep(diff_usec);
	}
	if (display_time_all_packet) {
		gettimeofday(&now, NULL);
		d_printf(">%04ld",
			 now.tv_usec / 1000);
	}
}

int
main(int argc, char *argv[])
{
	char           *buffer;
	int             fn = 0;	/* frame no */
	int             count = 0, ret;
	int	        i;
	
	char            p_header[PIPE_HEADER_LEN];

	if (opt_etc(argc, argv) == -1) {
		return -1;
	}
	if (isatty(0)) {
		e_printf("Standard input must be connected with pipe\n");
		return -1;
	}
	if ((buffer = malloc(RTP_HEADER_LEN + OPT.plen)) == NULL) {
		e_printf("cannot malloc for buffer\n");
		return -1;
	}
#if 0
	if (0 <= OPT.start_sec && OPT.start_sec < 60) {
		wait_start_second(OPT.start_sec);
	}
#endif

	memset(buffer, 0, RTP_HEADER_LEN + OPT.plen);

	srand(time(NULL));

	gettimeofday(&total_start, NULL);	/* recode start time */

	fn = 0;

	while((ret = next_mediadata_get(p_header,buffer))>=0) {
		int plen;
		if (ret == 0) {
			/* may be "too large file" or "TS over" */
			d1_printf("\n continue");

			count += OPT.rs_N;
			fn++;
			wait_proper_time(count);

			continue;
		}

		if (OPT.send_err_rate > 0) {
			if ((rand() % OPT.send_err_rate) == 0) {
				total_drop++;
				fn++;
				count++;
				continue;
			}
		}

		wait_proper_time(count);

		plen = pipe_get_length(p_header);
		d2_printf(" write %d bytes", plen);

		if (write(OPT.sfd, buffer, plen) == -1) {
			total_drop++;
			d3_printf(" writev-NG(ts/sq=%d/%d)", pipe_get_timestamp(p_header), count);
		} else {
			d3_printf(" writev-OK(ts/sq=%d/%d)", pipe_get_timestamp(p_header), count);
			total_send++;
		}
		for (i = 0; i < OPT.nsfd; i++){
			if (write(OPT.msfd[i], buffer, plen) == -1){
				d3_printf(" writev-NG(ts/sq/nfd=%d/%d/%d)\n",
					  pipe_get_timestamp(p_header),
					  count, i);
			}
		}
		fn++;
		count++;
	}
	return 0;
}

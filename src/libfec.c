/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/19
 *
 * $Id: libfec.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "rs_emon.h"
#include "debug.h"
#include "libfec.h"

/* ---------- prototype ---------- */

static int      fecs_add8(unsigned char *data);
static int      fecs_add4(unsigned char *data);

/* ---------- global variable ---------- */

fec_param_t     fecp;		/* fec parameters */


/* ---------- functions ---------- */
static unsigned long
Combination(int n, int k)
{
#if 1
	unsigned long   tmp = 1;
	int             i, j;

	if (n - k < k)
		k = n - k;
	for (i = n, j = 1; j <= k; i--, j++) {
		tmp *= i;
		tmp /= j;
	}
	/* d_printf("%d C %d=%lu\n", n, k, tmp); */

	return tmp;
#else

	/*
	 * from Algorism Dictoinary This function accept under 17 as K
	 */
	int             i, j;
	unsigned long   a[17];

	if (n - k < k)
		k = n - k;
	if (k == 0)
		return 1;
	if (k == 1)
		return n;
	if (k > 17) {
		d_printf("Combineation: can not calculate "
			 "because K is too large\n");
		return 0;
	}
	for (i = 1; i < k; i++)
		a[i] = i + 2;
	for (i = 3; i <= n - k + 1; i++) {
		a[0] = i;
		for (j = 1; j < k; j++)
			a[j] += a[j - 1];
	}
	return a[k - 1];
#endif
}


/*
 * ---------- fec_E_calc input k	original data size n	all data size
 * which include FEC data e	original error rate output E	error rate
 * when FEC will be applied
 */

double
fec_E_calc(int k, int n, double e)
{
	double          E = 0;
	int             i;

	for (i = n - k + 1; i <= n; i++) {
		E += Combination(n, i) * pow(e, i) * pow(1 - e, n - i);
	}
	/* E = sum_{i-k+1}^{n} n_C_i * e^{i} * (1-e)^{n-i} */

	/* d_printf("fec_E_calc: k=%d, n=%d, e=%e, E=%e\n", k, n, e, E); */

	return E;
}


/*
 * ---------- fec_param_calc input KK		Number of packets org_err
 * original error rate acv_err	achieve error rate
 * 
 * (output) MM		return value ( 2^MM - 1 = NN ) FF
 *  value ( FF : additional buffer for FEC )
 * 
 * output 0 : success other : fail ----------
 */
int
fec_param_calc(int kk, double org_err, double acv_err, int *mm, int *ff)
{
	double          E;
	int             i, n;

	if (acv_err < pow(0.1, 14)) {
		d_printf("achiev_err is too small\n");
		return -1;
	}
	for (i = 1; i < 20; i++) {
		/* E = sum_{i-k+1}^{n} _{n}C_{i} e^{i} (1-e)^{n-i} */
		E = fec_E_calc(kk, kk + i, org_err);
		if (E < acv_err)
			break;
	}
	*ff = i;		/* additional code length for FEC */
	n = kk + i;

	if (n > 126)
		*mm = 8;	/* MM.. */
	else if (n > 63)
		*mm = 7;
	else if (n > 31)
		*mm = 6;
	else if (n > 15)
		*mm = 5;
	else
		*mm = 4;	/* ..MM */

	d_printf("mm=%d,(1<<mm=%d), kk=%d, ff=%d\n",
		 *mm, (1 << *mm), kk, *ff);
	return 0;
}

int
fec_param_set(int mm, int kk, int ff, int elm_size)
{
	fecp.MM = mm;
	fecp.KK = kk;
	if (mm != 0)
		fecp.NN = (1 << mm) - 1;
	else
		fecp.NN = kk;
	fecp.FF = ff;
	fecp.elm_size = elm_size;

	init_rs(mm, fecp.NN - fecp.FF);

	return 0;
}


#if 0
/*
 * fec_init input plen		Payload length in a packet pnum
 *  of packets orig_err	original error rate achiev_err	achieve error rate
 * buf_num	number of buffer, 0=default(automatically set to
 * FEC_BUFFER_MAX) output number of nessecity packets
 */
int
fec_init(int kk, int elm_size, double org_err, double acv_err, int buf_num)
{
	double          E;	/* ratio of lossing more than (n-k+1) packets */
	int             nn;
	int             i;

	/* E = sum_{i-k+1}^{n} _{n}C_{i} e^{i} (1-e)^{n-i} */

	if (acv_err < pow(0.1, 14)) {
		d_printf("achiev_err is too small\n");
		return -1;
	}
	for (i = 1; i < 20; i++) {
		E = fec_E_calc(kk, kk + i, org_err);
		if (E < acv_err)
			break;
	}

	fecp.elm_size = elm_size;

	fecp.FF = i;
	fecp.KK = kk;
	nn = kk + i;		/* nn */
	if (nn > 126)
		fecp.MM = 8;	/* MM.. */
	else if (nn > 63)
		fecp.MM = 7;
	else if (nn > 31)
		fecp.MM = 6;
	else if (nn > 15)
		fecp.MM = 5;
	else
		fecp.MM = 4;	/* ..MM */

	fecp.NN = (1 << fecp.MM) - 1;	/* NN */

	d_printf("MM=%d,(1<<MM=%d), KK=%d, NN=%d, FF=%d\n",
		 fecp.MM, 1 << fecp.MM, fecp.KK, fecp.NN, fecp.FF);

	init_rs(fecp.MM, fecp.NN - nn + fecp.KK);	/* see fig.1 */

	if (fec_buffer_init(buf_num) == -1)
		return -1;

	return fecp.NN;
}
#endif


int
fecs_fecadd(unsigned char *data)
{
	int             ret = -1;

	switch (fecp.MM) {
	case 4:
		ret = fecs_add4(data);
		break;
	case 8:
		ret = fecs_add8(data);
		break;
	default:
		d_printf("Now this libraly can handle "
			 "only when MM==4 or MM=8\n");
		break;
	}
	return ret;
}

static int
fecs_add8(unsigned char *data)
{
	int             i;
	const int       fec_start_p = fecp.NN - fecp.FF;
	const int       plen = fecp.elm_size;	/* length of a packet */

	for (i = 0; i < plen; i++) {
		dtype          *data_org = (dtype *) (data + i);
		dtype          *data_fec = (dtype *) (data + i + fec_start_p * plen);
		encode_rs8_step(data_org, data_fec, plen);
	}
	return 0;
}


static int
fecs_add4(unsigned char *data)
{
	int             i;
	const int       fec_start_p = fecp.NN - fecp.FF;
	const int       plen = fecp.elm_size;	/* length of a packet */

	for (i = 0; i < plen; i++) {
		dtype          *data_org = (dtype *) (data + i);
		dtype          *data_fec = (dtype *) (data + i + fec_start_p * plen);
		encode_rs4_step(data_org, data_fec, plen);
	}
	return 0;
}

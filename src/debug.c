/* 
 * Error-correcting Multicast on Open Nodes (EMON)
 *
 * This software is placed in public domain.
 */ 

/*
 * author(s): KOMURA Takaaki <komura@astem.or.jp>
 * started:   2001/06/16
 *
 * $Id: debug.c,v 1.1 2008/09/26 15:10:34 emon Exp $
 */

#include <stdio.h>
#include "debug.h"

int             debug_level = 0;

void
debug_data_print(void *dat, int nbytes)
{
	int             i;
	for (i = 0; i < nbytes; i++) {
		if ((i % 16) == 0) {
			d_printf("\n%04X:%02X", i, ((unsigned char *) dat)[i]);
		} else {
			d_printf(" %02X", ((unsigned char *) dat)[i]);
		}
	}
}

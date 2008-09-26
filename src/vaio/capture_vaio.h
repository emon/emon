/* 
   Copyright (C) Andrew Tridgell 2000
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;


#if !defined(__NetBSD__)
#define _XOPEN_SOURCE 500
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifdef LINUX
#include <sys/io.h>
#endif
#include <sys/mman.h>
#include <dirent.h>
#include <ctype.h>
#ifdef LINUX
#include <malloc.h>
#endif
#include <string.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#if defined(LINUX)
#include <linux/pci.h>
#else
#if defined(__FreeBSD__)
#include <sys/pciio.h>
#include "pci.h" /*This is taken from Linux kernel*/
#define O_SYNC O_FSYNC
#include <sys/stat.h>
#endif
#if defined(__NetBSD__)
#include "pci.h" /*This is taken from Linux kernel*/
#include <sys/stat.h>
#include <getopt.h>
#endif
#endif
#include <sys/time.h>
#include "mchip.h"

#define CAPTURE_VERSION "0.2"

/* #include "avi.h"*/

#define PAGE_SIZE 0x1000

#define SONYPI_DEV "/proc/bus/pci/00/07.3"

#define SPIC_PCI_VENDOR 0x8086
#define SPIC_PCI_DEVICE 0x7113

/* the irq selection is 2 bits in the following port */
#define SPI_IRQ_PORT 0x8034
#define SPI_IRQ_SHIFT 22

#define SPI_BASE 0x50

#define SPI_G10A (SPI_BASE+0x14)
#define SPI_G10L (SPI_BASE+0x16) /* 4 bits at this offset - the port offset of
				    2nd port from first */

extern int mchip_dev;
extern int debug;
extern void *mchip_base;

#include "proto_vaio.h"

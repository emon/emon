/* sony programmable I/O control device (SPIC) functions for picturebook 

   Tridge and sfr, July 2000
*/
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

#include "debug.h"
#include "capture_vaio.h"

#define SPIC_PORT1 0x10a0
#define SPIC_PORT2 0x10a4

#define SPIC_CAMERA_BRIGHTNESS 0
#define SPIC_CAMERA_CONTRAST 1
#define SPIC_CAMERA_HUE 2
#define SPIC_CAMERA_COLOR 3
#define SPIC_CAMERA_SHARPNESS 4

#define SPIC_CAMERA_PICTURE 5
#define SPIC_CAMERA_EXPOSURE_MASK 0xC
#define SPIC_CAMERA_WHITE_BALANCE_MASK 0x3
#define SPIC_CAMERA_PICTURE_MODE_MASK 0x30
#define SPIC_CAMERA_MUTE_MASK 0x40

/* the rest don't need a loop until not 0xff */
#define SPIC_CAMERA_AGC 6
#define SPIC_CAMERA_AGC_MASK 0x30
#define SPIC_CAMERA_SHUTTER_MASK 0x7

#define SPIC_CAMERA_SHUTDOWN_REQUEST 7
#define SPIC_CAMERA_CONTROL 0x10

#define SPIC_CAMERA_STATUS 7
#define SPIC_CAMERA_STATUS_READY 0x2
#define SPIC_CAMERA_STATUS_POSITION 0x4

#define SPIC_DIRECTION_BACKWARDS 0x4

#define SPIC_CAMERA_REVISION 8
#define SPIC_CAMERA_ROMVERSION 9


#define SPIC_EVENT_CAPTURE_BUTTON 0x20
#define SPIC_EVENT_CAPTURE_PARTIAL 0x05
#define SPIC_EVENT_CAPTURE_FULL 0x07

#define JOGGER_V1 0x1
#define JOGGER_V2 0x19

#define BRIGHTNESS_V1 0x15
#define BRIGHTNESS_V2 0x29

#define VOLUME_V1 0x14
#define VOLUME_V2 0x29

#define MUTE_V1 0x13
#define MUTE_V2 0x29

static int spic_fd;
#if defined(LINUX)
static void OUTB(u8 v, int port)
{
	outb(v, port);
}

static u8 INB(int port)
{
	usleep(10);
	return inb(port);
}
#define OUTW outw
#else
#if defined(__FreeBSD__)
#include <machine/cpufunc.h>
static void OUTB(u8 v, unsigned int port)
{
	outb(port,v);
}
static void OUTW(u16 v, unsigned int port)
{
	outw(port,v);
}

static u8 INB(unsigned int port)
{
	usleep(10);
	return inb(port);
}
static int deviofd=-1;
static int iopl(int lvl)
{
  if(lvl==0){
    if(deviofd!=-1){
      close(deviofd);
      deviofd=-1;
    }    
    return 0;
  }else if(lvl==3){
    if(deviofd==-1){
      deviofd=open("/dev/io",0);
    }
    return deviofd;
  }
  return -1;
}
#endif
#if defined(__NetBSD__)
#include <sys/types.h>
typedef unsigned long u_long;
#include <machine/sysarch.h>
#include <machine/pio.h>

static void OUTB(u8 v, unsigned int port)
{
	outb(port,v);
}
static void OUTW(u16 v, unsigned int port)
{
	outw(port,v);
}

static u8 INB(unsigned int port)
{
	usleep(10);
	return inb(port);
}

static int iopl(int lvl)
{
	return i386_iopl(lvl);
}
#endif
#endif
/* initialise the SPIC - this comes from the AML code in the ACPI bios */
static void spic_srs(int fd, u16 port1, u16 port2, u8 irq)
{
	u8 v;
	u16 v2;

	pci_config_write_u16(fd, SPI_G10A, port1);
	pci_config_read_u8(fd, SPI_G10L, &v);
	v = (v & 0xF0) | (port1 ^ port2);
	pci_config_write_u8(fd, SPI_G10L, v);

	v2 = inw(SPI_IRQ_PORT);
	v2 &= ~(0x3 << SPI_IRQ_SHIFT);
	v2 |= (irq << SPI_IRQ_SHIFT);
	OUTW(v2, SPI_IRQ_PORT);

	pci_config_read_u8(fd, SPI_G10L, &v);
	v = (v & 0x1F) | 0xC0;
	pci_config_write_u8(fd, SPI_G10L, v);
}

/* disable the SPIC - this comes from the AML code in the ACPI bios */
static void spic_dis(void)
{
	u8 v1;
	u16 v;

	pci_config_read_u8(spic_fd, SPI_G10L, &v1);
	pci_config_write_u8(spic_fd, SPI_G10L, v1 & 0x3F);

	v = inw(SPI_IRQ_PORT);
	v |= (0x3 << SPI_IRQ_SHIFT);
	OUTW(v, SPI_IRQ_PORT);
	close(spic_fd);
}


static void spic_settle(void)
{
int data;
do {
	data = INB(SPIC_PORT2);
printf("data=0x%x\n", data);
	usleep(1);
} while (data & 2);

//	while (INB(SPIC_PORT2) & 2) usleep(1);
}

static u8 spic_call1(u8 dev)
{
	u8 v1, v2;
	spic_settle();

	OUTB(dev, SPIC_PORT2);
	v1 = INB(SPIC_PORT2);
	v2 = INB(SPIC_PORT1);
	if (debug_level > 2)
		printf("spic call1(%x) -> %x %x\n", dev, v1, v2);
	return v2;
}

static u8 spic_call2(u8 dev, u8 fn)
{
	u8 v1;

	while (INB(SPIC_PORT2) & 2) ;
	OUTB(dev, SPIC_PORT2);

	while (INB(SPIC_PORT2) & 2) ;
	OUTB(fn, SPIC_PORT1);

	v1 = INB(SPIC_PORT1);
	if (debug_level > 2)
		printf("spic call2(%x, %x) -> %x\n", dev, fn, v1);
	return v1;
}

static u8 spic_call3(u8 dev, u8 fn, u8 v)
{
	u8 v1;

	while (INB(SPIC_PORT2) & 2) ;
	OUTB(dev, SPIC_PORT2);

	while (INB(SPIC_PORT2) & 2) ;
	OUTB(fn, SPIC_PORT1);

	while (INB(SPIC_PORT2) & 2) ;
	OUTB(v, SPIC_PORT1);

	v1 = INB(SPIC_PORT1);
	if (debug_level > 2)
		printf("call3(%x, %x, %x) -> %x\n", dev, fn, v, v1);
	return v1;
}

static u8 spic_read(u8 fn)
{
	u8 v1, v2;
	int n = 100;
	while (n--) {
		v1 = spic_call2(0x8f, fn);
		v2 = spic_call2(0x8f, fn);
		if (v1 == v2 && v1 != 0xff) {
			return v1;
		}
	}
	return 0xff;
}

/* set brightness, hue etc */
static void spic_set(u8 fn, u8 v)
{
	int n = 100;
	while (n--) {
		if (spic_call3(0x90, fn, v) == 0) break;
	}
}

static int spic_camera_ready(void)
{
	u8 v = spic_call2(0x8f, SPIC_CAMERA_STATUS);
	return (v != 0xff && (v & SPIC_CAMERA_STATUS_READY));
}

/* turn the camera off */
void spic_camera_off(void)
{
	spic_call2(0x91, 0);
}

/* turn the camera on */
void spic_camera_on(void)
{
	int i;

	while (spic_call2(0x91, 0x1) != 0) usleep(1);
	spic_call1(0x93);

	if (!spic_camera_ready()) {
		printf("waiting for camera ready\n");
		for (i=400;i>0;i--) {
			if (spic_camera_ready()) break;
			usleep(100);
		}
		if (i == 0) {
			printf("failed to power on camera\n");
			return;
		}
	}

	spic_set(0x10, 0x5a);
}

/* return 0 if capture not pressed, return 1 if pressed to partial,
   return 2 if fully pressed */
int spic_capture_pressed(void)
{
	u8 v1, v2;
	v1 = inb(SPIC_PORT1);
	v2 = inb(SPIC_PORT2);
	if (v2 != 0x60) return 0;
	if (v1 == SPIC_EVENT_CAPTURE_PARTIAL) return 1;
	if (v1 == SPIC_EVENT_CAPTURE_FULL) return 2;
	return 0;
}

int spic_jogger_pressed(void)
{
	u8 v1, v2;
	v1 = inb(SPIC_PORT1);
	v2 = inb(SPIC_PORT2);
	return (v1 == 0x40 && v2 == 0x10);
}

int spic_jogger_turned(void)
{
	u8 v1, v2;
	v1 = inb(SPIC_PORT1);
	v2 = inb(SPIC_PORT2);
	if ((v2 & 0x10) == 0) return 0;
	spic_call2(0x81, 0xff);
	return (signed char)v1;
}

int spic_jogger(void)
{
	u8 v1, v2, ov1=0, ov2=1;
	while (1) {
		v1 = INB(SPIC_PORT1);
		v2 = INB(SPIC_PORT2);
		if (v1 != ov1 || v2 != ov2) {
			printf("jogger %x %x\n", v1, v2);
		}
		ov1 = v1;
		ov2 = v2;
	}
}

void spic_settings(int brightness, int contrast, int hue, int color, int sharpness, int picture, int agc)
{
	spic_set(SPIC_CAMERA_BRIGHTNESS, brightness);
	spic_set(SPIC_CAMERA_CONTRAST, contrast);
	spic_set(SPIC_CAMERA_HUE, hue);
	spic_set(SPIC_CAMERA_COLOR, color);
	spic_set(SPIC_CAMERA_SHARPNESS, sharpness);
	spic_set(SPIC_CAMERA_PICTURE, picture);
	spic_set(SPIC_CAMERA_AGC, agc);
}

void spic_setup_vga(void)
{
/* :about to start capture again */
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTB(0x0A, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000021 */
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000020 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000000F3 */
OUTW(0x2008, 0x03C4); usleep(10);
OUTW(0xF309, 0x03C4); usleep(10);
OUTW(0x2609, 0x03CE); usleep(10);
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTB(0x0A, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000021 */
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000020 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000000F3 */
OUTW(0xF109, 0x03C4); usleep(10);
OUTW(0x2609, 0x03CE); usleep(10);
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTB(0x0A, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000021 */
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000020 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 0000001F */
OUTW(0x1D09, 0x03C4); usleep(10);
OUTW(0x2609, 0x03CE); usleep(10);
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00002621 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000026E9 */
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00002621 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000026F9 */
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTB(0x09, 0x03CE); usleep(10);
OUTB(0x26, 0x03CF); usleep(10);
OUTB(0x0A, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000021 */
OUTB(0x0A, 0x03CE); usleep(10);
OUTB(0x21, 0x03CF); usleep(10);
OUTB(0x0F, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000001 */
OUTB(0x0F, 0x03C4); usleep(10);
OUTB(0x01, 0x03C5); usleep(10);
OUTB(0x0F, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000001 */
OUTB(0x0A, 0x03CE); usleep(10);
OUTB(0x21, 0x03CF); usleep(10);
OUTB(0x09, 0x03CE); usleep(10);
OUTB(0x26, 0x03CF); usleep(10);
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTB(0xBF, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000000 */
OUTB(0xA3, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 0000000C */
OUTW(0xBF, 0x03CE); usleep(10);
OUTW(0x0CA3, 0x03CE); usleep(10);
OUTB(0x09, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000026 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTB(0x0A, 0x03CE); usleep(10);
inb(0x03CF); usleep(10); /* -> 00000021 */
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00000021 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000000F9 */
OUTW(0x2609, 0x03CE); usleep(10);
OUTW(0x210A, 0x03CE); usleep(10);
OUTB(0x08, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 00002621 */
OUTB(0x09, 0x03C4); usleep(10);
inb(0x03C5); usleep(10); /* -> 000026F9 */
}

void spic_show_settings(void)
{
	printf("Brightness %d  ", spic_read(SPIC_CAMERA_BRIGHTNESS));
	printf("Color %d  ", spic_read(SPIC_CAMERA_COLOR));
	printf("Contrast %d  ", spic_read(SPIC_CAMERA_CONTRAST));
	printf("Hue %d  ", spic_read(SPIC_CAMERA_HUE));
	printf("Sharpness %d\n", spic_read(SPIC_CAMERA_SHARPNESS));
	printf("Picture 0x%02x  ", spic_read(SPIC_CAMERA_PICTURE));
	printf("AGC 0x%02x  ", spic_read(SPIC_CAMERA_AGC));
	printf("Direction: %s\n", (spic_read(SPIC_CAMERA_STATUS) & SPIC_DIRECTION_BACKWARDS) ? "back":"front");
	printf("RomVersion: %d  ", spic_read(SPIC_CAMERA_ROMVERSION));
	printf("Revision: %d\n", spic_read(SPIC_CAMERA_REVISION));
}

void spic_init(void)
{
	iopl(3);
	spic_fd = pci_find_device(SPIC_PCI_VENDOR, SPIC_PCI_DEVICE);
	if (spic_fd == -1) {
		printf("can't find spic PCI device\n");
		exit(1);
	}

	spic_srs(spic_fd, SPIC_PORT1, SPIC_PORT2, 0x3);

	spic_call1(0x82);
	spic_call2(0x81, 0xff);
	spic_call1(0x92);

	printf("spic enabled\n");
}

void sdelay(u32 usecs)
{
	INB(SPIC_PORT1);
	INB(SPIC_PORT2);
	usleep(usecs);
}

void spic_shutdown(int power_off)
{
	spic_call2(0x81, 0); /* make sure we don't get any more events */
	if (power_off) {
		spic_camera_off();
		printf("camera off\n");
	}
	spic_dis();
}





/* a hack module to allow access to video memory and a mchip page
   table in userspace */
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
#include "capture.h"

#define MAX_MEMORY (64*1024*1024)
#define VIDEO_BASE_ADDR 0xfd000000
#define TOKEN1 0xfeeb0123
#define TOKEN2 0xdaed9876

static int test_address(int fd, u32 addr, void *virt)
{
	u32 saved = *(u32 *)virt;
	u32 v;

	*(u32 *)virt = TOKEN1;
	if (pread(fd, &v, 4, addr) != 4) {
		perror("virt_to_phys: read");
		exit(1);
	}
	*(u32 *)virt = saved;
	if (v != TOKEN1) return 0;

	*(u32 *)virt = TOKEN2;
	if (pread(fd, &v, 4, addr) != 4) {
		perror("virt_to_phys: read");
		exit(1);
	}
	*(u32 *)virt = saved;
	if (v != TOKEN2) return 0;

	return 1;
}

/* this is _really_ ugly, but how else do we convert virtual to
   physical addresses in userspace? */
u32 virt_to_phys(void *virt)
{
	int fd;
	off_t addr;

	fd = open("/dev/mem", O_RDONLY|O_SYNC);
	if (fd == -1) {
		perror("virt_to_phys: open(/dev/mem)");
		exit(1);
	}

	for (addr = 0; addr < MAX_MEMORY; addr += PAGE_SIZE) {
		if (test_address(fd, addr, virt)) break;
	}

	close(fd);

	if (addr == MAX_MEMORY) {
		printf("failed to convert virtual address %p\n", virt);
		addr = (u32)-1;
	}

	return addr;
}



u32 videomem_init(void *vmem)
{
	int i;
	u32 *page_table;

	page_table = memalign(PAGE_SIZE, PAGE_SIZE);

	if (!page_table) {
		fprintf(stderr, "videomem_init: cannot allocate page table\n");
		exit(1);
	}

	memset(vmem, 0, PAGE_SIZE);
	if (mlock(vmem, PAGE_SIZE) == -1) {
		perror("failed to lock vmem\n");
		exit(1);
	}

	if (mlock(page_table, PAGE_SIZE) == -1) {
		perror("mlock(page_table, PAGE_SIZE)");
		exit(1);
	}

	page_table[0] = virt_to_phys(vmem);
	for (i = 1; i < 1024; i++) {
		page_table[i] = page_table[0];
	}

	return virt_to_phys(page_table);
}

/* 0x0801 */
void video_bluescreen(void)
{
	int fd;
	int x, y;
	u16 *vmem;
	u8 rgb[480][1024][3];
	static int n=0;

	fd = open("/dev/mem", O_RDWR);
	vmem = mmap(0, 4*1024*1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 
		    VIDEO_BASE_ADDR);

#if 1
	memset(rgb, 0, sizeof(rgb));
	for (y=0;y<480;y++) {
		for (x=3;x<1024;x++) {
			rgb[y][x][0] = 0x10;
			rgb[y][x][1] = 0x0;
			rgb[y][x][2] = 0x10;
		}
	}
	memcpy(vmem, rgb, sizeof(rgb));
#endif

#if 0
	display_rgb(rgb, 1024, 480);
	for (y=0;y<200;y++) {
		for (x=0;x<1024;x++) {
			vmem[y*1024+x] = 0x0108;
		}
	}
#endif
}


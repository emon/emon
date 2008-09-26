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
#include "capture_vaio.h"

#define MAX_MEMORY (256*1024*1024)
#define TOKEN 0xfeeb0000


/* return a page table pointing to N pages of locked memory */
void *ptable_init(int npages, u32 *pt_addr)
{
	int i;
	void *vmem;
	u32 ptable[npages+1];
	int fd;
	off_t addr;

	fd = open("/dev/mem", O_RDONLY|O_SYNC);

	if (fd == -1) {
		perror("virt_to_phys: open(/dev/mem)");
		exit(1);
	}
#ifdef LINUX
	vmem = memalign(PAGE_SIZE, (npages+1)*PAGE_SIZE);
#else __FreeBSD__
	vmem = malloc((npages+1)*PAGE_SIZE);/*Never mind!PAGESIZE aligned memalign is equivalent to valloc and see valloc(3).*/
#endif
	if (!vmem) {
		printf("failed to allocate ptable\n");
		exit(1);
	}

	memset(vmem, 0, (npages+1)*PAGE_SIZE);
	mlock(vmem, (npages+1)*PAGE_SIZE);

	for (i=0;i<npages+1;i++) {
		*(u32 *)(vmem + i*PAGE_SIZE) = TOKEN | i;
	}

	memset(ptable, 0, sizeof(ptable));

	for (addr = 0; addr < MAX_MEMORY; addr += PAGE_SIZE) {
		u32 v;
		pread(fd, &v, sizeof(v), addr);
		if ((v & 0xFFFF0000) != TOKEN) continue;
		i = (v & 0xFFFF);
		if (i > npages) continue;
		(*(u32 *)(vmem + i*PAGE_SIZE)) ^= ~0;
		pread(fd, &v, sizeof(v), addr);
		(*(u32 *)(vmem + i*PAGE_SIZE)) ^= ~0;
		if (~v != (TOKEN | i)) continue;
		ptable[i] = addr;
	}

	close(fd);

	for (i=0;i<npages+1;i++) {
		if (ptable[i] == 0) {
			printf("failed to convert virtual page %d\n", i);
			exit(1);
		}
	}

	memcpy(vmem+npages*PAGE_SIZE, ptable, PAGE_SIZE);
	*pt_addr = ptable[npages];

	return vmem;
}


/* manipulate PCI devices from user space

   Tridge, July 2000
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
#include "capture.h"

#define MAX_BUS 8

/* find a PCI device and return a handle to it */
int pci_find_device(u32 vendor, u32 device)
{
	int bus;
	DIR *dir;
	struct dirent *dent;
	char path[100];
	int fd;
	u16 ven, dev;

	for (bus=0;bus<MAX_BUS;bus++) {
		sprintf(path,"/proc/bus/pci/%02d", bus);
		dir = opendir(path);
		if (!dir) continue;
		while ((dent = readdir(dir))) {
			if (!isxdigit(dent->d_name[0])) continue;
			sprintf(path,"/proc/bus/pci/%02d/%s", bus, dent->d_name);
			fd = open(path,O_RDWR|O_SYNC);
			if (fd == -1) {
				perror(path);
				continue;
			}
			if (pread(fd, &ven, 2, PCI_VENDOR_ID) == 2 &&
			    pread(fd, &dev, 2, PCI_DEVICE_ID) == 2 &&
			    ven == vendor && dev == device) {
				closedir(dir);
				return fd;
			}
			close(fd);
		}
		closedir(dir);
	}
	
	printf("failed to find pci device %x:%x\n", vendor, device);
	return -1;
}


/* routines to read and write PCI config space */
int pci_config_write_u8(int fd, int ofs, u8 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_write_u16(int fd, int ofs, u16 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_write_u32(int fd, int ofs, u32 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_read_u8(int fd, int ofs, u8 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}

int pci_config_read_u16(int fd, int ofs, u16 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}

int pci_config_read_u32(int fd, int ofs, u32 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}

/* find a pci base address via /proc/bus/pci/devices. This seems to be
   needed on some boxes. Why? */
u32 pci_read_base_address(u32 vendor, u32 device)
{
	FILE *f;
	u32 mem, dev, dum;
	char buf[0x1000];

	f = fopen("/proc/bus/pci/devices", "r");
	if (!f) return 0;

	while (fgets(buf, sizeof(buf)-1, f)) {
		if (sscanf(buf,"%x %x %x %x",&dum,&dev,&dum,&mem) != 4) continue;
		if (dev == ((vendor<<16)|device)) {
			fclose(f);
			return mem;
		}
	}
	fclose(f);
	return 0;
}

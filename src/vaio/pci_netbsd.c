/*	$NetBSD: pci_machdep.c,v 1.39 2000/07/18 11:23:28 soda Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */
#include "capture_vaio.h"
#include <machine/pio.h>
#include <dev/pci/pcireg.h>

#define	PCI_MODE1_ENABLE	0x80000000UL
#define	PCI_MODE1_ADDRESS_REG	0x0cf8
#define	PCI_MODE1_DATA_REG	0x0cfc

#define	PCI_MODE2_ENABLE_REG	0x0cf8
#define	PCI_MODE2_FORWARD_REG	0x0cfa

union i386_pci_tag_u {
        u_int32_t mode1; 
        struct {
                u_int16_t port;
                u_int8_t enable;
                u_int8_t forward;
        } mode2;
}; 
typedef union i386_pci_tag_u pcitag_t;
typedef u_int32_t pcireg_t;

static int current_tag = 128;		/* XXX */
static pcitag_t tags[256];		/* XXX */

pcitag_t
pci_make_tag(bus, device, function)
	int bus, device, function;
{
	pcitag_t tag;

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
	tag.mode1 = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
#else
	tag.mode2.port = 0xc000 | (device << 8);
	tag.mode2.enable = 0xf0 | (function << 1);
	tag.mode2.forward = bus;
	return tag;
#endif
}

pcireg_t
pci_conf_read(tag, reg)
	pcitag_t tag;
	int reg;
{
	pcireg_t data;

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	data = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, 0);
	return data;
#else
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	data = inl(tag.mode2.port | reg);
	outb(PCI_MODE2_ENABLE_REG, 0);
	return data;
#endif
}

void
pci_conf_write(tag, reg, data)
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	outl(PCI_MODE1_DATA_REG, data);
	outl(PCI_MODE1_ADDRESS_REG, 0);
#else
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	outl(tag.mode2.port | reg, data);
	outb(PCI_MODE2_ENABLE_REG, 0);
#endif
	return;
}

/* find a PCI device and return a handle to it */
int pci_find_device(u32 vendor, u32 device)
{
	int bus, dev, func, t;
	pcitag_t tag;
	pcireg_t reg;

	for (bus = 0; bus < 16; bus++) {
		for (dev = 0; dev < 256; dev++) {
			for (func = 0; func < 8; func++) {
				tag = pci_make_tag(bus, dev, func);
				reg = pci_conf_read(tag, PCI_ID_REG);
				if (vendor != PCI_VENDOR(reg) ||
				    device != PCI_PRODUCT(reg))
					continue;

				t = current_tag++;
				tags[t] = tag;
				return t;
			}
		}
	}

	return -1;
}

/* routines to read and write PCI config space */
int pci_config_write(int fd, int ofs, int size, pcireg_t v)
{
	int reg;
	pcitag_t tag = tags[fd];
	pcireg_t data;

	reg = ofs & ~0x3;
	data = pci_conf_read(tag, reg);

	switch (size) {
	case 8:
		data &= ~(0xff << ((ofs - reg) * 8));
		data |= v << ((ofs - reg) * 8);
		break;
	case 16:
		data &= ~(0xffff << ((ofs - reg) * 8));
		data |= v << ((ofs - reg) * 8);
		break;
	case 32:
		data = v;
		break;
	}

	pci_conf_write(tag, reg, data);

	return 0;
}

int pci_config_write_u8(int fd, int ofs, u8 v)
{
	return pci_config_write(fd, ofs, 8, v);
}

int pci_config_write_u16(int fd, int ofs, u16 v)
{
	return pci_config_write(fd, ofs, 16, v);
}

int pci_config_write_u32(int fd, int ofs, u32 v)
{
	return pci_config_write(fd, ofs, 32, v);
}

int pci_config_read(int fd, int ofs, int size)
{
	int reg;
	pcitag_t tag = tags[fd];
	pcireg_t data;

	reg = ofs & ~0x3;
	data = pci_conf_read(tag, reg);

	switch (size) {
	case 8:
		return (data >> ((ofs - reg) * 8)) & 0xff;
	case 16:
		return (data >> ((ofs - reg) * 8)) & 0xffff;
	}
	return data;
}

int pci_config_read_u8(int fd, int ofs, u8 *v)
{
	*v = (u8)pci_config_read(fd, ofs, 8);
	return 0;
}

int pci_config_read_u16(int fd, int ofs, u16 *v)
{
	*v = (u16)pci_config_read(fd, ofs, 16);
	return 0;
}

int pci_config_read_u32(int fd, int ofs, u32 *v)
{
	*v = (u32)pci_config_read(fd, ofs, 32);
	return 0;
}

/* find a pci base address via /proc/bus/pci/devices. This seems to be
   needed on some boxes. Why? */
u32 pci_read_base_address(u32 vendor, u32 device)
{
	int fd;
	u32 v;

	fd = pci_find_device(vendor, device);
	pci_config_read_u32(fd, PCI_MAPREG_START, &v);

	return v;
}

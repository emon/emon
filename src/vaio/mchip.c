/* mchip control functions for picturebook 

   Tridge, July 2000

   based on earlier work by 
   Werner Almesberger, Paul `Rusty' Russell and Paul Mackerras.
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

#define NUM_PAGES 1024

void *mchip_base;
static int subsample=0;

static u8 *framebuf;

u32 readl(volatile void *addr)
{
	return *(volatile u32 *)(addr);
}

void writel(u32 v, void *addr)
{
	*(volatile u32 *)(addr) = v;
}

int mchip_hsize(void)
{
	return subsample ? 320 : 640;
}

int mchip_vsize(void)
{
	return subsample ? 240 : 480;
}


static int mchip_mcc_vrj_sync (int iRegister)  /* for iRegister > 0x80 */
{
	int i;
	int iStatus;

	if (iRegister >= MCHIP_VRJ_CMD) {
		while ((readl(mchip_base + MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_VRJ_RDY)==0)
			;
		return 1;
	}


	/* Only MCC synchronization is supported so far. */
	if ((iRegister >= MCHIP_VRJ_CMD) || (iRegister <= MCHIP_MCC_CMD)) {
		printf ( "mchip_mcc_vrj_sync(0x%02x) reg not supported #1\n", iRegister);
		return -1;
	}


	/* Each cycle of this loop checks to see if HIC's status
	   of MCC indicates that access possible. */
	for (i = MCHIP_MCC_VRJ_TIMEOUT; i; i--) { 
		iStatus = readl (mchip_base + MCHIP_HIC_STATUS);

		/* Synchronize for MCC registers. */
		if ((iRegister < MCHIP_VRJ_CMD) 
		    && (iStatus & MCHIP_HIC_STATUS_MCC_RDY)) {
			iStatus = readl (mchip_base + MCHIP_MCC_STATUS);
			switch (iRegister) {

				case MCHIP_MCC_IIC_WR:
					if (iStatus & MCHIP_MCC_STATUS_IIC_WR) {
						return (1);
					}
					break;

				case MCHIP_MCC_MCC_WR:
					if (iStatus & MCHIP_MCC_STATUS_MCC_WR) {
						return (1);
					}
					break;

				case MCHIP_MCC_MCC_RD:
					if (iStatus & MCHIP_MCC_STATUS_MCC_RD) {
						return (1);
					}
					break;

					/* Is this right?  Who can say, in this
					   topsy-turvy mixed up world? --RR */
				case MCHIP_MCC_STATUS:
				case MCHIP_MCC_SIG_POLARITY:
				case MCHIP_MCC_IRQ:
				case MCHIP_MCC_HSTART:
				case MCHIP_MCC_VSTART:
				case MCHIP_MCC_HCOUNT:
				case MCHIP_MCC_VCOUNT:
				case MCHIP_MCC_R_XBASE:
				case MCHIP_MCC_R_YBASE:
				case MCHIP_MCC_R_XRANGE:
				case MCHIP_MCC_R_YRANGE:
				case MCHIP_MCC_B_XBASE:
				case MCHIP_MCC_B_YBASE:
				case MCHIP_MCC_B_XRANGE:
				case MCHIP_MCC_B_YRANGE:
				case MCHIP_MCC_R_SAMPLING:
					return (1);

				default:
					printf ( "mchip_mcc_vrj_sync(0x%02x) reg not supported #2\n",
					    iRegister);
					return (0);
			}
		} 

	}



	printf("mchip_mcc_vrj_sync(0x%02x) %04x TIMEOUT\n",
		 iRegister, iStatus);
	return (0);
}



static void mchip_set(int iRegister, u32 iValue)
{
	if (iRegister >= MCHIP_VRJ_CMD) { 
		writel (iValue, mchip_base + iRegister);
	} else if (iRegister >= MCHIP_MCC_CMD) {    /* MCC low level regs*/
		if (mchip_mcc_vrj_sync (iRegister)) {
			writel (iValue, mchip_base + iRegister);
		}
	} else {                                   /* High lvl PCI & HIC regs */
		writel (iValue, mchip_base + iRegister);
	}
}


static void mchip_sync (int reg) /* wait until register is accessible */
{
	int i;
        int status;
	u32 mask = MCHIP_HIC_STATUS_MCC_RDY;

	if (reg <= 0x80) return;
	if (reg >= 0x100) mask = MCHIP_HIC_STATUS_VRJ_RDY;
	for (i = MCHIP_REG_TIMEOUT; 
	     i && !((status=readl(mchip_base+MCHIP_HIC_STATUS)) & mask);
	     i--) sdelay(1);
	if (!i) printf ("mchip_sync() TIMEOUT on reg 0x%x  status=0x%x\n", reg, status);
}


static u32 mchip_read(int reg)
{
	u32 res;

	mchip_sync (reg);
	res = readl (mchip_base + reg);
	return res;
}


static int frame_num;

static void mchip_dma_setup(void)
{
	static u32 pt_addr;
	int i;
	if (!framebuf) {
		printf("dma setup starting ...\n");
		framebuf = ptable_init(NUM_PAGES, &pt_addr);
		printf("dma setup done\n");
	}

	mchip_set(MCHIP_MM_PT_ADDR, pt_addr);
	for (i=0;i<4;i++) {
		mchip_set(MCHIP_MM_FIR(i), 0);
	}
	memset(framebuf, 0, NUM_PAGES*PAGE_SIZE);
}


void mchip_hic_stop(void)
{
	if (!(mchip_read(MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_BUSY)) return;
	printf("stopping HIC\n");
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_STOP);
	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);
	while (mchip_read(MCHIP_HIC_STATUS) != MCHIP_HIC_STATUS_IDLE) sdelay(1);
	sdelay(100);
	mchip_dma_setup();
}


void mchip_set_framerate(double framerate)
{
	int n;

	/* these are very approximate */
	if (subsample) {
		n = 30/framerate - 1;
	} else {
		n = 15/framerate - 1;
	}
	if (n <= 1) n = 0;
	mchip_set(MCHIP_HIC_S_RATE, n);
}


static u32 mchip_wait_frame(void)
{
	int n = 100;
	while (--n) {
		int i;
		for (i=0;i<4;i++) {
			u32 v = mchip_read(MCHIP_MM_FIR((frame_num+i)%4));
			if (v & MCHIP_MM_FIR_RDY) {
				frame_num = (frame_num+i)%4;
				return v;
			}
		}
		sdelay(1);
	}
	if (debug_level>2) {
		printf(__FUNCTION__ " timeout\n");
	}
	return 0;
}

static void mchip_cont_read_frame(u8 *buf, int size)
{
	u32 v;
	int pt_id;
	int avail;

	v = mchip_wait_frame();

	pt_id = (v >> 17) & 0x3FF;
	avail = NUM_PAGES-pt_id;

	if (size > avail*PAGE_SIZE) {
		memcpy(buf, framebuf+pt_id*PAGE_SIZE, avail*PAGE_SIZE);
		memcpy(buf+avail*PAGE_SIZE,
		       framebuf,
		       size-avail*PAGE_SIZE);
	} else {
		memcpy(buf, framebuf+pt_id*PAGE_SIZE, size);
	}

	mchip_set(MCHIP_MM_FIR(frame_num), 0);
	frame_num = (frame_num+1)%4;
}

static int mchip_comp_read_frame(u8 *buf, int size)
{
	u32 v;
	int pt_start, pt_end, trailer;
	int fsize, fsize2;

	v = mchip_wait_frame();

	pt_start = (v >> 19) & 0xFF;
	pt_end = (v >> 11) & 0xFF;
	trailer = (v>>1) & 0x3FF;

	if (pt_end < pt_start) {
		fsize = (256-pt_start)*PAGE_SIZE;
		fsize2 = pt_end*PAGE_SIZE+trailer*4;
		if (fsize+fsize2 > size) {
			printf("oversized compressed frame %d %d\n", 
			       fsize, fsize2);
			fsize = 0;
		} else {
			memcpy(buf, framebuf+pt_start*PAGE_SIZE, fsize);
			memcpy(buf+fsize, framebuf, fsize2); 
			fsize += fsize2;
		}
	} else {
		fsize = (pt_end-pt_start)*PAGE_SIZE+trailer*4;
		if (fsize > size) {
			printf("oversized compressed frame %d\n", fsize);
			fsize = 0;
		} else {
			memcpy(buf, framebuf+pt_start*PAGE_SIZE, fsize);
		}
	}


	mchip_set(MCHIP_MM_FIR(frame_num), 0);
	frame_num = (frame_num+1)%4;
	return fsize;
}

void mchip_take_picture(void)
{
	mchip_hic_stop();
	mchip_set (MCHIP_HIC_MODE, MCHIP_HIC_MODE_STILL_CAP);
	mchip_set (MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);

	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);
	while (mchip_read(MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_BUSY) sdelay(1);
}


/* get a previously taken picture into a buffer */
void mchip_get_picture(u8 *buf, int bufsize)
{
	mchip_set(MCHIP_HIC_MODE, MCHIP_HIC_MODE_STILL_OUT);
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);

	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);

	mchip_cont_read_frame(buf, bufsize);
	while (mchip_read(MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_BUSY) sdelay(1);
}


/* start continuous capture */
void mchip_continuous_start(void)
{
	mchip_hic_stop();

	mchip_set(MCHIP_HIC_MODE, MCHIP_HIC_MODE_CONT_OUT);
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);

	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);
	printf("started continuous capture\n");
}

void mchip_continuous_read(u8 *buf, int bufsize)
{
	mchip_cont_read_frame(buf, bufsize);
}



u16 *mchip_tables(int *size)
{
	static u16 tables[] = {
		/* quantisation tables */
		0xDBFF, 0x4300, 0x1000, 0x0C0B, 0x0C0E, 0x100A, 0x0D0E, 0x120E, 
		0x1011, 0x1813, 0x1A28, 0x1618, 0x1816, 0x2331, 0x1D25, 0x3A28, 
		0x3D33, 0x393C, 0x3833, 0x4037, 0x5C48, 0x404E, 0x5744, 0x3745, 
		0x5038, 0x516D, 0x5F57, 0x6762, 0x6768, 0x4D3E, 0x7971, 0x6470, 
		0x5C78, 0x6765, 0xFF63, 
		0xDBFF, 0x4300, 0x1101, 0x1212, 0x1518, 0x2F18, 0x1A1A, 0x632F, 
		0x3842, 0x6342, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 
		0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 
		0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 0x6363, 
		0x6363, 0x6363, 0xFF63,
		/* huffman tables */
		0xC4FF, 0xB500, 0x0010, 0x0102, 0x0303, 0x0402, 0x0503, 0x0405, 
		0x0004, 0x0100, 0x017D, 0x0302, 0x0400, 0x0511, 0x2112, 0x4131, 
		0x1306, 0x6151, 0x2207, 0x1471, 0x8132, 0xA191, 0x2308, 0xB142, 
		0x15C1, 0xD152, 0x24F0, 0x6233, 0x8272, 0x0A09, 0x1716, 0x1918, 
		0x251A, 0x2726, 0x2928, 0x342A, 0x3635, 0x3837, 0x3A39, 0x4443, 
		0x4645, 0x4847, 0x4A49, 0x5453, 0x5655, 0x5857, 0x5A59, 0x6463, 
		0x6665, 0x6867, 0x6A69, 0x7473, 0x7675, 0x7877, 0x7A79, 0x8483, 
		0x8685, 0x8887, 0x8A89, 0x9392, 0x9594, 0x9796, 0x9998, 0xA29A, 
		0xA4A3, 0xA6A5, 0xA8A7, 0xAAA9, 0xB3B2, 0xB5B4, 0xB7B6, 0xB9B8, 
		0xC2BA, 0xC4C3, 0xC6C5, 0xC8C7, 0xCAC9, 0xD3D2, 0xD5D4, 0xD7D6, 
		0xD9D8, 0xE1DA, 0xE3E2, 0xE5E4, 0xE7E6, 0xE9E8, 0xF1EA, 0xF3F2, 
		0xF5F4, 0xF7F6, 0xF9F8, 0xFFFA, 
		0xC4FF, 0xB500, 0x0011, 0x0102, 0x0402, 0x0304, 0x0704, 0x0405, 
		0x0004, 0x0201, 0x0077, 0x0201, 0x1103, 0x0504, 0x3121, 0x1206, 
		0x5141, 0x6107, 0x1371, 0x3222, 0x0881, 0x4214, 0xA191, 0xC1B1, 
		0x2309, 0x5233, 0x15F0, 0x7262, 0x0AD1, 0x2416, 0xE134, 0xF125, 
		0x1817, 0x1A19, 0x2726, 0x2928, 0x352A, 0x3736, 0x3938, 0x433A, 
		0x4544, 0x4746, 0x4948, 0x534A, 0x5554, 0x5756, 0x5958, 0x635A, 
		0x6564, 0x6766, 0x6968, 0x736A, 0x7574, 0x7776, 0x7978, 0x827A, 
		0x8483, 0x8685, 0x8887, 0x8A89, 0x9392, 0x9594, 0x9796, 0x9998, 
		0xA29A, 0xA4A3, 0xA6A5, 0xA8A7, 0xAAA9, 0xB3B2, 0xB5B4, 0xB7B6, 
		0xB9B8, 0xC2BA, 0xC4C3, 0xC6C5, 0xC8C7, 0xCAC9, 0xD3D2, 0xD5D4, 
		0xD7D6, 0xD9D8, 0xE2DA, 0xE4E3, 0xE6E5, 0xE8E7, 0xEAE9, 0xF3F2, 
		0xF5F4, 0xF7F6, 0xF9F8, 0xFFFA, 
		0xC4FF, 0x1F00, 0x0000, 0x0501, 0x0101, 0x0101, 0x0101, 0x0000, 
		0x0000, 0x0000, 0x0000, 0x0201, 0x0403, 0x0605, 0x0807, 0x0A09, 
		0xFF0B, 
		0xC4FF, 0x1F00, 0x0001, 0x0103, 0x0101, 0x0101, 0x0101, 0x0101, 
		0x0000, 0x0000, 0x0000, 0x0201, 0x0403, 0x0605, 0x0807, 0x0A09, 
		0xFF0B
	};

	*size = sizeof(tables);
	return tables;
}

/* load some huffman and quantisation tables into the VRJ chip ready
   for JPEG compression */
static void mchip_load_tables(void)
{
	int i;
	int size;
	u16 *tables = mchip_tables(&size);

	for (i=0;i<size/2;i++) {
		writel(tables[i], mchip_base + MCHIP_VRJ_TABLE_DATA);
	}
}


static void mchip_vrj_setup(u8 mode)
{
	mchip_set(MCHIP_VRJ_BUS_MODE, 5);
	mchip_set(MCHIP_VRJ_SIGNAL_ACTIVE_LEVEL, 0x1f);
	mchip_set(MCHIP_VRJ_PDAT_USE, 1);
	mchip_set(MCHIP_VRJ_IRQ_FLAG, 0x20);
	mchip_set(MCHIP_VRJ_MODE_SPECIFY, mode);
	mchip_set(MCHIP_VRJ_NUM_LINES, mchip_vsize());
	mchip_set(MCHIP_VRJ_NUM_PIXELS, mchip_hsize());
	mchip_set(MCHIP_VRJ_NUM_COMPONENTS, 0x1b);
	mchip_set(MCHIP_VRJ_LIMIT_COMPRESSED_LO, 0xFFFF);
	mchip_set(MCHIP_VRJ_LIMIT_COMPRESSED_HI, 0xFFFF);
	mchip_set(MCHIP_VRJ_COMP_DATA_FORMAT, 0xC);
	mchip_set(MCHIP_VRJ_RESTART_INTERVAL, 0);
	mchip_set(MCHIP_VRJ_SOF1, 0x601);
	mchip_set(MCHIP_VRJ_SOF2, 0x1502);
	mchip_set(MCHIP_VRJ_SOF3, 0x1503);
	mchip_set(MCHIP_VRJ_SOF4, 0x1596);
	mchip_set(MCHIP_VRJ_SOS,  0x0ed0);

	mchip_load_tables();
}



/* compress one frame into a buffer */
int mchip_compress_frame(u8 *buf, int bufsize)
{
	int ret;

	mchip_vrj_setup(0x3f);
	sdelay(50);

	mchip_set(MCHIP_HIC_MODE, MCHIP_HIC_MODE_STILL_COMP);
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);
	
	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);

	ret = mchip_comp_read_frame(buf, bufsize);

	while (mchip_read(MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_BUSY) sdelay(1);

	return ret;
}


/* uncompress one image into a buffer */
int mchip_uncompress_frame(u8 *img, int imgsize, u8 *buf, int bufsize)
{
	mchip_vrj_setup(0x3f);
	sdelay(50);

	mchip_set(MCHIP_HIC_MODE, MCHIP_HIC_MODE_STILL_DECOMP);
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);
	
	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);

	return mchip_comp_read_frame(buf, bufsize);
}


/* start continuous capture */
void mchip_cont_compression_start(void)
{
	mchip_hic_stop();
	mchip_vrj_setup(0x3f);
	mchip_set(MCHIP_HIC_MODE, MCHIP_HIC_MODE_CONT_COMP);
	mchip_set(MCHIP_HIC_CMD, MCHIP_HIC_CMD_START);

	while (mchip_read(MCHIP_HIC_CMD)) sdelay(1);

	printf("continuous compressed capture started\n");
}

int mchip_cont_compression_read(u8 *buf, int bufsize)
{
	return mchip_comp_read_frame(buf, bufsize);
}

void mchip_subsample(int sub)
{
	subsample = sub;
	mchip_set(MCHIP_MCC_R_SAMPLING, subsample);
	mchip_set(MCHIP_MCC_R_XRANGE, mchip_hsize());
	mchip_set(MCHIP_MCC_R_YRANGE, mchip_vsize());
	mchip_set(MCHIP_MCC_B_XRANGE, mchip_hsize());
	mchip_set(MCHIP_MCC_B_YRANGE, mchip_vsize());
}


void mchip_init(void)
{
      u32 mem;
      int error;
      u8 revision;
      u32 val;
      int fd;
      u8 irq;
      int mchip_dev;

      mchip_dev = pci_find_device(PCI_VENDOR_ID_KAWASAKI, PCI_DEVICE_ID_KAWASAKI_MCHIP);
      if (mchip_dev == -1) {
	      printf("failed to find mchip\n");
	      exit(1);
      }

      pci_config_read_u32(mchip_dev, PCI_BASE_ADDRESS_0, &mem);

      if (mem == 0) {
	      mem = pci_read_base_address(PCI_VENDOR_ID_KAWASAKI, PCI_DEVICE_ID_KAWASAKI_MCHIP);
	      if (mem) printf("Got base address from devices\n");
      }

      if (mem == 0) {
	      printf("No device base address for mchip - exiting\n");
	      exit(1);
      }

      /* check if the chip is powered on */
      pci_config_read_u32(mchip_dev, MCHIP_PCI_POWER_CSR, &val);

      if ((val & 3) != 0) {
	      u32 intetc;
	      printf( "mchip_init: turning device on\n");
	      pci_config_read_u32(mchip_dev, PCI_INTERRUPT_LINE, &intetc);
	      pci_config_write_u32(mchip_dev, MCHIP_PCI_POWER_CSR, 0);
	      sdelay(100);
	      pci_config_write_u32(mchip_dev, PCI_BASE_ADDRESS_0, mem);
	      pci_config_write_u32(mchip_dev, PCI_INTERRUPT_LINE, intetc);
      }

      /* Read the revision byte. */
      error = pci_config_read_u8(mchip_dev, PCI_REVISION_ID, &revision);

      /* Enable response in PCI memory space and enable PCI bus mastering. */
      error = pci_config_write_u16(mchip_dev, PCI_COMMAND,
				   PCI_COMMAND_MEMORY|
				   PCI_COMMAND_MASTER|
				   PCI_COMMAND_INVALIDATE);

      pci_config_write_u8(mchip_dev, PCI_CACHE_LINE_SIZE, 8);
      pci_config_write_u8(mchip_dev, PCI_LATENCY_TIMER, 64);


      /* we don't want interrupts */
      pci_config_read_u8(mchip_dev, PCI_INTERRUPT_LINE, &irq);
      printf("mchip_init(): KL5A72002 rev. %d, base %p, irq %d\n", 
	      revision, (void *) mem, irq);

      fd = open("/dev/mem", O_RDWR|O_SYNC);
      if (fd == -1) {
	      perror("/dev/mem");
	      exit(1);
      }
      mchip_base = mmap(0, MCHIP_MM_REGS + (mem & (PAGE_SIZE-1)),
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, mem & ~(PAGE_SIZE-1));
      if (mem & (PAGE_SIZE-1)) {
	      mchip_base += (mem & (PAGE_SIZE-1));
      }
      close(fd);

      /* Ask the camera to perform a soft reset. */
      pci_config_write_u16(mchip_dev, MCHIP_PCI_SOFTRESET_SET, 1);

      /* wait for HIC command register to auto clear. */
      while (mchip_read(MCHIP_HIC_CMD) & MCHIP_HIC_CMD_BITS) sdelay(5);

      /* Wait for HIC to become idle. */
      while (mchip_read(MCHIP_HIC_STATUS) & MCHIP_HIC_STATUS_BUSY) 
	      sdelay(10);

      sdelay(10);
      mchip_set(MCHIP_VRJ_SOFT_RESET, 1);
      sdelay(10);
      mchip_set(MCHIP_MM_INTA, 0x0);

      sdelay(600); 

      mchip_set(MCHIP_MM_PCI_MODE, 5);

      mchip_dma_setup();

      close(mchip_dev);

      printf("mchip open\n");
}

void mchip_shutdown(void)
{
	mchip_hic_stop();
	mchip_set(MCHIP_MM_INTA, 0x0);
}

/* see if the camera is powered on */
int mchip_camera_enabled(void)
{
	u16 val = readl(mchip_base + MCHIP_HIC_CTL);
	return (val & MCHIP_HIC_CTL_MCORE_RDY) != 0;
}

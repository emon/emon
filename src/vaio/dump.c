/* mchip register dump functions for picturebook 

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
#include "capture.h"


void dump_pci_if_registers(void)
{
	unsigned int i;
	u32 val;

	printf("==== PCI IF REGISTERS ====\n");


	val = readl(mchip_base + MCHIP_MM_PCI_MODE);
	printf( "PCI mode [0x%08x]: ", val);
	if (val & MCHIP_MM_PCI_MODE_RETRY)
		printf( "RETRY ");
	if (val & MCHIP_MM_PCI_MODE_MASTER)
		printf( "MASTER ");
	if (val & MCHIP_MM_PCI_MODE_READ_LINE)
		printf( "READ_LINE ");
	printf( "\n");

	val = readl(mchip_base + MCHIP_MM_INTA);
	printf( "IRQ status [0x%08x]: ", val);
	if (val & MCHIP_MM_INTA_MCC)
		printf( "MCC interrupt, ");
	if (val & MCHIP_MM_INTA_VRJ)
		printf( "VRJ interrupt, ");
	if (val & MCHIP_MM_INTA_HIC_1)
		printf( "(de)compression done%s, ",
			       (val & MCHIP_MM_INTA_HIC_1_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MM_INTA_HIC_END)
		printf( "all frames done%s, ",
			       (val & MCHIP_MM_INTA_HIC_1_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MM_INTA_JPEG)
		printf( "decompress. error%s, ",
			       (val & MCHIP_MM_INTA_HIC_1_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MM_INTA_CAPTURE)
		printf( "capture end, ");
	if (val & MCHIP_MM_INTA_PCI_ERR)
		printf( "PCI error%s, ",
			       (val & MCHIP_MM_INTA_HIC_1_MASK)
			       ? "" : " [INVALID]");
	printf( "\n");

	printf( "Page Table Address: 0x%08x\n",
		       readl(mchip_base + MCHIP_MM_PT_ADDR));


	for (i = 0; i < 4; i++) {
		/* Two interpretations of thsi field: continuous compression,
		   or continuous pixel output */
		val = readl(mchip_base + MCHIP_MM_FIR(i));

		printf(
			       "Frame Information %u (compression) [0x%08x]: ",
			       i, val);
		printf( "%s possible, ",
			       (val & MCHIP_MM_FIR_RDY) ? "read" : "write");
		printf( "dwords at end of page %u, ",
			       (val & MCHIP_MM_FIR_C_ENDL_MASK)
			       >> MCHIP_MM_FIR_C_ENDL_SHIFT);
		printf( "end page table ID %u, ",
			       (val & MCHIP_MM_FIR_C_ENDP_MASK)
			       >> MCHIP_MM_FIR_C_ENDP_SHIFT);
		printf( "start page table ID %u, ",
			       (val & MCHIP_MM_FIR_C_STARTP_MASK)
			       >> MCHIP_MM_FIR_C_STARTP_SHIFT);
		printf( "# lost frames %u\n",
			       (val & MCHIP_MM_FIR_FAILFR_MASK)
			       >> MCHIP_MM_FIR_FAILFR_SHIFT);
	
		printf( "Frame Information %u (pixel): ",
			       i);
		if (val & MCHIP_MM_FIFO_IDLE)
			printf( "end frame, ");
		else
			printf( "possible to use frame, ");
		printf( "start page table ID %u, ",
			       (val & MCHIP_MM_FIR_C_STARTP_MASK)
			       >> MCHIP_MM_FIR_C_STARTP_SHIFT);
		printf( "# lost frames %u\n",
			       (val & MCHIP_MM_FIR_FAILFR_MASK)
			       >> MCHIP_MM_FIR_FAILFR_SHIFT);
	}


	val = readl(mchip_base + MCHIP_MM_FIFO_STATUS);
	printf( "FIFO status [0x%08x]: ", val);
	switch (val & MCHIP_MM_FIFO_MASK) {
	case MCHIP_MM_FIFO_IDLE:
		printf( "IDLE(0), ");
		break;

	case MCHIP_MM_FIFO_IDLE1:
		printf( "IDLE(1), ");
		break;

	case MCHIP_MM_FIFO_WAIT:
		printf( "wait request, ");
		break;

	case MCHIP_MM_FIFO_READY:
		printf( "data ready, ");
		break;
	}
	printf( "\n");
}

void print_bit(const char *prefix,
		      int regpos, const char *true, const char *false)
{
	u32 val = readl(mchip_base + regpos);

	printf("%s[0x%08x]: ", prefix, val);
	if (val & 0x01)
		printf("%s\n", true);
	else
		printf("%s\n", false);
}



void dump_hic_registers(void)
{
	u32 val;

	printf("==== HIC REGISTERS ====\n");

	print_bit("PCI mode",
			 MCHIP_HIC_HOST_USEREQ,
			 "host request to use MCORE",
			 "TAKE_PIC valid");

	print_bit( "TP busy",
			 MCHIP_HIC_TP_BUSY,
			 "under camera capture",
			 "idle");

	print_bit( "PIC saved",
			 MCHIP_HIC_PIC_SAVED,
			 "camera captured picture",
			 "no picture");

	print_bit( "LOWPOWER",
			 MCHIP_HIC_LOWPOWER,
			 "Internal MCORE clock stopped",
			 "normal mode");

	val = readl(mchip_base + MCHIP_HIC_CTL);
	printf( "HIC control [0x%08x]: ", val);
	if (val & MCHIP_HIC_CTL_SOFT_RESET)
		printf( "reset to MCORE, ");
	else
		printf( "normal mode, ");
	if (val & MCHIP_HIC_CTL_MCORE_RDY)
		printf( "MCORE_RDY high");
	else
		printf( "MCORE_RDY low");
	printf( "\n");

	printf( "HIC command: ");
	val = readl(mchip_base + MCHIP_HIC_CMD);
	printf( "0x%02X\n", val);

	val = readl(mchip_base + MCHIP_HIC_MODE);
	printf( "HIC mode [0x%08x]: ", val);
	switch (val & 0x0F) {
	case MCHIP_HIC_MODE_NOOP:
		printf( "NO-OP");
		break;
	case MCHIP_HIC_MODE_STILL_CAP:
		printf( "Still picture capture");
		break;
	case MCHIP_HIC_MODE_DISPLAY:
		printf( "Display");
		break;
	case MCHIP_HIC_MODE_STILL_COMP:
		printf( "Still image compression");
		break;
	case MCHIP_HIC_MODE_STILL_DECOMP:
		printf( "Still image decompression");
		break;
	case MCHIP_HIC_MODE_CONT_COMP:
		printf( "Continuous capture & compression");
		break;
	case MCHIP_HIC_MODE_CONT_DECOMP:
		printf( "Continuous decompression & display");
		break;
	case MCHIP_HIC_MODE_STILL_OUT:
		printf( "Still image output");
		break;
	case MCHIP_HIC_MODE_CONT_OUT:
		printf( "Continuous image output");
		break;
	}
	printf( "\n");

	val = readl(mchip_base + MCHIP_HIC_STATUS);
	printf( "HIC status [0x%08x]: ", val);
	if (val & MCHIP_HIC_STATUS_MCC_RDY)
		printf( "access MCC reg possible, ");
	else
		printf( "access MCC reg impossible, ");
	if (val & MCHIP_HIC_STATUS_VRJ_RDY)
		printf( "access VRJ reg possible, ");
	else
		printf( "access VRJ reg impossible, ");
	if (val & MCHIP_HIC_STATUS_CAPDIS)
		printf( "capture/display in progress, ");
	else
		printf( "no capture/display in progress, ");
	if (val & MCHIP_HIC_STATUS_COMPDEC)
		printf( "(de)compress in progress, ");
	else
		printf( "no (de)compress in progress, ");
	if (val & MCHIP_HIC_STATUS_BUSY)
		printf( "HIC busy");
	else
		printf( "HIC not busy");
	printf( "\n");

	
	printf( "MPEG processing rate: %u\n",
		       readl(mchip_base + MCHIP_HIC_S_RATE) & 0x1F);
	

	print_bit( "PCI video format",
			 MCHIP_HIC_PCI_VFMT,
			 "{V,Y',U,Y}",
			 "{Y',V,Y,U}");
}

void dump_mcc_registers(void)
{
	u32 val;

	printf("==== MCC REGISTERS ====\n");

	val = readl(mchip_base + MCHIP_MCC_CMD);
	printf( "MCC command [0x%08x]: ", val);
	switch (val & 0x0F) {
	case MCHIP_MCC_CMD_INITIAL:
		printf( "Initial setup");
		break;
	case MCHIP_MCC_CMD_IIC_START_SET:
		printf( "IIC setting start");
		break;
	case MCHIP_MCC_CMD_IIC_END_SET:
		printf( "IIC setting end");
		break;
	case MCHIP_MCC_CMD_FM_WRITE:
		printf( "FM writing");
		break;
	case MCHIP_MCC_CMD_FM_READ:
		printf( "FM read-out");
		break;
	case MCHIP_MCC_CMD_FM_STOP:
		printf( "FM access stop");
		break;
	case MCHIP_MCC_CMD_CAPTURE:
		printf( "Capture");
		break;
	case MCHIP_MCC_CMD_DISPLAY:
		printf( "Display");
		break;
	case MCHIP_MCC_CMD_END_DISP:
		printf( "Display end");
		break;
	case MCHIP_MCC_CMD_STILL_COMP:
		printf( "Still image compression");
		break;
	case MCHIP_MCC_CMD_STILL_DECOMP:
		printf( "Still image decompression");
		break;
	case MCHIP_MCC_CMD_STILL_OUTPUT:
		printf( "Still image output");
		break;
	case MCHIP_MCC_CMD_CONT_OUTPUT:
		printf( "Continuous image output");
		break;
	case MCHIP_MCC_CMD_CONT_COMP:
		printf( "Continuous image compression");
		break;
	case MCHIP_MCC_CMD_CONT_DECOMP:
		printf( "Continuous image decompression");
		break;
	case MCHIP_MCC_CMD_RESET:
		printf( "MCC softreset");
		break;
	}

	printf( "MCC read = %u\n",
		       readl(mchip_base + MCHIP_MCC_MCC_RD));

	val = readl(mchip_base + MCHIP_MCC_STATUS);
	printf( "MCC status [0x%08x]: ", val);
	if (val & MCHIP_MCC_STATUS_CAPT)
		printf( "capturing, ");
	else
		printf( "not capturing, ");
	if (val & MCHIP_MCC_STATUS_DISP)
		printf( "displaying, ");
	else
		printf( "not displaying, ");
	if (val & MCHIP_MCC_STATUS_COMP)
		printf( "compressing, ");
	else
		printf( "not compressing, ");
	if (val & MCHIP_MCC_STATUS_DECOMP)
		printf( "decompressing, ");
	else
		printf( "not decompressing, ");
	if (val & MCHIP_MCC_STATUS_MCC_WR)
		printf( "MCC WRREG ready, ");
	else
		printf( "MCC WRREG not ready, ");
	if (val & MCHIP_MCC_STATUS_MCC_RD)
		printf( "MCC RDREG ready, ");
	else
		printf( "MCC RDREG not ready, ");
	if (val & MCHIP_MCC_STATUS_IIC_WR)
		printf( "IIC WRREG ready, ");
	else
		printf( "IIC WRREG not ready, ");
	if (val & MCHIP_MCC_STATUS_OUTPUT)
		printf( "Outputting picture");
	else
		printf( "Picture output end");
	printf( "\n");

	val = readl(mchip_base + MCHIP_MCC_SIG_POLARITY);
	printf( "Video signal polarity [0x%08x]: ", val);
	if (val & MCHIP_MCC_SIG_POL_VS_H)
		printf( "VS high, ");
	else
		printf( "VS low, ");
	if (val & MCHIP_MCC_SIG_POL_HS_H)
		printf( "HS high, ");
	else
		printf( "HS low, ");
	if (val & MCHIP_MCC_SIG_POL_DOE_H)
		printf( "DOE high");
	else
		printf( "DOE low");
	printf( "\n");

	val = readl(mchip_base + MCHIP_MCC_IRQ);
	printf( "MIRQ [0x%08x]: ", val);
	if (val & MCHIP_MCC_IRQ_CAPDIS_STRT)
		printf( "capture/display started%s, ",
			       (val & MCHIP_MCC_IRQ_CAPDIS_STRT_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MCC_IRQ_CAPDIS_END)
		printf( "capture/display ended%s, ",
			       (val & MCHIP_MCC_IRQ_CAPDIS_END_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MCC_IRQ_COMPDEC_STRT)
		printf( "(de)compression started%s, ",
			       (val & MCHIP_MCC_IRQ_COMPDEC_STRT_MASK)
			       ? "" : " [INVALID]");
	if (val & MCHIP_MCC_IRQ_COMPDEC_END)
		printf( "(de)compression ended%s, ",
			       (val & MCHIP_MCC_IRQ_COMPDEC_END_MASK)
			       ? "" : " [INVALID]");

	printf("\n");
	printf( "HSTART = %u\n",
		       readl(mchip_base + MCHIP_MCC_HSTART) & 0x3FF);

	printf( "VSTART = %u\n",
		       readl(mchip_base + MCHIP_MCC_VSTART) & 0x3FF);

	printf( "HCOUNT = %u\n",
		       readl(mchip_base + MCHIP_MCC_HCOUNT) & 0x3FF);

	printf( "VCOUNT = %u\n",
		       readl(mchip_base + MCHIP_MCC_VCOUNT) & 0x3FF);

	printf( "Capture/display XBASE = %u\n",
		       readl(mchip_base + MCHIP_MCC_R_XBASE) & 0x3FF);

	printf( "Capture/display YBASE = %u\n",
		       readl(mchip_base + MCHIP_MCC_R_YBASE) & 0x3FF);

	printf( "Capture/display XRANGE = %u\n",
		       readl(mchip_base + MCHIP_MCC_R_XRANGE) & 0x3FF);

	printf( "Capture/display YRANGE = %u\n",
		       readl(mchip_base + MCHIP_MCC_R_YRANGE) & 0x3FF);

	printf( "(de)compression XBASE = %u\n",
		       readl(mchip_base + MCHIP_MCC_B_XBASE) & 0x3FF);

	printf( "(de)compression YBASE = %u\n",
		       readl(mchip_base + MCHIP_MCC_B_YBASE) & 0x3FF);

	printf( "(de)compression XRANGE = %u\n",
		       readl(mchip_base + MCHIP_MCC_B_XRANGE) & 0x3FF);

	printf( "(de)compression YRANGE = %u\n",
		       readl(mchip_base + MCHIP_MCC_B_YRANGE) & 0x3FF);

	print_bit("Sub sampling",
		  MCHIP_MCC_R_SAMPLING,
		  "1:4",
		  "none");
}


void dump_vrj_registers(void)
{
	printf("======= VRJ REGISTERS =========\n");

	printf("Mode specify: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_MODE_SPECIFY));
	printf("VRJ status: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_STATUS) & 0xFF);
	printf("VRJ IRQ: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_IRQ_FLAG));
	printf("Bus mode: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_BUS_MODE));
	printf("PDAT use: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_PDAT_USE));
	printf("Limit compressed lo: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_LIMIT_COMPRESSED_LO));
	printf("Limit compressed hi: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_LIMIT_COMPRESSED_HI));
	printf("Comp data format: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_COMP_DATA_FORMAT));
	printf("Error report: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_ERROR_REPORT));
	printf("IRQ Flag: 0x%08x\n", readl(mchip_base + MCHIP_VRJ_IRQ_FLAG));
}

void dump_all_registers(void)
{
	dump_pci_if_registers();
	dump_hic_registers();
	dump_mcc_registers();
	dump_vrj_registers();
}

/*
 * (C) Copyright 2007
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#if defined(CONFIG_440)

#include <ppc4xx.h>
#include <ppc440.h>
#include <asm/io.h>
#include <asm/mmu.h>

typedef struct region {
	unsigned long base;
	unsigned long size;
	unsigned long tlb_word2_i_value;
} region_t;

static int add_tlb_entry(unsigned long base_addr,
			 unsigned long tlb_word0_size_value,
			 unsigned long tlb_word2_i_value)
{
	int i;
	unsigned long tlb_word0_value;
	unsigned long tlb_word1_value;
	unsigned long tlb_word2_value;

	/* First, find the index of a TLB entry not being used */
	for (i=0; i<PPC4XX_TLB_SIZE; i++) {
		tlb_word0_value = mftlb1(i);
		if ((tlb_word0_value & TLB_WORD0_V_MASK) == TLB_WORD0_V_DISABLE)
			break;
	}
	if (i >= PPC4XX_TLB_SIZE)
		return -1;

	/* Second, create the TLB entry */
	tlb_word0_value = TLB_WORD0_EPN_ENCODE(base_addr) | TLB_WORD0_V_ENABLE |
		TLB_WORD0_TS_0 | tlb_word0_size_value;
	tlb_word1_value = TLB_WORD1_RPN_ENCODE(base_addr) | TLB_WORD1_ERPN_ENCODE(0);
	tlb_word2_value = TLB_WORD2_U0_DISABLE | TLB_WORD2_U1_DISABLE |
		TLB_WORD2_U2_DISABLE | TLB_WORD2_U3_DISABLE |
		TLB_WORD2_W_DISABLE | tlb_word2_i_value |
		TLB_WORD2_M_DISABLE | TLB_WORD2_G_DISABLE |
		TLB_WORD2_E_DISABLE | TLB_WORD2_UX_ENABLE |
		TLB_WORD2_UW_ENABLE | TLB_WORD2_UR_ENABLE |
		TLB_WORD2_SX_ENABLE | TLB_WORD2_SW_ENABLE |
		TLB_WORD2_SR_ENABLE;

	/* Wait for all memory accesses to complete */
	sync();

	/* Third, add the TLB entries */
	mttlb1(i, tlb_word0_value);
	mttlb2(i, tlb_word1_value);
	mttlb3(i, tlb_word2_value);

	/* Execute an ISYNC instruction so that the new TLB entry takes effect */
	asm("isync");

	return 0;
}

static void program_tlb_addr(unsigned long base_addr, unsigned long mem_size,
			     unsigned long tlb_word2_i_value)
{
	int rc;
	int tlb_i;

	tlb_i = tlb_word2_i_value;
	while (mem_size != 0) {
		rc = 0;
		/* Add the TLB entries in to map the region. */
		if (((base_addr & TLB_256MB_ALIGN_MASK) == base_addr) &&
		    (mem_size >= TLB_256MB_SIZE)) {
			/* Add a 256MB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_256MB, tlb_i)) == 0) {
				mem_size -= TLB_256MB_SIZE;
				base_addr += TLB_256MB_SIZE;
			}
		} else if (((base_addr & TLB_16MB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_16MB_SIZE)) {
			/* Add a 16MB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_16MB, tlb_i)) == 0) {
				mem_size -= TLB_16MB_SIZE;
				base_addr += TLB_16MB_SIZE;
			}
		} else if (((base_addr & TLB_1MB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_1MB_SIZE)) {
			/* Add a 1MB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_1MB, tlb_i)) == 0) {
				mem_size -= TLB_1MB_SIZE;
				base_addr += TLB_1MB_SIZE;
			}
		} else if (((base_addr & TLB_256KB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_256KB_SIZE)) {
			/* Add a 256KB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_256KB, tlb_i)) == 0) {
				mem_size -= TLB_256KB_SIZE;
				base_addr += TLB_256KB_SIZE;
			}
		} else if (((base_addr & TLB_64KB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_64KB_SIZE)) {
			/* Add a 64KB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_64KB, tlb_i)) == 0) {
				mem_size -= TLB_64KB_SIZE;
				base_addr += TLB_64KB_SIZE;
			}
		} else if (((base_addr & TLB_16KB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_16KB_SIZE)) {
			/* Add a 16KB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_16KB, tlb_i)) == 0) {
				mem_size -= TLB_16KB_SIZE;
				base_addr += TLB_16KB_SIZE;
			}
		} else if (((base_addr & TLB_4KB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_4KB_SIZE)) {
			/* Add a 4KB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_4KB, tlb_i)) == 0) {
				mem_size -= TLB_4KB_SIZE;
				base_addr += TLB_4KB_SIZE;
			}
		} else if (((base_addr & TLB_1KB_ALIGN_MASK) == base_addr) &&
			   (mem_size >= TLB_1KB_SIZE)) {
			/* Add a 1KB TLB entry */
			if ((rc = add_tlb_entry(base_addr, TLB_WORD0_SIZE_1KB, tlb_i)) == 0) {
				mem_size -= TLB_1KB_SIZE;
				base_addr += TLB_1KB_SIZE;
			}
		} else {
			printf("ERROR: no TLB size exists for the base address 0x%0X.\n",
				base_addr);
		}

		if (rc != 0)
			printf("ERROR: no TLB entries available for the base addr 0x%0X.\n",
				base_addr);
	}

	return;
}

/*
 * Program one (or multiple) TLB entries for one memory region
 *
 * Common usage for boards with SDRAM DIMM modules to dynamically
 * configure the TLB's for the SDRAM
 */
void program_tlb(u32 start, u32 size, u32 tlb_word2_i_value)
{
	region_t region_array;

	region_array.base = start;
	region_array.size = size;
	region_array.tlb_word2_i_value = tlb_word2_i_value;	/* en-/disable cache */

	/* Call the routine to add in the tlb entries for the memory regions */
	program_tlb_addr(region_array.base, region_array.size,
			 region_array.tlb_word2_i_value);

	return;
}

#endif /* CONFIG_440 */

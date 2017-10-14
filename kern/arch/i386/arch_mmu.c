/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIM.
 *
 * AIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/early_kmmap.h>
#include <aim/mmu.h>

#define ENTRY_NUM (1024)
#define _4MB_OFFSET (22)
#define MY_EARLY_PAGE_SIZE (1<<_4MB_OFFSET)

bool early_mapping_valid(struct early_mapping *entry)
{
	if((((entry->paddr)>>_4MB_OFFSET)<<_4MB_OFFSET)==entry->addr)
		return true;
	return false;
}

void page_index_clear(pgindex_t *index)
{
	pde_t clear_item=0;
	for(uint32_t i=0;i<ENTRY_NUM;i++)
		index[i]=clear_item;
	return;
}

int page_index_early_map(pgindex_t *index, addr_t paddr,
	void *vaddr, size_t length)
{
	pde_t entry_base=0x183;        //binary:  0000 0001 1000 0011
	pde_t paddr_split[2];
	paddr_split=&paddr;
	pde_t phy_page_index=paddr_split[1]>>_4MB_OFFSET;
	uint32_t page_offset=((uint32_t)vaddr)>>_4MB_OFFSET;
	//index=(pgindex_t *)((uint32_t)index+page_offset*4);
	uint32_t page_num=(uint32_t)(length/MY_EARLY_PAGE_SIZE)+(length%MY_EARLY_PAGE_SIZE==0);
	for(uint32_t i=0;i<page_num;i++,phy_page_index+=1)
		index[page_offset+i]=((phy_page_index<<_4MB_OFFSET)&entry_base);
	return 0;
	
	return -1;
}

void mmu_init(pgindex_t *boot_page_index)
{
	// enable paging
	__asm__ __volatile__ (
		"movl %%ebx,%%cr0;"
		"andl $0xfffff000,%%ebx;"
		"movl %%ebx,%%cr3;"
		"movl %%cr4,%%ebx;"
		"orl $0x90,%%ebx;"
		"movl %%ebx,%%cr4;"
		"movl %%cr0,%%ebx;"
		"orl $0x80000000,%%ebx;"
		"movl %%ebx,%%cr0;"
		::"ebx"(boot_page_index):"memory"
	);
	
}


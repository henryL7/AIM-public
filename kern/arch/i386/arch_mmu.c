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
#define _4KB_PDE_OFFSET (22)
#define _4KB_PTE_OFFSET (12)


bool early_mapping_valid(struct early_mapping *entry)
{
	if((((entry->paddr)>>_4MB_OFFSET)<<_4MB_OFFSET)==entry->paddr)
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
	pde_t paddr_split;
	paddr_split=(pde_t)paddr;
	pde_t phy_page_index=paddr_split>>_4MB_OFFSET;
	uint32_t page_offset=((uint32_t)vaddr)>>_4MB_OFFSET;
	//index=(pgindex_t *)((uint32_t)index+page_offset*4);
	uint32_t page_num=(uint32_t)(length/MY_EARLY_PAGE_SIZE)+(length%MY_EARLY_PAGE_SIZE!=0);
	for(uint32_t i=0;i<page_num;i++,phy_page_index+=1)
		index[page_offset+i]=((phy_page_index<<_4MB_OFFSET)|entry_base);
	return 0;
	
	return -1;
}

void mmu_init(pgindex_t *boot_page_index)
{
	// enable paging
	__asm__ __volatile__ (
		"andl $0xfffff000,%%ebx;"
		"movl %%ebx,%%cr3;"
		"movl %%cr4,%%ebx;"
		"orl $0x90,%%ebx;"
		"movl %%ebx,%%cr4;"
		"movl %%cr0,%%ebx;"
		"orl $0x80000000,%%ebx;"
		"movl %%ebx,%%cr0;"
		"nop;"
		::"ebx"(boot_page_index):"memory"
	);

}

pgindex_t *init_pgindex(void)
{
	extern char page_table_start[];
	uint32_t page_table_align=(uint32_t)page_table_start;
	page_table_align=(page_table_align/PAGE_SIZE)*PAGE_SIZE+(page_table_align%PAGE_SIZE!=0)*PAGE_SIZE;
	static uint32_t count=0;
	return (pgindex_t*)(page_table_align+(count++)*PAGE_SIZE);
}

void destroy_pgindex(pgindex_t *pgindex)
{
	pde_t clear_item=0;
	for(uint32_t i=0;i<ENTRY_NUM;i++)
		pgindex[i]=clear_item;
	return;
}

void vaddr_preprocessing(void *vaddr,size_t size,uint32_t *results)
{
	results[0]=(uint32_t)((size/PAGE_SIZE)+(size%PAGE_SIZE!=0));  //pte_num
	results[1]=(((uint32_t)vaddr)>>_4KB_PTE_OFFSET)&0x3ff;  //pte_index
	results[2]=((uint32_t)vaddr)>>_4KB_PDE_OFFSET;   //pde_index
	results[3]=(results[0]+results[1])/ENTRY_NUM+((results[0]+results[1])%ENTRY_NUM!=0);  //pde_num
	return;
}
int map_pages(pgindex_t *pgindex, void *vaddr, addr_t paddr, size_t size,uint32_t flags)
{
	uint32_t pde_base=7;     // binary 00111
	uint32_t results[4]={0,0,0,0};
	vaddr_preprocessing(vaddr,size,results);
	uint32_t pte_num=results[0];
	uint32_t pte_index=results[1];
	uint32_t pde_index=results[2];
	uint32_t pde_num=results[3];
	pde_t paddr_split;
	paddr_split=(pde_t)paddr;
	pde_t phy_page_index=paddr_split>>_4KB_PTE_OFFSET;
	for(uint32_t i=0;i<pde_num;i++)
	{
		pgindex_t* pte_addr;
		if(((pgindex[i+pde_index])&(1))==0)
		{
			pte_addr=init_pgindex();
			pgindex[i+pde_index]=((uint32_t)pte_addr)|pde_base;
		}
		pte_addr=(pgindex_t*)((pgindex[i+pde_index])&0xfffff000);
		uint32_t pte_start=0,pte_end=0;
		if(i==0)
			pte_start=pte_index;
		else
			pte_start=0;
		if(i==pde_num-1)
		{
			pte_end=(pte_index+pte_num)%ENTRY_NUM;
			if(pte_end==0)
			pte_end=ENTRY_NUM;
		}
		else
			pte_end=ENTRY_NUM;
		for(uint32_t j=pte_start;j<pte_end;j++)
		{
			pte_addr[j]=((phy_page_index<<_4KB_PTE_OFFSET)|flags);
			phy_page_index+=1;
		}
	}
	return 0;
}

ssize_t unmap_pages(pgindex_t *pgindex, void *vaddr, size_t size, addr_t *paddr)
{
	uint32_t results[4]={0,0,0,0};
	vaddr_preprocessing(vaddr,size,results);
	uint32_t pte_num=results[0];
	uint32_t pte_index=results[1];
	uint32_t pde_index=results[2];
	uint32_t pde_num=results[3];
	ssize_t phy_size=0;
	for(uint32_t i=0;i<pde_num;i++)
	{
		pgindex_t* pte_addr;
		pte_addr=(pgindex_t*)((pgindex[i+pde_index])&0xfffff000);
		uint32_t pte_start=0,pte_end=0;
		if(i==0)
			pte_start=pte_index;
		else
			pte_start=0;
		if(i==pde_num-1)
		{
			pte_end=(pte_index+pte_num)%ENTRY_NUM;
			if(pte_end==0)
			pte_end=ENTRY_NUM;
		}
		else
			pte_end=ENTRY_NUM;
		for(uint32_t j=pte_start;j<pte_end;j++)
		{
			if(i==0&&j==pte_start)
			{
				if((pte_addr[j]&1)!=0)
					*paddr=pte_addr[j];
				else
					return -1;
			}
			if(((pte_addr[j])&(1))!=0)
				phy_size+=PAGE_SIZE;
			pte_addr[j]=0;
		}
		if(pte_start==0&&pte_end==ENTRY_NUM)
			pgindex[i+pde_index]=0;
	}
	return phy_size;	
}

int set_pages_perm(pgindex_t *pgindex, void *vaddr, size_t size, uint32_t flags)
{
	uint32_t results[4]={0,0,0,0};
	vaddr_preprocessing(vaddr,size,results);
	uint32_t pte_num=results[0];
	uint32_t pte_index=results[1];
	uint32_t pde_index=results[2];
	uint32_t pde_num=results[3];
	for(uint32_t i=0;i<pde_num;i++)
	{
		pgindex_t* pte_addr;
		pte_addr=(pgindex_t*)((pgindex[i+pde_index])&0xfffff000);
		uint32_t pte_start=0,pte_end=0;
		if(i==0)
			pte_start=pte_index;
		else
			pte_start=0;
		if(i==pde_num-1)
		{
			pte_end=(pte_index+pte_num)%ENTRY_NUM;
			if(pte_end==0)
			pte_end=ENTRY_NUM;
		}
		else
			pte_end=ENTRY_NUM;
		for(uint32_t j=pte_start;j<pte_end;j++)
		{
			pte_addr[j]=pte_addr[j]|flags;
		}
	}
	return 0;
}

ssize_t invalidate_pages(pgindex_t *pgindex, void *vaddr, size_t size,addr_t *paddr)
{
	uint32_t results[4]={0,0,0,0};
	vaddr_preprocessing(vaddr,size,results);
	uint32_t pte_num=results[0];
	uint32_t pte_index=results[1];
	uint32_t pde_index=results[2];
	uint32_t pde_num=results[3];
	ssize_t phy_size=0;
	for(uint32_t i=0;i<pde_num;i++)
	{
		pgindex_t* pte_addr;
		pte_addr=(pgindex_t*)((pgindex[i+pde_index])&0xfffff000);
		uint32_t pte_start=0,pte_end=0;
		if(i==0)
			pte_start=pte_index;
		else
			pte_start=0;
		if(i==pde_num-1)
		{
			pte_end=(pte_index+pte_num)%ENTRY_NUM;
			if(pte_end==0)
			pte_end=ENTRY_NUM;
		}
		else
			pte_end=ENTRY_NUM;
		for(uint32_t j=pte_start;j<pte_end;j++)
		{
			if(i==0&&j==pte_start)
			{
				if((pte_addr[j]&1)!=0)
					*paddr=pte_addr[j];
				else
					return -1;
			}
			if((pte_addr[j]&1)!=0)
				phy_size+=PAGE_SIZE;
			pte_addr[j]=pte_addr[j]&0;
		}
		if(pte_start==0&&pte_end==ENTRY_NUM)
			pgindex[i+pde_index]=pgindex[i+pde_index]&0xfffffffe;
	}
	return phy_size;	
}

int switch_pgindex(pgindex_t *pgindex)
{
	__asm__ __volatile__ ("movl %%ebx,%%cr3;"::"ebx"(pgindex):"memory");
	return 0;
}

pgindex_t *get_pgindex(void)
{
	pgindex_t *result=NULL;
	__asm__ __volatile__ ("movl %%cr3,%%ebx;":"=ebx"(result)::"memory");
	return result;
}




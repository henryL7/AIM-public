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
#include <aim/init.h>

#define phy_base (0)
#define gdt_num (3)
#define Kern_Base (0x80000000)
void arch_early_init(void)
{
    /*uint32_t gdt_table[2*gdt_num];
    gdt_table[0]=0;
    gdt_table[1]=0;
    gdt_table[2]=0xffff;
    gdt_table[3]=0xcf9a00;
    gdt_table[4]=0xffff;
    gdt_table[5]=0xcf9200;
    uint16_t gdtr[3]={0,0,0};
    gdtr[0]=gdt_num*8;
    gdtr[1]=(uint16_t)(((uint32_t)gdt_table)>>16);
    gdtr[2]=(uint16_t)((uint32_t)gdt_table);*/
    __asm__ __volatile__ (
      ".data:;"
      "GDT_TABLE:;"
      "0;"
      "0;"
      "0Xffff;"
      "0xcf9a00;"
      "0xffff;"
      "0xcf9200;"
      "GDTR:;"
      ".word (GDTR-GDT_TABLE-1);"
      ".long GDT_TABLE;"
      ".text:;"
      "lgdt GDTR;":::"memory"
    );
    extern char kernel_end[];
    size_t kernel_length=(unsigned long)kernel_end;
    kernel_length-=Kern_Base;
    struct early_mapping entry = {
		.paddr	= phy_base,
		.vaddr	= phy_base,
		.size	= kernel_length,
		.type	= EARLY_MAPPING_MEMORY
    };
    early_mapping_add(entry);
    early_mapping_add_memory(phy_base,kernel_length);
    pgindex_t* boot_pgindex=init_pgindex();
    page_index_init(boot_pgindex);
    return;
}


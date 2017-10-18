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
#include <aim/early_kmmap.h>
#include <aim/mmu.h>

#define phy_base (0)
#define gdt_num (3)
#define Kern_Base (0x80000000)
#define _4MB_PAGE_SIZE (1<<22)
#define MAP_LENGTH (0x40000000)
void arch_early_init(void)
{
    static uint32_t gdt_table[2*gdt_num];
    gdt_table[0]=0;
    gdt_table[1]=0;
    gdt_table[2]=0xffff;
    gdt_table[3]=0xcf9a00;
    gdt_table[4]=0xffff;
    gdt_table[5]=0xcf9200;
    __asm__ __volatile__ (
      "pushl %%ecx;"
      "pushw %%bx;"
      "lgdtl (%%esp);"
      "popw %%bx;"
      "popl %%ebx;"
      "movw $0x10,%%bx;"
      "movw %%bx,%%es;"
      "movw %%bx,%%ss;"
      "movw %%bx,%%gs;"
      "movw %%bx,%%ds;"
      ::"bx"(gdt_num*8),"ecx"(gdt_table):"memory"
    );
    extern char kernel_end[];
    size_t kernel_length=(size_t)kernel_end;
    kernel_length=premap_addr(kernel_length);
    struct early_mapping entry = {
		.paddr	= phy_base,
		.vaddr	= phy_base,
		.size	= MAP_LENGTH,
		.type	= EARLY_MAPPING_MEMORY
    };
    early_mapping_add(&entry);
    early_mapping_add_memory(phy_base,MAP_LENGTH);
    return;
}


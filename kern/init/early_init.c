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
#include <aim/console.h>
#include <aim/device.h>
#include <aim/early_kmmap.h>
#include <aim/init.h>
#include <aim/mmu.h>
#include <aim/panic.h>
#include <drivers/io/io-mem.h>
#include <drivers/io/io-port.h>
#include <platform.h>
#include <aim/simple_alloc.h>
#include <aim/phypage_alloc.h>

#define _4MB_PAGE_SIZE (1<<22)
static inline
int early_devices_init(void)
{
#ifdef IO_MEM_ROOT
	if (io_mem_init(&early_memory_bus) < 0)
		return EOF;
#endif /* IO_MEM_ROOT */

#ifdef IO_PORT_ROOT
	if (io_port_init(&early_port_bus) < 0)
		return EOF;
#endif /* IO_PORT_ROOT */
	return 0;
}
/*
inline void* get_bootpage_start(size_t size)
{
    uint32_t page_num=0;
    page_num=((uint32_t)size/PAGE_SIZE+((uint32_t)size%PAGE_SIZE!=0))*PAGE_SIZE;
    uint32_t stack_poi,astack_poi;
    __asm__ __volatile__ (
        "movl %%esp,%%eax;"
        :"=a"(stack_poi)::"memory"
    );
    astack_poi=stack_poi;
    if(stack_poi%PAGE_SIZE)
        astack_poi=((uint32_t)(stack_poi>>12)<<12)+PAGE_SIZE;
    uint32_t page_start=0;
    page_start=astack_poi+page_num*PAGE_SIZE;
    __asm__ __volatile__ ("subl %%eax,%%esp;"::"a"(page_start-stack_poi):"memory");
    return (void*)page_start;
}*/

__noreturn
void master_early_init(void)
{
	static uint32_t entry_count=0;
	char boot_vmm_start[PAGE_SIZE];
	if(entry_count!=0)
		goto next_line;
	entry_count++;
	/* clear address-space-related callback handlers */
	early_mapping_clear();
	mmu_handlers_clear();
	/* prepare early devices like memory bus and port bus */
	if (early_devices_init() < 0)
		goto panic;
	/* other preperations, including early secondary buses */
	arch_early_init();
	//get_mem_size();
	if (early_console_init(
		EARLY_CONSOLE_BUS,
		EARLY_CONSOLE_BASE,
		EARLY_CONSOLE_MAPPING
	) < 0)
		panic("Early console init failed.\n");
	kputs("Hello, world!\n");
	kprintf("Hello world\n");
	arch_init_mmu();
	/*__asm__ __volatile__(
		"subl $0xc,%%esp;"
		"sgdtl (%%esp);"
		"popw %%bx;"
		"popl %%eax;"
		"addl $0x80000000,%%eax;"
		"pushl %%eax;"
		"pushw %%bx;"
		"lgdtl (%%esp);"
		"ljmpl $0x8,$next_line;"
		"next_line:;"
		"movl $0x80000000,%%ebx;"
		"addl %%ebx,%%ebp;"
		"addl %%ebx,%%esp;"
		"movw  $0x10,%%ax;"     
		"movw  %%ax,%%ds;"
		"movw  %%ax,%%ss;"
		"movw  %%ax,%%es;":::"memory"
	);*/
	arch_jump_high();
next_line:
<<<<<<< HEAD
    /*{   
		uint32_t page_num=1;
		uint32_t stack_poi,astack_poi;
		__asm__ __volatile__ (
			"movl %%esp,%%eax;"
			:"=a"(stack_poi)::"memory"
		);
		astack_poi=stack_poi;
		if(stack_poi%PAGE_SIZE)
			astack_poi=((stack_poi)/PAGE_SIZE)*PAGE_SIZE+PAGE_SIZE;
		uint32_t page_start=0;
		page_start=astack_poi+page_num*PAGE_SIZE;
		__asm__ __volatile__ ("subl %%eax,%%esp;"::"a"(page_start-stack_poi):"memory");
		boot_vmm_start=page_start;
	}*/
=======
>>>>>>> 15c64576b5f25b850eba8213c55eb7bfb6487e1c
	simple_allocator_bootstrap((void*)boot_vmm_start,PAGE_SIZE);
	page_allocator_init();
	simple_allocator_init();
	add_memory_pages();
	test_two();
	test_one();
	test_two();
	goto panic;
panic:
	/*
	__asm__ __volatile__(
		"jmp L2;"
		"int_0x70:"
		"push %eax;"
		"push %edx;"
		"movb $0x0c,%al;"
		"outb $0x70;"
		"inb $0x71;"
		"movb $0x20,%al;"
		"outb $0xa0;"
		"outb $0x20;"
		"pop %edx;"
		"pop %eax;"
		"iret;"
		"L2:;"
		"movl $0x3e000380,%edx;"
		"movl $int_0x70,%eax;"
		"movw %ax,(%edx);"
		"shrl $0x10,%eax;"
		"movw %ax,0x6(%edx);"
		"movw $0x8e00,0x4(%edx);"
		"movw $0x8,0x2(%edx);"
		"movl $0x3e010000,%eax;"
		"movw $0x800,(%eax);"
		"movl $0x3e000000,0x2(%eax);"
		"lidtl 0x3e010000;"
		"movb $0xb,%al;"
		"orb $0x80,%al;"
		"outb $0x70;"
		"movb $0x40,%al;"
		"outb $0x71;"
		"movb $0xa,%al;"
		"outb $0x70;"
		"inb $0x71;"
		"orb $0xf,%al;"
		"outb $0x71;"
		"movb $0xc,%al;"
		"outb $0x70;"
		"inb $0x71;"
		"inb $0xa1;"
		"andb $0xfe,%al;"
		"outb $0xa1;"
		"sti;"
		"lazyloop:;"
		"hlt;"
		"jmp lazyloop;"
	); can be used only after the interrupt discriptor table is all set*/
    while(1);
}


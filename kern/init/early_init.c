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

__noreturn
void master_early_init(void)
{
	arch_early_init();
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
		"movl $0x3e000230,%edx;"
		"movl $int_0x70,%eax;"
		"movw %ax,0xc(%edx);"
		"shrl $0x10,%eax;"
		"movw %ax,(%edx);"
		"movw $0x8e00,0x4(%edx);"
		"movw $0x8,0x8(%edx);"
		"movl $0x3e010000,%eax;"
		"movw $800,(%eax);"
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
	); debug later*/
    while(1);
}


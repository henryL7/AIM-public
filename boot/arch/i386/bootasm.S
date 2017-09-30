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

.section .entry

.globl _start
.globl bss_start
.globl bss_end
_start:

cli /*disable interrupts*/

movw %cs,%ax
movw %ax,%ss    /*set up stack-segment register*/
xor %sp,%sp
xor %bp,%bp
movw $0x7c00,%sp  /*bios loading address*/
movw %sp,%bp    /*set up stack*/

movw bss_start,%bx   /*clear bss*/
movw bss_end,%dx
.clearbss:
movw $0x0,(%bx)
addw $0x2,%bx
cmpw %bx,%dx
jne .clearbss

inb $0x92
orb $0x2,%al
outb $0x92           /* open A20*/

movw $0x0,%ax
movw %ax,%ds
movw $0x7e00,%bx     /* get GDT's logic address*/

movl  $0x0,(%bx)
movl  $0x0,0x4(%bx)  /*create #0 discripter */

movl  $0xffff,0x8(%bx)
movl  $0xcf9800,0x12(%bx)  /* create discripter for code, flat mode is chosen here, maximum privilege given */

movl  $0xffff,0x16(%bx)
movl  $0xcf9200,0x20(%bx)  /* create discripter for data, flat mode is chosen here, maximum privilege given */

movw  $0x18,0x24(%bx)    /*set gdtr*/
movl  $0x7e00,0x26(%bx)

lgdt  0x24(%bx)      /*load gdtr*/

movw $0x1,0x30(%bx)
xor %ecx,%ecx
movw bootasm,%cx
addw $0x7c00,%cx
movl %ecx,0x32(%bx)
movl $0x7e30,%ecx  /*store longjump destination*/


movw  $0x2,%ax       /*set segement register for data*/
movw  %ax,%ds
movw  %ax,%es
movw  %ax,%fs
movw  %ax,%gs
movw  %ax,%ss


movl %cr0,%eax
orl $0x1,%eax
movl %eax,%cr0       /* set PE*/

ljmp *(%ecx)

.text

bootasm:
call bootmain
jmp bootasm


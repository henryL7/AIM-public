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
_start:

cli ;disable interrupts

movw %cs,%ax
movw %ax,%ss    /*set up stack-segment register*/
xor %sp,%sp
xor %bp,%bp
movw $0x7c00,%sp  /*bios loading address*/
movw %sp,%bp    /*set up stack*/

movw bss_start,%bx   /*clear bss*/
movw bss_end,%dx
.clearbss
movw $0x0,%ax
addw $0x2,%ax
cmpw %ax,%bx
jng .clearbss

movw $0x0,%ax
movw %ax,%ds
movw $0x7e00,%bx     /* get GDT's logic address*/

movl  $0x0,(%bx)
movl  $0x0,0x4(%bx)  /*create #0 discripter */

movl  $0xffff,0x8(%bx)
movl  $0xcf9a00,0x12(%bx)  /* create #1 discripter, flat mode is chosen here, maximum privilege given */

movw  $0x10,0x16(%bx)    /*set gdtr*/
movl  $0x7e00,0x18(%bx)

lgdt  0x16(%bx)      /*load gdtr*/

inb $0x92
or %al,0x2
outb $0x92           /* open A20*/

movl %cr0,%eax
or %eax,$0x1
movl %eax,%cr0       /* set PE*/


jmp bootasm

.text

bootasm:
call bootmain
jmp bootasm


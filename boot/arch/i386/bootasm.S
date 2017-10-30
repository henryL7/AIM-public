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
.code16

.globl _start
.weak bss_start
.weak bss_end
_start:

cli /*disable interrupts*/

cld
	/* zero out segment registers */
xorw	%ax, %ax
movw	%ax, %ds
movw	%ax, %es
movw	%ax, %ss
   /*set up stack-segment register*/
movw $0x7c00,%sp  /*bios loading address*/
movw %sp,%bp    /*set up stack*/

movw $bss_start,%bx   /*clear bss*/
movw $bss_end,%dx

clearbss:

movb $0x0,(%bx)
cmpw %bx,%dx
je L1
incw %bx
jmp clearbss


L1:
inb $0x92
orb $0x2,%al
outb $0x92           /* open A20*/

movw $0x7f00,%bx     /* get GDT's logic address*/

movl  $0x0,(%bx)
movl  $0x0,0x4(%bx)  /*create #0 discripter */

movl  $0xffff,0x8(%bx)
movl  $0xcf9a00,0xc(%bx)  /* create discripter for code, flat mode is chosen here, maximum privilege given */

movl  $0xffff,0x10(%bx)
movl  $0xcf9200,0x14(%bx)  /* create discripter for data, flat mode is chosen here, maximum privilege given */

movw  $0x18,0x18(%bx)    /*set gdtr*/
movl  $0x7f00,0x1a(%bx)

lgdtl  0x7f18     /*load gdtr*/


/*readin the rest of code of the bootloader*/
movb $0x1,%al
movw $0x1f2,%dx
outb (%dx)        /*read one block*/
incw %dx
incb %al
outb (%dx)        /*read 1# block, using CHS mode*/
xorb %al,%al
incw %dx
outb (%dx)
incw %dx  
outb (%dx) 
incw %dx 
outb (%dx)  
movb $0x20,%al     /*ask for read*/
incw %dx
outb (%dx)
.waits:
inb (%dx)
andb $0x88,%al
cmpb $0x08,%al
jnz .waits
movw $0x100,%bx
movw $0x1f0,%dx
movw $0x8000,%di  /*address for the start of the rest of later, in a hardcode way*/
.readw:
inw (%dx)
movw %ax,(%di)
addl $0x2,%di
subw $0x1,%bx
cmpw $0x0,%bx
jnz .readw


movl %cr0,%eax
orl $0x1,%eax
movl %eax,%cr0       /* set PE*/


ljmpl $0x8,$bootasm

.code32
.text

bootasm:

movw  $0x10,%ax       /*set segement register for data*/
movw  %ax,%ds
movw  %ax,%ss
movw  %ax,%es
call bootmain
jmp bootasm


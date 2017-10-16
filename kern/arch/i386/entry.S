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

.text

.globl _start
.globl master_early_entry
.weak bss_start
.weak bss_end

master_early_entry:
call _start

_start:

/*disable interrupts*/
cli 

/*cause all subsequent string operations to increment the index registers*/
cld


/*set up stack-segment register*/
movl $stack_addr,%esp  
subl $kern_offset,%esp
movl %esp,%ebp    

/*clear bss*/

movl $bss_start,%ebx
movl $bss_end,%edx

clearbss:

movb $0x0,(%ebx)
cmpl %ebx,%edx
je L1
incl %ebx
jmp clearbss

L1:

call master_early_init


.data

.local stack_addr 
stack_addr = 0xf0000000
.local kern_offset
kern_offset = 0x80000000

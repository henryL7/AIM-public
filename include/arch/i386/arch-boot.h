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

#ifndef _ARCH_BOOT_H
#define _ARCH_BOOT_H

#ifndef __ASSEMBLER__
#define PAGESIZE 512
typedef struct elf32hdr		elf_hdr;
typedef struct elf32_phdr	elf_phdr;

static inline
void read_disk(uint8_t quantities,uint32_t lba_number,uint32_t offset,void *address)
{
    __asm__ __volatile__ ("\
    push %%eax\
    outb $0x1f2\
    movl %%ebx,%%eax\
    outl $0x1f3\
    movb $0x20,%%al\
    outb $0x1f7\
    .waits:\
    inb $0x1f7\
    andb $0x88,%%al\
    cmpb $0x08,%%al\
    jnz .waits\
    pop %%eax\
    movl $0x200,%%ebx\
    mull %%ebx\
    xor %%ebx,%%ebx\
    movw %%cx,%%bx\
    subl %%ebx,%%eax\
    movl %%eax,%%ebx\
    .nextw:\
    inw $0x1f0\
    loop .nextw\
    .readw:\
    inw $0x1f0\
    movw %%ax,(%%edx)\
    addl $0x2,%%edx\
    subl $0x2,%%ebx\
    cmpl $0x0,%%ebx\
    jnz .readw\
    "::"a"(quantities),"b"(lba_number),"edx"(address),"cx"((uint16_t)offset):"memory"\
    );
    return;
}
#endif /* !__ASSEMBLER__ */

#endif /* !_ARCH_BOOT_H */


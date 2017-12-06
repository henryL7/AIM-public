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
/*
static inline
void read_disk(uint8_t quantities,uint32_t lba_number,void *address)
{
    __asm__ __volatile__ (
    "movl %%eax,%%ecx;"
    "outb $0x1f2;"
    "movl %%ebx,%%eax;"
    "shrl $0xc,%%eax;"
    "outb $0x1f3;"
    "movl %%ebx,%%eax;"
    "shrl $0x8,%%eax;"
    "outb $0x1f4;"
    "movl %%ebx,%%eax;"
    "shrl $0x4,%%eax;"
    "outb $0x1f5;"
    "movl %%ebx,%%eax;"
    "outb $0x1f6;"
    "movb $0x20,%%al;"
    "outb $0x1f7;"
    ".waits:;"
    "inb $0x1f7;"
    "andb $0x88,%%al;"
    "cmpb $0x08,%%al;"
    "jnz .waits;"
    "movl %%ecx,%%eax;"
    "movl $0x200,%%ebx;"
    "mull %%ebx;"
    "movl %%eax,%%ebx;"
    "movl %%edx,%%ecx;"
    "movw $0x1f0,%%dx;"
    ".readw:;"
    "inw (%%dx);"
    "movw %%ax,(%%ecx);"
    "addl $0x2,%%ecx;"
    "subl $0x2,%%ebx;"
    "cmpl $0x0,%%ebx;"
    "jnz .readw;"
    ::"a"(quantities),"b"(lba_number),"edx"(address):"memory"\
    );
    return;
}
*/
static 
uint8_t myinb(uint16_t port)
{
	uint8_t result=0;
	__asm__ __volatile__ ("inb (%%dx)":"=a"(result):"d"(port):"memory");
	return result;
}

static 
uint16_t myinw(uint16_t port)
{
	uint16_t result=0;
	__asm__ __volatile__ ("inw (%%dx)":"=a"(result):"d"(port):"memory");
	return result;
}

static 
void myoutb(uint16_t port, uint8_t data)
{
    __asm__ __volatile__ ("outb (%%dx)"::"a"(data),"dx"(port):"memory");
}

/*static 
void myoutw(uint16_t port, uint8_t data)
{
    __asm__ __volatile__ ("inb (%%dx)"::"a"(data),"dx"(port):"memory");
}*/
#endif /* !__ASSEMBLER__ */

#endif /* !_ARCH_BOOT_H */


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
#include <aim/boot.h>
#include <elf.h>

#define BUFFER_SIZE 0x200
uint32_t KERN_LBA;
elf32_phdr_t *elfhead;
elf32hdr_t *elf32;
char buffer[BUFFER_SIZE];
uint8_t* temp;
inline static void read_block(uint32_t lba_number)
{
	uint16_t* address=(uint16_t*)buffer;
	uint16_t port=0x1f2;
	myoutb(port++,1);
	temp=(uint8_t*)&(lba_number);
	int i;
	for(i=0;i<4;i++,port++)
		myoutb(port,temp[i]);
	myoutb(port,0x20);
	while(!(myinb(port)&0x08));
	for(i=0;i<256;i++,address++)
		*address=myinw(0x1f0);
	return;
}
static void read_disk(uint32_t offset,uint32_t length,char *address) 
{
	uint32_t pageoff;
	pageoff=offset/PAGESIZE;
	uint32_t byteoff;
	byteoff=offset%PAGESIZE;
	uint32_t last_length;
	last_length=(length+byteoff)%PAGESIZE;
	uint32_t page_num;
	page_num=(length+byteoff)/PAGESIZE+(last_length!=0);
	pageoff+=KERN_LBA;
	uint32_t l;
	uint32_t j;
	for(uint32_t i=0;i<page_num;i++)
	{
		read_block(pageoff);
		j=(i==0?byteoff:0);
		l=(i==page_num-1?last_length:PAGESIZE);
		for(;j<l;j++,address++)
			*address=buffer[j];
	}
	return;
}

__noreturn
void bootmain(void)
{
	KERN_LBA=(uint32_t)mbr;
	read_disk(0,PAGESIZE,(char*)elf32);
	read_disk(elf32->e_phoff,(uint32_t)elf32->e_phnum*(uint32_t)elf32->e_phentsize,(char*)elfhead);
	unsigned int i=0;
	for(i=0;i<(unsigned int)elf32->e_phnum;i++,elfhead++)
	{
		if(elfhead->p_type!=PT_LOAD)
		continue;
		char* location=(char*)elfhead->p_vaddr;
		read_disk(elfhead->p_offset,elfhead->p_filesz,location);
		uint32_t bss_size=elfhead->p_memsz-elfhead->p_filesz;
		uint32_t padding=(elfhead->p_vaddr+elfhead->p_filesz)%elfhead->p_align;
		if(padding!=0)
		bss_size+=elfhead->p_align-padding;
		location=(char*)((uint32_t) location+elfhead->p_filesz);
		uint32_t i=0;
		for(i=0;i<bss_size;i++)
		{
			(*location)=0;
			location++;
		}
	}
	__asm__ __volatile__ ("jmp *(%%edx)"::"d"(elf32->e_entry):"memory");
	while (1);
}


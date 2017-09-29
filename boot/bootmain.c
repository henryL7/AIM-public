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

uint32_t KERN_LBA=0;
static void read_from_disk(uint32_t base,uint32_t offset,uint16_t length,char *address) 
{
	uint32_t pageoff;
	pageoff=offset/PAGESIZE;
	uint32_t byteoff;
	byteoff=offset%PAGESIZE;
	uint8_t page_num;
	page_num=(uint8_t)(((length+(uint16_t)byteoff)/PAGESIZE)+!!((length+(uint16_t)byteoff)%PAGESIZE));
	char* buffer;
	read_disk(page_num,	base+pageoff,(void*)buffer);
	address=&(buffer[byteoff]);
	return;
}

inline void program_loader(elf32_phdr_t *elfhead)
{
   if(elfhead->p_type!=PT_LOAD)
   return;
   char* location=(char*)elfhead->p_vaddr;
   read_from_disk(KERN_LBA,elfhead->p_offset,(uint16_t)elfhead->p_filesz,location);
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
   return;
}
__noreturn
void bootmain(void)
{
	uint32_t head=(uint32_t)(mbr[0]);
	uint32_t sector=(uint32_t)(mbr[1]);
	sector=sector>>2;
	uint32_t cylinder=(uint32_t)(mbr[1]);
	uint32_t mask=63;
	cylinder=cylinder&mask;
	cylinder=cylinder|(((uint32_t)(mbr[2]))<<2);
	KERN_LBA=head*63*255+cylinder*63+sector-1;
	char *readin;
	read_from_disk(KERN_LBA,0,PAGESIZE,readin);
	elf32hdr_t *elf32;
	elf32=(elf32hdr_t *)readin;
	char *buffer;
	read_from_disk(KERN_LBA,elf32->e_phoff,(uint16_t)(elf32->e_phnum*elf32->e_phentsize),buffer);
	elf32_phdr_t *elfhead;
	elfhead=(elf32_phdr_t *)buffer;
	unsigned int i=0;
	for(i=0;i<(unsigned int)elf32->e_phnum;i++)
	{
		program_loader(elfhead);
		elfhead++;
	}
	__asm__ __volitile__("\
       jmp *(%%edx)\
	"::"%edx"(elf32->e_entry):"memory"\
	);
	while (1);
}


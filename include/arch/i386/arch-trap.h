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

#ifndef _ARCH_TRAP_H
#define _ARCH_TRAP_H

#ifndef __ASSEMBLER__
#include<sys/types.h>

// use the impletation in xv6
struct trapframe {
    // registers as pushed by pusha
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t oesp;      
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  // rest of trap frame
  uint16_t gs;
  uint16_t padding1;
  uint16_t fs;
  uint16_t padding2;
  uint16_t es;
  uint16_t padding3;
  uint16_t ds;
  uint16_t padding4;  
  uint32_t trapno;

  // below here defined by x86 hardware
  uint32_t err;
  uint32_t eip;
  uint16_t cs;
  uint16_t padding5;
  uint32_t eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32_t esp;
  uint16_t ss;
  uint16_t padding6;
}__attribute__((packed));

// Gate descriptors for interrupts and traps, use the impletation in xv6
struct gatedesc {
  unsigned int off_15_0 : 16;   // low 16 bits of offset in segment
  unsigned int cs : 16;         // code segment selector
  unsigned int args : 5;        // # args, 0 for interrupt/trap gates
  unsigned int rsv1 : 3;        // reserved(should be zero I guess)
  unsigned int type : 4;        // type(STS_{TG,IG32,TG32})
  unsigned int s : 1;           // must be 0 (system)
  unsigned int dpl : 2;         // descriptor(meaning new) privilege level
  unsigned int p : 1;           // Present
  unsigned int off_31_16 : 16;  // high bits of offset in segment
}__attribute__((packed));

void trap_test1(void);
#endif	/* !__ASSEMBLER__ */

#endif /* _ARCH_TRAP_H */


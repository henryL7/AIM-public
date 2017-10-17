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

#define __LDSCRIPT__

/*
 * Using the C preprocessor, we allow includes and macro expansion in this
 * linker script.
 */

kern_offset = 0x80000000 ;
master_early_entry -= kern_offset ;
ENTRY(master_early_entry)
SECTIONS{
    .text 0x80010000 : AT (0x10000)
    {
        * ( .text ) ;
    }
    .rodata ALIGN(16):
    {
        *(.rodata)
    }
    .data ALIGN(16):
    {
        *(.data)
    }
    . = ALIGN (16) ;
    bss_start = . - kern_offset ;
    .bss :
    {
        *(.bss)
    }
    bss_end = . - kern_offset ;
    kernel_end = . ;
    page_table_start = . - kern_offset ;
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}


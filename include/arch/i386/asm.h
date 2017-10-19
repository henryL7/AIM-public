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

static inline
uint8_t inb(uint16_t port)
{
	uint8_t result=0;
	__asm__ __volatile__ ("inb (%%dx)":"=a"(result):"d"(port):"memory");
	return result;
}

static inline
uint16_t inw(uint16_t port)
{
	uint16_t result=0;
	__asm__ __volatile__ ("inw (%%dx)":"=a"(result):"d"(port):"memory");
	return result;
}

static inline
uint32_t inl(uint16_t port)
{
	uint32_t result=0;
	__asm__ __volatile__ ("inl (%%dx)":"=a"(result):"d"(port):"memory");
	return result;
}

static inline
void outb(uint16_t port, uint8_t data)
{
	__asm__ __volatile__ ("outb (%%dx)"::"a"(data),"dx"(port):"memory");
}

static inline
void outw(uint16_t port, uint16_t data)
{
	__asm__ __volatile__ ("outw (%%dx)"::"a"(data),"dx"(port):"memory");
}

static inline
void outl(uint16_t port, uint32_t data)
{
	__asm__ __volatile__ ("outl (%%dx)"::"a"(data),"dx"(port):"memory");
}


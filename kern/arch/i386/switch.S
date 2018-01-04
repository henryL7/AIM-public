/* Copyright (C) 2016 Gan Quan <coin2028@hotmail.com>
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

/* switch_regs(struct context *old, struct context *new) */

.globl switch_regs

.text
switch_regs:

  movl 4(%esp), %eax
  movl 8(%esp), %edx

/* save */

  movl %edi,(%eax)
  movl %esi,4(%eax)
  movl %ebx,8(%eax)
  movl %ebp,12(%eax)
  movl %esp,16(%eax)

/* load */
  movl (%edx),%edi
  movl 4(%edx),%esi
  movl 8(%edx),%ebx
  movl 12(%edx),%ebp
  movl 16(%edx),%esp
  ret



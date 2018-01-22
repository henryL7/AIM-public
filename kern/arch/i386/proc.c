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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/proc.h>
#include <aim/percpu.h>
#include <aim/smp.h>
#include <aim/trap.h>
#include <arch-trap.h>
#include <context.h>
#include <aim/sync.h>

#define SYS_CS (uint16_t)(1<<3)
#define SYS_DS (uint16_t)(2<<3)
#define USER_CS (uint16_t)((3<<3)|3)
#define USER_DS (uint16_t)((4<<3)|3)

extern lock_t sched_lock;
extern unsigned long __sched_intrflags;

void forkret(void)
{
	spin_unlock_irq_restore(&sched_lock, __sched_intrflags);
	proc_trap_return(current_proc);
	return;
}

extern void switch_regs(struct context *old, struct context *new);

static struct trapframe *__proc_trapframe(struct proc *proc)
{
	struct trapframe *tf;

	tf = (struct trapframe *)(kstacktop(proc) - sizeof(*tf));
	return tf;
}


static void __bootstrap_trapframe(struct trapframe *tf,
				   void *entry,
				   void *stacktop,
				   void *args)
{
	tf->cs=SYS_CS;
	tf->ds=SYS_DS;
	tf->gs=SYS_DS;
	tf->fs=SYS_DS;
	tf->es=SYS_DS;
	tf->eip=entry;
	tf->esp=stacktop;
	tf->ebp=tf->esp;
	*((uint32_t*)tf->esp)=forkret;
	tf->eflags=0x200;
}

static void __bootstrap_context(struct context *context, struct trapframe *tf)
{
	context->esp=tf->esp;
}

static void __bootstrap_user(struct trapframe *tf)
{
	tf->esp=premap_addr(tf->esp);
	tf->ebp=tf->esp;
	tf->cs=USER_CS;
	tf->ds=USER_DS;
	tf->gs=USER_DS;
	tf->fs=USER_DS;
	tf->es=USER_DS;
	tf->ss=USER_DS;
}

void __proc_ksetup(struct proc *proc, void *entry, void *args)
{
	struct trapframe *tf = __proc_trapframe(proc);
	__bootstrap_trapframe(tf, entry, kstacktop(proc)-sizeof(struct trapframe), args);
	__bootstrap_context(&(proc->context), tf);
}

void __proc_usetup(struct proc *proc, void *entry, void *stacktop, void *args)
{
	struct trapframe *tf = __proc_trapframe(proc);
	__bootstrap_trapframe(tf, entry, stacktop, args);
	__bootstrap_context(&(proc->context), tf);
	__bootstrap_user(tf);
}

void __prepare_trapframe_and_stack(struct trapframe *tf, void *entry,
    void *ustacktop, int argc, char *argv[], char *envp[])
{
}

void proc_trap_return(struct proc *proc)
{
	struct trapframe *tf = __proc_trapframe(proc);

	trap_return(tf);
}

void __arch_fork(struct proc *child, struct proc *parent)
{
	struct trapframe *tf_child = __proc_trapframe(child);
	struct trapframe *tf_parent = __proc_trapframe(parent);

	*tf_child = *tf_parent;
	/* fill return value here */

	__bootstrap_context(&(child->context), tf_child);
}

void switch_context(struct proc *proc)
{
	struct proc *current = current_proc;
	current_proc = proc;

	/* Switch page directory */
	switch_pgindex(proc->mm->pgindex);
	/* Switch general registers */
	switch_regs(&(current->context), &(proc->context));
}


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <arch/i386/arch-trap.h>
#include <aim/trap.h>
#include <arch/i386/arch-io.h>
#include <aim/panic.h>
#include <arch/i386/asm.h>
#define INTER_NUM 256

__attribute__((visibility("hidden")))
struct gatedesc IDT[INTER_NUM];

// use the impletation in xv6
// Set up a normal interrupt/trap gate descriptor.
// - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate.
//   interrupt gate clears FL_IF, trap gate leaves FL_IF alone
// - sel: Code segment selector for interrupt/trap handler
// - off: Offset in code segment for interrupt/trap handler
// - dpl: Descriptor Privilege Level -
//        the privilege level required for software to invoke
//        this interrupt/trap gate explicitly using an int instruction.
#define SETGATE(gate, istrap, sel, off, d)                \
{                                                         \
  (gate).off_15_0 = (uint32_t)(off) & 0xffff;                \
  (gate).cs = (sel);                                      \
  (gate).args = 0;                                        \
  (gate).rsv1 = 0;                                        \
  (gate).type = (istrap) ? 0xf : 0xe;           \
  (gate).s = 0;                                           \
  (gate).dpl = (d);                                       \
  (gate).p = 1;                                           \
  (gate).off_31_16 = (uint32_t)(off) >> 16;                  \
}

  

void init_8259A()
{
    // relocate the interuption table
    outb(0x20,0x11);
    outb(0x21,0x20);
    outb(0x21,0x04);
    outb(0x21,0x01);
    outb(0xa0,0x11);
    outb(0xa1,0x28);
    outb(0xa1,0x04);
    outb(0xa1,0x01);
    // mask interuption
    outb(0x21,0Xff);
    outb(0xa1,0xff);
    return;
}

void trap_init()
{
    init_8259A();
    void th0();
	void th1();
	void th2();
	void th3();
	void th4();
	void th5();
	void th6();
	void th7();
	void th8();
	void th9();
	void th10();
	void th11();
	void th12();
	void th13();
	void th14();
	void th15();
	void th16();

	void th32();
	void th33();
	void th34();
	void th35();
	void th36();
	void th37();
	void th38();
	void th39();
	void th40();
	void th41();
	void th42();
	void th43();
	void th44();
	void th45();
	void th46();
	void th47();

	void th0x80();
		
	SETGATE(IDT[0], 0, 0x8, th0, 0);
	SETGATE(IDT[1], 0, 0x8, th1, 0);
	SETGATE(IDT[2], 0, 0x8, th2, 0);
	SETGATE(IDT[3], 0, 0x8, th3, 3);
	SETGATE(IDT[4], 0, 0x8, th4, 0);
	SETGATE(IDT[5], 0, 0x8, th5, 0);
	SETGATE(IDT[6], 0, 0x8, th6, 0);
	SETGATE(IDT[7], 0, 0x8, th7, 0);
	SETGATE(IDT[8], 0, 0x8, th8, 0);
	SETGATE(IDT[9], 0, 0x8, th9, 0);
	SETGATE(IDT[10], 0, 0x8, th10, 0);
	SETGATE(IDT[11], 0, 0x8, th11, 0);
	SETGATE(IDT[12], 0, 0x8, th12, 0);
	SETGATE(IDT[13], 0, 0x8, th13, 0);
	SETGATE(IDT[14], 0, 0x8, th14, 0);
	SETGATE(IDT[15], 0, 0x8, th15, 0);
	SETGATE(IDT[16], 0, 0x8, th16, 0);
	
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER], 0, 0x8, th32, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+1], 0, 0x8, th33, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+2], 0, 0x8, th34, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+3], 0, 0x8, th35, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+4], 0, 0x8, th36, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+5], 0, 0x8, th37, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+6], 0, 0x8, th38, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+7], 0, 0x8, th39, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+8], 0, 0x8, th40, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+9], 0, 0x8, th41, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+10], 0, 0x8, th42, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+11], 0, 0x8, th43, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+12], 0, 0x8, th44, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+13], 0, 0x8, th45, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+14], 0, 0x8, th46, 0);
	SETGATE(IDT[IRQ_OFFSET+IRQ_TIMER+15], 0, 0x8, th47, 0);

    SETGATE(IDT[0x80], 0, 0x8, th0x80, 3);
    __asm__ __volatile__ (
        "pushl %%ecx;"
        "pushw %%bx;"
        "lgdtl (%%esp);"
        "popw %%bx;"
        "popl %%ebx;"
        "sti;"
        ::"bx"(INTER_NUM*8),"ecx"(IDT):"memory"
      );
	return; 
}

void trap(struct trapframe *tf)
{
    if(tf->trapno<17)
    {
        handle_interrupt(tf->trapno);
    }
    else if(tf->trapno==0x80)
    {
        tf->eax=handle_syscall(tf->eax,tf->ebx,tf->ecx,tf->edx,tf->esi,tf->edi,tf->ebp);
    }
    else
    panic("abort from interrupt\n");
    return;
}

void trap_return(struct trapframe *tf)
{
    uint32_t tfend=(uint32_t)tf+sizeof(struct trapframe);
    __asm__ __volatile__(
        "subl %%esp,%%eax;"
        "addl %%eax,%%esp;"
        "popal;"
        "popl %%gs;"
        "popl %%fs;"
        "popl %%es;"
        "popl %%ds;"
        "addl $0x8, %%esp;  # trapno and errcode"
        "iret;"
		::"a"(tfend):"memory");
	panic("trap_return fail");
}

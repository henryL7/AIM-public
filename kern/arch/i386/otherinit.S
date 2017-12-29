#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

.code16
.text

.globl _otherstart
.globl _otherend

.text
_otherstart:
_start:
cli  /* disable interrupts*/

cld
	/* zero out segment registers */
xorw	%ax, %ax
movw	%ax, %ds
movw	%ax, %es
movw	%ax, %ss
   /*set up stack-segment register*/
movw $0x7c00,%sp  /*bios loading address*/
movw %sp,%bp    /*set up stack*/

movw $0x7f00,%bx     /* get GDT's logic address*/

movl  $0x0,(%bx)
movl  $0x0,0x4(%bx)  /*create #0 discripter */

movl  $0xffff,0x8(%bx)
movl  $0xcf9a00,0xc(%bx)  /* create discripter for code, flat mode is chosen here, maximum privilege given */

movl  $0xffff,0x10(%bx)
movl  $0xcf9200,0x14(%bx)  /* create discripter for data, flat mode is chosen here, maximum privilege given */

movw  $0x18,0x18(%bx)    /*set gdtr*/
movl  $0x7f00,0x1a(%bx)

lgdtl  0x7f18     /*load gdtr*/

/*
movl $_start,%eax
movl $bootasm,%ecx
subl %eax,%ecx
addl $0x8000,%ecx
movl %ecx,0x22(%bx)
movw $0x8,0x20(%bx)
*/

movl %cr0,%eax
orl $0x1,%eax
movl %eax,%cr0       /* set PE*/


ljmpl $(0x8),$(bootasm-_otherstart+0x8000)

.code32
.text

bootasm:

movw  $0x10,%ax       /*set segement register for data*/
movw  %ax,%ds
movw  %ax,%ss
movw  %ax,%es

movl (0x8000-8),%ebx
andl $0xfffff000,%ebx
movl %ebx,%cr3
movl %cr4,%ebx
orl $0x90,%ebx
movl %ebx,%cr4
movl %cr0,%ebx
orl $0x80000000,%ebx
movl %ebx,%cr0
movl (0x8000-12),%esp
call *(0x8000-4)


_otherend:

jmp _otherend

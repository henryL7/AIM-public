/* use the impletation in JOS */


#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp alltraps

.text

TRAPHANDLER_NOEC(th0, 0)
TRAPHANDLER_NOEC(th1, 1)
TRAPHANDLER_NOEC(th2, 2)
TRAPHANDLER_NOEC(th3, 3)
TRAPHANDLER_NOEC(th4, 4)
TRAPHANDLER_NOEC(th5, 5)
TRAPHANDLER_NOEC(th6, 6)
TRAPHANDLER_NOEC(th7, 7)
TRAPHANDLER(th8, 8)
TRAPHANDLER_NOEC(th9, 9)  // reserved
TRAPHANDLER(th10, 10)
TRAPHANDLER(th11, 11)
TRAPHANDLER(th12, 12)
TRAPHANDLER(th13, 13)
TRAPHANDLER(th14, 14)
TRAPHANDLER_NOEC(th15, 15)  // reserved
TRAPHANDLER_NOEC(th16, 16)

//TRAPHANDLER(th17, T_ALIGN)
//TRAPHANDLER_NOEC(th18, T_MCHK)
//TRAPHANDLER_NOEC(th19, T_SIMDERR)

TRAPHANDLER_NOEC(th32, 32)  // timer: 32~47
TRAPHANDLER_NOEC(th33, 32+1)
TRAPHANDLER_NOEC(th34, 32+2)
TRAPHANDLER_NOEC(th35, 32+3)
TRAPHANDLER_NOEC(th36, 32+4)
TRAPHANDLER_NOEC(th37, 32+5)
TRAPHANDLER_NOEC(th38, 32+6)
TRAPHANDLER_NOEC(th39, 32+7)
TRAPHANDLER_NOEC(th40, 32+8)
TRAPHANDLER_NOEC(th41, 32+9)
TRAPHANDLER_NOEC(th42, 32+132)
TRAPHANDLER_NOEC(th43, 32+11)
TRAPHANDLER_NOEC(th44, 32+12)
TRAPHANDLER_NOEC(th45, 32+13)
TRAPHANDLER_NOEC(th46, 32+14)
TRAPHANDLER_NOEC(th47, 32+15)

TRAPHANDLER_NOEC(th0x80, 0x80)

alltraps:
pushl %ds
pushl %es
pushl %fs
pushl %gs
pushal

# Set up data segments.
movw $0x10, %ax
movw %ax, %ds
movw %ax, %es

# Call trap(tf), where tf=%esp
pushl %esp
call trap
# Call trap_return(tf), where tf=%esp
call trap_return

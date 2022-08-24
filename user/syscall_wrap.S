#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>

/*** exercise 4.1 ***/
LEAF(msyscall)
    // TODO: you JUST need to execute a `syscall` instruction and return from msyscall
	syscall		// step into kernel mode. 
	jr ra
	nop
END(msyscall)

LEAF(get_my_sp)
	move	v0, sp
	jr	ra
	nop
END(get_my_sp)

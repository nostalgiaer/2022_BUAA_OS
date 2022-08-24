#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
	.text
LEAF(_my_putchar)
	andi a0, a0, 0x00ff
	li	t0, 0xb0000000
	sb 	a0, 0(t0)
	j	ra
	nop
END(_my_putchar)


LEAF(_my_getchar)
	li	t0, 0xb0000000
	lb	v0, 0(t0)
	sb	v0, 0(t0)
	j	ra
	nop
END(_my_getchar)


LEAF(_my_exit)
	li	t0, 0xb0000010
	sb 	$0, 0(t0)
	j	ra
	nop
END(_my_exit)

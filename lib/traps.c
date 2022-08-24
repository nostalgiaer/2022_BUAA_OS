#include <trap.h>
#include <env.h>
#include <printf.h>

extern void handle_int();
extern void handle_reserved();
extern void handle_tlb();
extern void handle_sys();
extern void handle_mod();
unsigned long exception_handlers[32];

void trap_init()
{
    int i;

    for (i = 0; i < 32; i++) {
        set_except_vector(i, handle_reserved);
    }

    set_except_vector(0, handle_int);
    set_except_vector(1, handle_mod);
    set_except_vector(2, handle_tlb);
    set_except_vector(3, handle_tlb);
    set_except_vector(8, handle_sys);
}
void *set_except_vector(int n, void *addr)
{
    unsigned long handler = (unsigned long)addr;
    unsigned long old_handler = exception_handlers[n];
    exception_handlers[n] = handler;
    return (void *)old_handler;
}


/*** exercise 4.11 ***/
void
page_fault_handler(struct Trapframe *tf)
{
    struct Trapframe PgTrapFrame;
    extern struct Env *curenv;

/*	u_int instr = *(u_int *)tf->cp0_epc;
	u_int opcode = (instr >> 26) & 0x3f;
	u_int reg_rs = (instr >> 21) & 0x1f;
	u_int reg_rt = (instr >> 16) & 0x1f;

	curenv->env_runs++;
	printf("Env: 0x%x, Instr: 0x%x, opcode: %b, reg_rs: %d, reg_rt: %d, OUT_count: %d, COW_count: %d\n", 
		curenv->env_id, instr, opcode, reg_rs, reg_rt, curenv->env_nop, curenv->env_runs);
*/
    bcopy(tf, &PgTrapFrame, sizeof(struct Trapframe));
	/*  copy the all information to user space(UXSTACKTOP)  */
    if (tf->regs[29] >= (curenv->env_xstacktop - BY2PG) &&
        tf->regs[29] <= (curenv->env_xstacktop - 1)) {
            tf->regs[29] = tf->regs[29] - sizeof(struct  Trapframe);
            bcopy(&PgTrapFrame, (void *)tf->regs[29], sizeof(struct Trapframe));
        } else {
            tf->regs[29] = curenv->env_xstacktop - sizeof(struct  Trapframe);
            bcopy(&PgTrapFrame,(void *)curenv->env_xstacktop - sizeof(struct  Trapframe),sizeof(struct Trapframe));
        }
    // TODO: Set EPC to a proper value in the trapframe
	tf->cp0_epc = curenv->env_pgfault_handler;
	/*   then return to user mode  */
    return;
}

#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <stackframe.h>
#include <unistd.h>

/*** exercise 4.2 ***/
NESTED(handle_sys,TF_SIZE, sp)
    SAVE_ALL                            // Macro used to save trapframe
    CLI                                 // Clean Interrupt Mask
    nop
    .set at                             // Resume use of $at

    // TODO: Fetch EPC from Trapframe, calculate a proper value and store it back to trapframe.
	lw		t0, TF_EPC(sp)
	addiu	t0, t0, 4
	sw		t0, TF_EPC(sp)

    // TODO: Copy the syscall number into $a0.

	lw		a0, TF_REG4(sp)
    addiu   a0, a0, -__SYSCALL_BASE     // a0 <- relative syscall number
    sll     t0, a0, 2                   // t0 <- relative syscall number times 4
    la      t1, sys_call_table          // t1 <- syscall table base
    addu    t1, t1, t0                  // t1 <- table entry of specific syscall
    lw      t2, 0(t1)                   // t2 <- function entry of specific syscall

    lw      t0, TF_REG29(sp)            // t0 <- user's stack pointer
    lw      t3, 16(t0)                  // t3 <- the 5th argument of msyscall
    lw      t4, 20(t0)                  // t4 <- the 6th argument of msyscall

    // TODO: Allocate a space of six arguments on current kernel stack and copy the six arguments to proper location
	addiu	sp, sp, -24
	sw		t3, 16(sp)
	sw		t4, 20(sp)

    jalr    t2                          // Invoke sys_* function
    nop

    // TODO: Resume current kernel stack
	addiu	sp, sp, 24

/*	move    s3, t2			// function_entry = s3
	lw  t0, TF_REG29(sp)        //t0 = 用户栈指针

	lw  a0, TF_REG4(sp)
	lw  a1, TF_REG6(sp)
	lw  a2, TF_REG7(sp)  //load  

	lw  s1, TF_REG5(sp)     // cnt = s1
	li	t1, 3
	bge	s1, t1, save_a3
	nop	
	j	start
	nop
save_a3:
	lw  a3, 16(t0) 
start:
	addiu   s2, s1, 1
	sll     s2, s2, 2
	subu    sp, sp, s2      // 预留内核栈空间保存参数 4*(cnt+1)

	li  t1, 3       // have_saved_para = 3
	addiu   t0, t0, 20      //用户栈栈指针
	addiu   t2, sp, 16      //内核栈栈指针
	bge     t1, s1, copy_over  // t1 >= cnt -> 直接调用, 无需保留参数
	nop 
	loop:
    	beq     t1, s1, copy_over
		nop
    	lw      t3, 0(t0)
    	sw      t3, 0(t2)
    	addiu   t1, t1, 1
    	addiu   t0, t0, 4
    	addiu   t2, t2, 4
    	j       loop
    	nop
copy_over:
    jalr    s3
	nop
    addu    sp, sp, s2
*/
    sw      v0, TF_REG2(sp)             // Store return value of function sys_* (in $v0) into trapframe

    j       ret_from_exception          // Return from exeception
    nop
END(handle_sys)

sys_call_table:                         // Syscall Table
    .align 2
    .word sys_putchar
    .word sys_getenvid
    .word sys_yield
    .word sys_env_destroy
    .word sys_set_pgfault_handler
    .word sys_mem_alloc
    .word sys_mem_map
    .word sys_mem_unmap
    .word sys_env_alloc
    .word sys_set_env_status
    .word sys_set_trapframe
    .word sys_panic
    .word sys_ipc_can_send
    .word sys_ipc_recv
    .word sys_cgetc
	.word sys_thread_alloc
	.word sys_gettcbid
	.word sys_thread_destroy
	.word sys_set_thread_status
	.word sys_thread_join
	.word sys_sem_alloc
	.word sys_sem_free
	.word sys_sem_wait
	.word sys_sem_post
	.word sys_sem_getvalue
	.word sys_sem_open
	.word sys_sem_unlink
	.word sys_thread_detach

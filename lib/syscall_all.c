#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>
#include <semaphore.h>
#include <error.h>

extern char *KERNEL_SP;
extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
	//copy
	bcopy(
		  (void *)KERNEL_SP - sizeof(struct Trapframe), 
		  (void *)TIMESTACK - sizeof(struct Trapframe), 
		  sizeof(struct Trapframe));

	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}
	
	env->env_xstacktop = xstacktop;
	env->env_pgfault_handler = func;

	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;

	if (va >= UTOP || !(perm & PTE_V) || (perm & PTE_COW)) {
		return -E_INVAL; 
	}
	if ((ret = envid2env(envid, &env, 1)) < 0) {
		return ret;
	}
	if ((ret = page_alloc(&ppage)) < 0) {
		return ret;
	}
	if ((ret = page_insert(env->env_pgdir, ppage, va, perm)) < 0) {
		return ret;
	}
	return 0;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);
    //your code here
	if (srcva >= UTOP || dstva >= UTOP) {
		return -E_INVAL;	
	}
	if (!(perm & PTE_V)) {
		return -E_INVAL;
	}

	if ((ret = envid2env(srcid, &srcenv, 0)) < 0) {
		return ret;
	}
	if ((ret = envid2env(dstid, &dstenv, 0)) < 0) {
		return ret;
	}

	ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);
	if (ppage == NULL) {
		return -E_INVAL;
	}
	if ((ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm)) < 0) {
		return ret;
	}

	ret = 0;
	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret;
	struct Env *env;

	if (va >= UTOP) {
		return -E_INVAL;
	}

	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	page_remove(env->env_pgdir, va);
	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(int sysno, u_int thread_id)
{
	// Your code here.
	int r;
	struct Env *e;

	if ((thread_id & 0x7) >= THREAD_MAX) {
		return -E_INVAL;
	}
	if (curenv) {
		r = env_alloc(&e, curenv->env_id);
	} else {
		r = env_alloc(&e, 0);
	}

	if (r < 0) {
		return r;
	}

	struct Tcb *t = &e->env_threads[thread_id & 0x7];
	bzero((void *)&e->env_threads[0], sizeof(struct Tcb));

	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe), 
		  (void *)&t->tcb_tf, 
   		  sizeof(struct Trapframe));

	t->tcb_status = THREAD_NOT_RUNNABLE;
	t->tcb_pri = curtcb->tcb_pri;
	t->tcb_tf.regs[2] = 0;
	t->tcb_tf.pc = t->tcb_tf.cp0_epc;
	t->tcb_detach = UNJOINABLE;
	t->thread_id = mktcbid(t);
	e->env_parent_id = curenv->env_id;
	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

int sys_thread_alloc(void) {
	struct Tcb *t;
	int r;

	if ((r = thread_alloc(curenv, &t)) < 0) {
		return r;
	}
	t->tcb_pri = curenv->env_pri;
	t->tcb_status = THREAD_NOT_RUNNABLE;
	t->tcb_tf.regs[2] = 0;
	return t->thread_id;
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;
	
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE) {
		return -E_INVAL;
	}
	
	if ((ret = envid2env(envid, &env, 0)) < 0) {
		return ret;
	}

	struct Tcb *t = &env->env_threads[0];

	if (status == ENV_RUNNABLE && t->tcb_status != ENV_RUNNABLE) {
		LIST_INSERT_HEAD(&tcb_sched_list[0], t, tcb_sched_link);
	} else if (t->tcb_status == ENV_RUNNABLE && status != ENV_RUNNABLE) {
		LIST_REMOVE(t, tcb_sched_link);
	}
	t->tcb_status = status;
	
	return 0;
	//	panic("sys_env_set_status not implemented");
}

int sys_set_thread_status(int sysno, u_int envid, u_int tcbid, u_int status) {
	struct Tcb *t;
	struct Env *e;
	int r;

	if (status != THREAD_FREE && status != THREAD_RUNNABLE && status != THREAD_NOT_RUNNABLE) {
		return -E_INVAL;
	}

	if ((r = envid2env(envid, &e, 0)) < 0) {
		return r;
	}

	t = &e->env_threads[tcbid & 0x7];

	if (status == THREAD_RUNNABLE && t->tcb_status != THREAD_RUNNABLE) {
		LIST_INSERT_HEAD(&tcb_sched_list[0], t, tcb_sched_link);
	} else if (t->tcb_status == THREAD_RUNNABLE && status != THREAD_RUNNABLE) {
		LIST_REMOVE(t, tcb_sched_link);
	}
	t->tcb_status = status;
	
	return 0;
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)
{
	if (dstva >= UTOP) {
		return;
	}

	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_ipc_waiting_thread = curtcb->thread_id & 0x7;
	curtcb->tcb_status = THREAD_NOT_RUNNABLE;

	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env.
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{

	int r;
	struct Env *e;
	struct Page *p;
	struct Tcb *t;

	if (srcva >= UTOP) {
		return -E_INVAL;
	}

	if ((r = envid2env(envid, &e, 0)) < 0) {
		return r;
	}
	t = &e->env_threads[e->env_ipc_waiting_thread];

	if (e->env_ipc_recving == 0) {
		return -E_IPC_NOT_RECV;
	}

	e->env_ipc_recving = 0;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_ipc_perm = perm;
	t->tcb_status = THREAD_RUNNABLE;

	Pte *ppte;

	if (srcva) {
		p = page_lookup(curenv->env_pgdir, srcva, &ppte);
		if (p == NULL) {
			return -E_IPC_NOT_RECV;
		}

		if ((r = page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm)) < 0) {
			return r;
		}
	}

	return 0;
}

u_int sys_gettcbid(void) {
	if (!curtcb) {
		return -E_THREAD_NOT_FOUND;
	}
	return curtcb->thread_id;
}

u_int sys_thread_join(int sysno, u_int threadid, void **value_ptr) {
	struct Tcb *t;
	int r;
	if ((r = threadid2tcb(threadid, &t)) < 0) {
		return r;
	}
	if (t->tcb_detach != JOINABLE) {
		return -E_THREAD_NO_JOINABLE;
	}

	if (t->tcb_status == THREAD_FREE || t->tcb_status == THREAD_ZOMBIE) {
		if (value_ptr) {
			*value_ptr = t->tcb_exit_ptr;
		}
		if (t->tcb_status == THREAD_ZOMBIE) {
			thread_free(t);
		}
		return 0;
	}

	if (!LIST_EMPTY(&t->tcb_joined_list)) {
		return -E_THREAD_REPETITIVE_WAIT;
	}

	extern struct Tcb *curtcb;
	struct Tcb *wait_thread = LIST_FIRST(&curtcb->tcb_joined_list);
	if (wait_thread) {
		if (wait_thread == t) {
			return -E_THREAD_DEADLOCK;
		}
	}

	LIST_INSERT_TAIL(&t->tcb_joined_list, curtcb, tcb_joined_link);
	curtcb->tcb_join_value_ptr = value_ptr;
	curtcb->tcb_status = THREAD_NOT_RUNNABLE;
	struct Trapframe *tf = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
	t->tcb_detach = UNJOINABLE;
	tf->regs[2] = 0;
	tf->pc = tf->cp0_epc;
	sys_yield();
	return 0;
}

int sys_thread_destroy(int sysno, u_int tcbid) 
{
	int r;
	struct Tcb *t;

	if ((r = threadid2tcb(tcbid, &t)) < 0) {
		return r;
	}

	struct Tcb *temp;
	while (!LIST_EMPTY(&t->tcb_joined_list)) {
		temp = LIST_FIRST(&t->tcb_joined_list);
		LIST_REMOVE(temp, tcb_joined_link);
		if (temp->tcb_join_value_ptr) {
			*(temp->tcb_join_value_ptr) = t->tcb_exit_ptr;
		}
		temp->tcb_status = THREAD_RUNNABLE;
	}

	printf("[%08x] destroying tcb %08x\n", curenv->env_id, t->thread_id);	
	thread_destroy(t);
	return 0;
}

int sys_thread_detach(int sysno, u_int tcbid) {
	int r;
	struct Tcb *t;

	if ((r = threadid2tcb(tcbid, &t)) < 0) {
		return r;
	}

	if (t->tcb_detach != JOINABLE) {
		return -E_THREAD_NO_JOINABLE;
	}
	t->tcb_detach = UNJOINABLE;
	return 0;
}



/*	 Wzm's Semaphore System		*/
/////////////////////////////////

static int sem_look_up(sem_t *sem_no, sem_entry_t **sem) {
	sem_t sem_number = ((u_int)*sem_no) & 0x3ff;
	sem_entry_t *target;
	if (SEM_UNNAMED(*sem_no)) {
		if (sem_number > ENV_UNNAMED_SEM_MAX) {
			return -E_NO_SEM;
		}
		sem_entry_t *start = (sem_entry_t *)USEMS;
		target = &start[sem_number];
	} else {
		if (sem_number > NAMED_SEM_MAX) {
			return -E_NO_SEM;
		}
		target = &sems[sem_number];
	}
	if (!target->ref) {
		return -E_SEM_NOT_INIT;
	}

	if (target->envid != curenv->env_id && !target->pshared) {
		return -E_SEM_NO_PERM;
	}
	*sem = target;
	return 0;
}

u_int sys_sem_alloc(int sysno, sem_t *sem, int pshared, u_short value) 
{
	sem_entry_t *new_sem = LIST_FIRST(&sem_free_list);

	if (!new_sem) {
		return -E_NO_SEM;
	}

	new_sem->val = value;
	new_sem->pshared = SHARED;
	new_sem->ref = 1;
	new_sem->envid = curenv->env_id;
	LIST_REMOVE(new_sem, sem_entry_link);
	LIST_INIT(&new_sem->wait_list);
	
	*sem = ((u_int)(new_sem - sems)) | (1 << 11);
	return 0;
}

u_int sys_sem_free(int sysno, sem_t *sem_no) 
{
	if (!(*sem_no) || !sem_no) {
		return -E_SEM_NOT_INIT;
	}

	sem_entry_t *sem;
	int r;
	if ((r = sem_look_up(sem_no, &sem)) < 0) {
		return r;
	}

	if (!LIST_EMPTY(&sem->wait_list)) {
		return -E_RAMAIN_WAIT_PROCESS;	
	}

	sem->ref = 0;
	bzero((void *)sem, sizeof(sem_entry_t));
	if (SEM_NAMED(*sem_no)) {
		LIST_INSERT_HEAD(&sem_free_list, sem, sem_entry_link);
	}
	
	return 0;
}

int sys_sem_wait(int sysno, sem_t *sem_no, int block) {
	if (!(*sem_no) || !sem_no) {
		return -E_SEM_NOT_INIT;
	}

	sem_wait_t *sem_wait;
	sem_entry_t *sem;
	int r;
	if ((r = sem_look_up(sem_no, &sem)) < 0) {
		return r;
	}
	if (sem->ref == 0) {
		return -E_SEM_NOT_INIT;
	}

	if (sem->val > 0) {
		sem->val--;
		return 0;
	}
	
	sem_wait = LIST_FIRST(&sem_wait_free_list);
	if (LIST_EMPTY(&sem_wait_free_list)) {
		return -E_NO_FREE_WAIT;
	}
	LIST_REMOVE(sem_wait, sem_wait_link);
	sem_wait->thread_id = curtcb->thread_id;
	sem_wait->env_id = curenv->env_id;
	LIST_INSERT_HEAD(&sem->wait_list, sem_wait, sem_wait_link);
	curtcb->tcb_status = THREAD_NOT_RUNNABLE;

	struct Trapframe *tf = (struct Trapframe *)(KERNEL_SP - sizeof(struct Trapframe));
	tf->regs[2] = 0;
	tf->pc = tf->cp0_epc;

	sys_yield();
	return 0;
}

int sys_sem_post(int sysno, sem_t *sem_no) {
	if (!(*sem_no) || !sem_no) {
		return -E_SEM_NOT_INIT;
	}
	sem_entry_t *sem;
	int r;
	if ((r = sem_look_up(sem_no, &sem)) < 0) {
		return r;
	}

	if (sem->ref == 0) {
		return -E_NO_SEM;
	}

	if (LIST_EMPTY(&sem->wait_list)) {
		sem->val++;
		return 0;
	}
	sem_wait_t *sem_wait = LIST_FIRST(&sem->wait_list);
	LIST_REMOVE(sem_wait, sem_wait_link);
	LIST_INSERT_HEAD(&sem_wait_free_list, sem_wait, sem_wait_link);

	struct Env *e;
	struct Tcb *t;
	if (sem_wait->env_id == curenv->env_id) {
		t = &curenv->env_threads[sem_wait->thread_id & 0x7];
		t->tcb_status = THREAD_RUNNABLE;
	} else {
		if ((r = envid2env(sem_wait->env_id, &e, 0)) < 0) {
			return r;
		}
		t = &e->env_threads[sem_wait->thread_id & 0x7];
		t->tcb_status = THREAD_RUNNABLE;
	}
	return 0;
}

int sys_sem_getvalue(int sysno, sem_t *sem_no, int *valuep) {
	if (!(*sem_no) || !sem_no) {
		return -E_SEM_NOT_INIT;
	}

	sem_entry_t *sem;
	int r;
	if ((r = sem_look_up(sem_no, &sem)) < 0) {
		return r;
	}
	*valuep = (int)sem->val;
	return 0;
}

static int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q) {
		p++, q++;
	}

	if ((u_int)*p < (u_int)*q) {
		return -1;
	}

	if ((u_int)*p > (u_int)*q) {
		return 1;
	}

	return 0;
}

int sys_sem_open(int sysno, const char *name, u_int oflag, u_short mode, u_short val) {
	int i;
	sem_entry_t *sem = NULL;
	for (i = 0; i < NAMED_SEM_MAX; i++) {
		if (!strcmp((char *)sems[i].name, name)) {
			sem = &sems[i];
			break;
		}
	}
	
	if((oflag & O_CREAT) && (oflag & O_EXCL) && sem != NULL) {
		return -E_SEM_EXIST;
	}
	if (sem) {
		sem->ref++;
		return (int)(sem - sems) | (1 << 11);
	}

	sem_t new_sem;
	int r;
	if ((r = sys_sem_alloc(0, &new_sem, 0, val)) < 0) {
		return r;
	} 
	bcopy((void *)name, (void *)sems[SEMX(new_sem)].name, SEM_NAME_MAX);
	return (int)new_sem;
}

int sys_sem_unlink(int sysno, const char *name) {
	sem_entry_t *sem = NULL;
	int i;

	for (i = 0; i < NAMED_SEM_MAX; i++) {
		if (!strcmp((char *)sems[i].name, name)) {
			sem = &sems[i];
			break;
		}
	}

	if (!sem) {
		return -E_NO_SEM;
	}

	sem->ref--;
	sem_t sem_no = (u_int)(sem - sems) | (1 << 11);
	if (sem->ref == 0) {
		printf("free sem %d\n", sem_no);
		sys_sem_free(0, &sem_no);
	}
	return 0;
}


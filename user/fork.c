// implement fork from user space

#include "lib.h"
#include <mmu.h>
#include <env.h>

#define ROUNDDOWN(a, n)	(((u_long)(a)) & ~((n)-1))
#define TEST_ATFORK 0

//u_int user_getsp(void) {
//	u_int my_sp = get_my_sp();
//	return ROUNDDOWN(my_sp, 4096);
//}

/* ----------------- help functions ---------------- */

/* Overview:
 * 	Copy `len` bytes from `src` to `dst`.
 *
 * Pre-Condition:
 * 	`src` and `dst` can't be NULL. Also, the `src` area
 * 	 shouldn't overlap the `dest`, otherwise the behavior of this
 * 	 function is undefined.
 */
void user_bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	//	writef("~~~~~~~~~~~~~~~~ src:%x dst:%x len:%x\n",(int)src,(int)dst,len);
	max = dst + len;

	// copy machine words while possible
	if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
		while (dst + 3 < max) {
			*(int *)dst = *(int *)src;
			dst += 4;
			src += 4;
		}
	}

	// finish remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst = *(char *)src;
		dst += 1;
		src += 1;
	}

	//for(;;);
}

/* Overview:
 * 	Sets the first n bytes of the block of memory
 * pointed by `v` to zero.
 *
 * Pre-Condition:
 * 	`v` must be valid.
 *
 * Post-Condition:
 * 	the content of the space(from `v` to `v`+ n)
 * will be set to zero.
 */
void user_bzero(void *v, u_int n)
{
	char *p;
	int m;

	p = v;
	m = n;

	while (--m >= 0) {
		*p++ = 0;
	}
}
/*--------------------------------------------------------------*/

/* Overview:
 * 	Custom page fault handler - if faulting page is copy-on-write,
 * map in our own private writable copy.
 *
 * Pre-Condition:
 * 	`va` is the address which leads to a TLBS exception.
 *
 * Post-Condition:
 *  Launch a user_panic if `va` is not a copy-on-write page.
 * Otherwise, this handler should map a private writable copy of
 * the faulting page at correct address.
 */
/*** exercise 4.13 ***/
static void
pgfault(u_int va)
{
	u_int *tmp;
	int ret;
	//	writef("fork.c:pgfault():\t va:%x\n",va);
	va = ROUNDDOWN(va, BY2PG);

	u_int perm = (*vpt)[VPN(va)] & 0xfff;
	
	if (!(perm & PTE_COW)) {
		user_panic("pgfault: page is not copy-on-write");
	}

	perm = (perm & ~PTE_COW) | PTE_R;
	//map the new page at a temporary place
	tmp = (u_int *)USTACKTOP;
	if (syscall_mem_alloc(0, tmp, perm) < 0) {
		user_panic("pgfault: can't alloc page'");
	}
	user_bcopy((void *)va, (void *)tmp, BY2PG);
	//copy the content

	//map the page on the appropriate place
	if ((ret = syscall_mem_map(0, (u_int)tmp, 0, va, perm)) < 0) {
		user_panic("pgfault: page is not mapped!");
	}	

	//unmap the temporary place
	if ((ret = syscall_mem_unmap(0, (u_int)tmp)) < 0) {
		user_panic("pgfault: could not unmap the temporary page");
	}
}

/* Overview:
 * 	Map our virtual page `pn` (address pn*BY2PG) into the target `envid`
 * at the same virtual address.
 *
 * Post-Condition:
 *  if the page is writable or copy-on-write, the new mapping must be
 * created copy on write and then our mapping must be marked
 * copy on write as well. In another word, both of the new mapping and
 * our mapping should be copy-on-write if the page is writable or
 * copy-on-write.
 *
 * Hint:
 * 	PTE_LIBRARY indicates that the page is shared between processes.
 * A page with PTE_LIBRARY may have PTE_R at the same time. You
 * should process it correctly.
 */
/*** exercise 4.10 ***/
static void
duppage(u_int envid, u_int pn)
{
	u_int addr;
	u_int perm;

	addr = pn << PGSHIFT;
	perm = (*vpt)[pn] & 0xfff;
	
	if ((perm & PTE_R) && !(perm & PTE_LIBRARY) && !(perm & PTE_COW)) {
		perm = (perm & ~PTE_R) | PTE_COW;
	}

	u_int r;

	if ((r = syscall_mem_map(0, addr, envid, addr, perm) < 0)) {
		return;
	}
	if ((r = syscall_mem_map(0, addr, 0, addr, perm)) < 0) {
		return;
	}
	//	user_panic("duppage not implemented");
}

/* Overview:
 * 	User-level fork. Create a child and then copy our address space
 * and page fault handler setup to the child.
 *
 * Hint: use vpd, vpt, and duppage.
 * Hint: remember to fix "env" in the child process!
 * Note: `set_pgfault_handler`(user/pgfault.c) is different from
 *       `syscall_set_pgfault_handler`.
 */
/*** exercise 4.9 4.15***/
extern void __asm_pgfault_handler(void);
int
fork(void)
{
	// Your code here.
	u_int newenvid;
	extern struct Env *envs;
	extern struct Env *env;
	extern struct Tcb *tcb;	
	extern size_t _pthread_atfork_count;
	extern void (*_pthread_prepare_hooks[_PTHREAD_ATFORK_MAX])();
	extern void (*_pthread_parent_hooks[_PTHREAD_ATFORK_MAX])();
	extern void (*_pthread_child_hooks[_PTHREAD_ATFORK_MAX])();

	int i;
	int hook_count = (int)_pthread_atfork_count;
	void (*hook)();
	u_int now_thread_id = syscall_gettcbid();

	for (i = hook_count - 1; i >= 0; i--) {
		hook = _pthread_prepare_hooks[i];
		if (hook) {
			hook();
		}
	}

	set_pgfault_handler(pgfault);
	if ((newenvid = syscall_env_alloc(now_thread_id)) < 0) {
		return newenvid;
	} else if (newenvid == 0) {
		env = &envs[ENVX(syscall_getenvid())];
		tcb = &env->env_threads[now_thread_id & 0x7];
		for (i = 0; i < hook_count; i++) {
			hook = _pthread_child_hooks[i];
			if (hook) {
				hook();
			}
		}
		if (TEST_ATFORK) {
			return 0;
		}
	}

	for (i = 0; i < USTACKTOP; i += BY2PG) {
		if (i >= USTACKTOP - STACKSIZE*THREAD_MAX*BY2PG) {
			if (i < USTACKTOP - STACKSIZE*((now_thread_id&0x7)+1)*BY2PG 
				|| i >= USTACKTOP - STACKSIZE*(now_thread_id&0x7)*BY2PG) {
				continue;
			}			
		}
		if (((*vpd)[PDX(i)] & PTE_V) && ((*vpt)[VPN(i)] & PTE_V)) {
			duppage(newenvid, VPN(i));	
		}
	}

	//The parent installs pgfault using set_pgfault_handler
	if (syscall_mem_alloc(newenvid, UXSTACKTOP - BY2PG, PTE_V | PTE_R) < 0) {
		user_panic("fork: could not allocate exception stack");
	}
	if (syscall_set_pgfault_handler(newenvid, __asm_pgfault_handler, UXSTACKTOP) < 0) {
		user_panic("fork: could not set page fault handler");
	}
	//alloc a new alloc
	if (syscall_set_thread_status(newenvid, now_thread_id, THREAD_RUNNABLE) < 0) {
		user_panic("fork: could not start the child process");
	}
	for (i = 0; i < hook_count; i++) {
		hook = _pthread_parent_hooks[i];
		if (hook) {
			hook();
		}
	}


	return newenvid;
}

// Challenge!
int
sfork(void)
{
	user_panic("sfork not implemented");
	return -E_INVAL;
}

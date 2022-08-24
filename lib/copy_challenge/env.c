/* Notes written by Qian Liu <qianlxc@outlook.com>
  If you find any bug, please contact with me.*/

#include <mmu.h>
#include <error.h>
#include <env.h>
#include <kerelf.h>
#include <sched.h>
#include <pmap.h>
#include <printf.h>
#include <semaphore.h>

struct Env *envs = NULL;        // All environments
struct Env *curenv = NULL;            // the current env
struct Tcb *curtcb = NULL;				// the current tcb

static struct Env_list env_free_list;    // Free list
struct Tcb_list tcb_sched_list[2];

struct sem_head sem_free_list;
sem_entry_t sems[SEM_NSEMS_MAX];

struct sem_wait_head sem_wait_free_list;
sem_wait_t	sem_waits[SEM_WAIT_MAX];

extern Pde *boot_pgdir;
extern char *KERNEL_SP;

static u_int asid_bitmap[2] = {0}; // 64

void 
mips_sem_init() {
	int i;

	LIST_INIT(&sem_free_list);
	for (i = SEM_NSEMS_MAX - 1; i >= 0; i--) {
		LIST_INSERT_HEAD(&sem_free_list, &sems[i], sem_entry_link);
	}

	LIST_INIT(&sem_wait_free_list);
	for (i = SEM_WAIT_MAX - 1; i >= 0; i--) {
		LIST_INSERT_HEAD(&sem_wait_free_list, &sem_waits[i], sem_wait_link);
	}

}

/* Overview:
 *  This function is to allocate an unused ASID
 *
 * Pre-Condition:
 *  the number of running processes should be less than 64
 *
 * Post-Condition:
 *  return the allocated ASID on success
 *  panic when too many processes are running
 */


static u_int asid_alloc() {
    int i, index, inner;
    for (i = 0; i < 64; ++i) {
        index = i >> 5;
        inner = i & 31;
        if ((asid_bitmap[index] & (1 << inner)) == 0) {
            asid_bitmap[index] |= 1 << inner;
            return i;
        }
    }
    panic("too many processes!");
}

/* Overview:
 *  When a process is killed, free its ASID
 *
 * Post-Condition:
 *  ASID is free and can be allocated again later
 */
static void asid_free(u_int i) {
    int index, inner;
    index = i >> 5;
    inner = i & 31;
    asid_bitmap[index] &= ~(1 << inner);
}

/* Overview:
 *  This function is to make a unique ID for every env
 *
 * Pre-Condition:
 *  e should be valid
 *
 * Post-Condition:
 *  return e's envid on success
 */
u_int mkenvid(struct Env *e) {
    u_int idx = e - envs;
    u_int asid = asid_alloc();
    return (asid << (1 + LOG2NENV)) | (1 << LOG2NENV) | idx;
}


u_int mktcbid(struct Tcb *t) {
	struct Env *e = (struct Env *)ROUNDDOWN(t, BY2PG);
	u_int tcb_number = ((u_int)t - (u_int)e - BY2PG/2)/(BY2PG/16);
	return ((e->env_id << 3) | tcb_number);
}


int threadid2tcb(u_int threadid, struct Tcb **ptcb) {
	struct Tcb *t;
	struct Env *e;

	if (!threadid) {
		*ptcb = curtcb;
		return 0;
	}

	e = &envs[ENVX(threadid >> 3)];
	t = &e->env_threads[threadid & 0x7];
	
	*ptcb = t;
	return 0;
}

/* Overview:
 *  Convert an envid to an env pointer.
 *  If envid is 0 , set *penv = curenv; otherwise set *penv = envs[ENVX(envid)];
 *
 * Pre-Condition:
 *  penv points to a valid struct Env *  pointer,
 *  envid is valid, i.e. for the result env which has this envid, 
 *  its status isn't ENV_FREE,
 *  checkperm is 0 or 1.
 *
 * Post-Condition:
 *  return 0 on success,and set *penv to the environment.
 *  return -E_BAD_ENV on error,and set *penv to NULL.
 */
/*** exercise 3.3 ***/
int envid2env(u_int envid, struct Env **penv, int checkperm)
{
    struct Env *e;
    /* Hint: If envid is zero, return curenv.*/
    /* Step 1: Assign value to e using envid. */
	if (envid == 0) {
		*penv = curenv;
		return 0;
	} else {
		e = &envs[ENVX(envid)];	
	}

    if (e->env_id != envid) {
        *penv = 0;
        return -E_BAD_ENV;
    }
    /* Hints:
     *  Check whether the calling env has sufficient permissions
     *    to manipulate the specified env.
     *  If checkperm is set, the specified env
     *    must be either curenv or an immediate child of curenv.
     *  If not, error! */
    /*  Step 2: Make a check according to checkperm. */
	if (checkperm) {
		if (e != curenv && e->env_parent_id != curenv->env_id) {
			*penv = NULL;
			return -E_BAD_ENV;
		}
	}
	
    *penv = e;
    return 0;
}

/* Overview:
 *  Mark all environments in 'envs' as free and insert them into the env_free_list.
 *  Insert in reverse order,so that the first call to env_alloc() returns envs[0].
 *
 * Hints:
 *  You may use these macro definitions below:
 *      LIST_INIT, LIST_INSERT_HEAD
 */
/*** exercise 3.2 ***/
void
env_init(void)
{
    int i;
	int j;
    /* Step 1: Initialize env_free_list. */
	LIST_INIT(&env_free_list);
	
    /* Step 2: Traverse the elements of 'envs' array,
     *   set their status as free and insert them into the env_free_list.
     * Choose the correct loop order to finish the insertion.
     * Make sure, after the insertion, the order of envs in the list
     *   should be the same as that in the envs array. */
	for (i = 0; i < NENV; i++) {
		for (j = 0; j < THREAD_MAX; j++) {
			envs[i].env_threads[j].tcb_status = THREAD_FREE;
			envs[i].env_thread_count = 0;
		}
		LIST_INSERT_TAIL(&env_free_list, &envs[i], env_link);
	}

}


/* Overview:
 *  Initialize the kernel virtual memory layout for 'e'.
 *  Allocate a page directory, set e->env_pgdir and e->env_cr3 accordingly,
 *    and initialize the kernel portion of the new env's address space.
 *  DO NOT map anything into the user portion of the env's virtual address space.
 */
/*** exercise 3.4 ***/
static int
env_setup_vm(struct Env *e)
{

    int i, r;
    struct Page *p = NULL;
//    Pde *pgdir;

    /* Step 1: Allocate a page for the page directory
     *   using a function you completed in the lab2 and add its pp_ref.
     *   pgdir is the page directory of Env e, assign value for it. */
    if ((r = page_alloc(&p)) < 0) {
        panic("env_setup_vm - page alloc error\n");
        return r;
    }
	p->pp_ref++;
	e->env_pgdir = (Pde *)(page2kva(p));
	e->env_cr3 = page2pa(p);

    /* Step 2: Zero pgdir's field before UTOP. */
	for (i = 0; i < PDX(UTOP); i++) {
		e->env_pgdir[i] = 0;
	}

    /* Step 3: Copy kernel's boot_pgdir to pgdir. */

    /* Hint:
     *  The VA space of all envs is identical above UTOP
     *  (except at UVPT, which we've set below).
     *  See ./include/mmu.h for layout.
     *  Can you use boot_pgdir as a template?
     */
	for (i = PDX(UTOP); i < PDX(UVPT); i++) {
		e->env_pgdir[i] = boot_pgdir[i];
	}

    /* UVPT maps the env's own page table, with read-only permission.*/
    e->env_pgdir[PDX(UVPT)]  = e->env_cr3 | PTE_V;
    return 0;
}

/* Overview:
 *  Allocate and Initialize a new environment.
 *  On success, the new environment is stored in *new.
 *
 * Pre-Condition:
 *  If the new Env doesn't have parent, parent_id should be zero.
 *  env_init has been called before this function.
 *
 * Post-Condition:
 *  return 0 on success, and set appropriate values of the new Env.
 *  return -E_NO_FREE_ENV on error, if no free env.
 *
 * Hints:
 *  You may use these functions and macro definitions:
 *      LIST_FIRST,LIST_REMOVE, mkenvid (Not All)
 *  You should set some states of Env:
 *      id , status , the sp register, CPU status , parent_id
 *      (the value of PC should NOT be set in env_alloc)
 */
/*** exercise 3.5 ***/
int
env_alloc(struct Env **new, u_int parent_id)
{
    int r;
    struct Env *e;
	struct Tcb *t;

    /* Step 1: Get a new Env from env_free_list*/
	if (LIST_EMPTY(&env_free_list)) {
		return -E_NO_FREE_ENV;
	}
	e = LIST_FIRST(&env_free_list);

    /* Step 2: Call a certain function (has been completed just now) to init kernel memory layout for this new Env.
     *The function mainly maps the kernel address to this new Env address. */
	if ((r = env_setup_vm(e)) < 0) {
		return r;
	}

    /* Step 3: Initialize every field of new Env with appropriate values.*/
	e->env_id = mkenvid(e);
	e->env_parent_id = parent_id;

	if ((r = thread_alloc(e, &t)) < 0) {
		return r;
	}

    /* Step 5: Remove the new Env from env_free_list. */
	LIST_REMOVE(e, env_link);

	*new = e;
	return 0;
}

/*	alloc a new tcb , 
	return 0 on success, other on error */
int thread_alloc(struct Env *e, struct Tcb **new) {
	if (e->env_thread_count >= THREAD_MAX) {
		return -E_THREAD_MAX;
	}
	/* Step 1: look for free tcb in this env */
	u_int thread_number = e->env_thread_count;
	u_int try = 0;
	while (e->env_threads[thread_number].tcb_status != THREAD_FREE) {
		thread_number++;
		thread_number = thread_number % THREAD_MAX;
		if (++try >= THREAD_MAX) {
			return -E_THREAD_MAX;
		}
	}
	e->env_thread_count += 1;
	struct Tcb *t = &e->env_threads[thread_number];
	/*	Step 2: set some basic thread information	*/
	t->thread_id = mktcbid(t);
	t->tcb_detach = UNJOINABLE;
	printf("thread id is 2'b%b\n",t->thread_id);
	t->tcb_status = THREAD_RUNNABLE;
	t->tcb_tf.cp0_status = 0x1000100c;
	t->tcb_exit_ptr = (void *)0;
	/*	set thread stack according the thread_id  */
	t->tcb_tf.regs[29] = USTACKTOP - STACKSIZE*BY2PG*(t->thread_id & 0x7);
	/*	set cancel state & type	 */
	t->tcb_cancel_state = THREAD_CANCEL_ENABLE;
	t->tcb_cancel_type = THREAD_CANCEL_DEFERED;
	t->tcb_canceled = 0;
	t->tcb_exit_value = 0;
	t->tcb_exit_ptr = (void *)&t->tcb_exit_value;
	/*	Step 3: initialize the join list in this tcb */
	LIST_INIT(&t->tcb_joined_list);
	*new = t;

	return 0;
}


/* Overview:
 *   This is a call back function for kernel's elf loader.
 * Elf loader extracts each segment of the given binary image.
 * Then the loader calls this function to map each segment
 * at correct virtual address.
 *
 *   `bin_size` is the size of `bin`. `sgsize` is the
 * segment size in memory.
 *
 * Pre-Condition:
 *   bin can't be NULL.
 *   Hint: va may be NOT aligned with 4KB.
 *
 * Post-Condition:
 *   return 0 on success, otherwise < 0.
 */
/*** exercise 3.6 ***/
static int load_icode_mapper(u_long va, u_int32_t sgsize,
                             u_char *bin, u_int32_t bin_size, void *user_data)
{
    struct Env *env = (struct Env *)user_data;
    struct Page *p = NULL;
    u_long i;
    int r;
    u_long offset = va - ROUNDDOWN(va, BY2PG);

	int size = 0;
    /* Step 1: load all content of bin into memory. */
	if (offset > 0) {
		// alloc new page //
		if ((r = page_alloc(&p)) < 0) {
			return r;
		}
		
		// construct map from va to p //
		if ((r = page_insert(env->env_pgdir, p, va, PTE_R)) < 0) {
			return r;
		}
		size = (offset + bin_size >= BY2PG)? (BY2PG - offset): bin_size;
		bcopy((void *) bin, (void *)(page2kva(p) + offset), size);
	}

    for (i = size; i < bin_size; i += size) {
        /* Hint: You should alloc a new page. */
		if ((r = page_alloc(&p)) < 0) {
			return r;
		} 

		if ((r = page_insert(env->env_pgdir, p, va + i, PTE_R)) < 0) {
			return r;
		}
		size = (BY2PG < (bin_size - i))? BY2PG: (bin_size - i);
		bcopy((void *)(bin + i), (void *)(page2kva(p)), size);
    }
    /* Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`.
     * hint: variable `i` has the value of `bin_size` now! */
	offset = (va + i) - ROUNDDOWN((va + i), BY2PG);
	if (offset > 0) {
		size = ((offset + sgsize - i) < BY2PG)? (sgsize - bin_size): (BY2PG - offset);
		// bzero((void *)(page2kva(p) + offset), size);
		i += size;
	}

    while (i < sgsize) {
		if ((r = page_alloc(&p)) < 0) {
			return r;
		}

		if ((r = page_insert(env->env_pgdir, p, va + i, PTE_R)) < 0) {
			return r;
		}
		size = (BY2PG < sgsize - i)? BY2PG: (sgsize - i);
		// bzero((void *)(page2kva(p)), size);
		i += size;
    }
    return 0;
}
/* Overview:
 *  Sets up the the initial stack and program binary for a user process.
 *  This function loads the complete binary image by using elf loader,
 *  into the environment's user memory. The entry point of the binary image
 *  is given by the elf loader. And this function maps one page for the
 *  program's initial stack at virtual address USTACKTOP - BY2PG.
 *
 * Hints:
 *  All mapping permissions are read/write including text segment.
 *  You may use these :
 *      page_alloc, page_insert, page2kva , e->env_pgdir and load_elf.
 */
/*** exercise 3.7 ***/
static void
load_icode(struct Env *e, u_char *binary, u_int size)
{
    /* Hint:
     *  You must figure out which permissions you'll need
     *  for the different mappings you create.
     *  Remember that the binary image is an a.out format image,
     *  which contains both text and data.
     */
    struct Page *p = NULL;
    u_long entry_point;
    u_long r;

    /* Step 1: alloc a page. */
	if ((r = page_alloc(&p)) < 0) {
		return;
	}

    /* Step 2: Use appropriate perm to set initial stack for new Env. */
    /* Hint: Should the user-stack be writable? */
	/* user-stack should be writable! */ 
	if ((r = page_insert(e->env_pgdir, p, USTACKTOP - BY2PG, PTE_R)) < 0) {
		return;
	}

    /* Step 3: load the binary using elf loader. */
	r = load_elf(binary, size, &entry_point, (void *)e, &load_icode_mapper);
	if (r < 0) {
		return;
	}

    /* Step 4: Set CPU's PC register as appropriate value. */
	/*	set the first thread pc to entry_point	*/
	e->env_threads[0].tcb_tf.pc = entry_point;
}

/* Overview:
 *  Allocate a new env with env_alloc, load the named elf binary into
 *  it with load_icode and then set its priority value. This function is
 *  ONLY called during kernel initialization, before running the FIRST
 *  user_mode environment.
 *
 * Hints:
 *  this function wraps the env_alloc and load_icode function.
 */
/*** exercise 3.8 ***/
void
env_create_priority(u_char *binary, int size, int priority)
{
    struct Env *e;
    /* Step 1: Use env_alloc to alloc a new env. */
	env_alloc(&e, 0);
	/* Step 2: alloc a new thread control block	*/
    /* Step 3: assign priority to the new env. */
    /* need to set the pri to the first thread in this env */
	e->env_threads[0].tcb_pri = priority;
	e->env_threads[0].tcb_status = THREAD_RUNNABLE;
	/* Step 4: Use load_icode() to load the named elf binary,
       and insert it into env_sched_list using LIST_INSERT_HEAD. */
	load_icode(e, binary, size);
	int r;	
//	if ((r = sys_mem_alloc(0, 0, USEMS, PTE_V | PTE_R)) < 0) {
//		return r;
//	}
	/* Step 5: insert the tcb into sched list */
	LIST_INSERT_HEAD(&tcb_sched_list[0], &e->env_threads[0], tcb_sched_link);
}
/* Overview:
 * Allocate a new env with default priority value.
 *
 * Hints:
 *  this function calls the env_create_priority function.
 */
/*** exercise 3.8 ***/
void
env_create(u_char *binary, int size)
{
     /* Step 1: Use env_create_priority to alloc a new env with priority 1 */
	env_create_priority(binary, size, 1);
}

void thread_free(struct Tcb *t) {
	struct Env *e = (struct Env *)ROUNDDOWN(t, BY2PG);
	printf("[%08x] free tcb %08x\n", e->env_id, t->thread_id);
	
	int tcb_no = t->thread_id & 0x7;
	int i, r;
	int lower = USTACKTOP - STACKSIZE*(tcb_no+1)*BY2PG;
	int higher = USTACKTOP - STACKSIZE*(tcb_no)*BY2PG;
	for (i = lower; i < higher; i += BY2PG) {
		if ((r = sys_mem_unmap(0, 0, i)) < 0) {
			return r;
		}
	}
	bzero((void *)t, sizeof(struct Tcb));		

	if (e->env_thread_count <= 0) {
		env_free(e);
	}
	t->tcb_status = THREAD_FREE;
}

/* Overview:
 *  Free env e and all memory it uses.
 */
void
env_free(struct Env *e)
{
    Pte *pt;
    u_int pdeno, pteno, pa;

    /* Hint: Note the environment's demise.*/
    printf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

    /* Hint: Flush all mapped pages in the user portion of the address space */
    for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
        /* Hint: only look at mapped page tables. */
        if (!(e->env_pgdir[pdeno] & PTE_V)) {
            continue;
        }
        /* Hint: find the pa and va of the page table. */
        pa = PTE_ADDR(e->env_pgdir[pdeno]);
        pt = (Pte *)KADDR(pa);
        /* Hint: Unmap all PTEs in this page table. */
        for (pteno = 0; pteno <= PTX(~0); pteno++)
            if (pt[pteno] & PTE_V) {
                page_remove(e->env_pgdir, (pdeno << PDSHIFT) | (pteno << PGSHIFT));
            }
        /* Hint: free the page table itself. */
        e->env_pgdir[pdeno] = 0;
        page_decref(pa2page(pa));
    }
    /* Hint: free the page directory. */
    pa = e->env_cr3;
    e->env_pgdir = 0;
    e->env_cr3 = 0;
    /* Hint: free the ASID */
    asid_free(e->env_id >> (1 + LOG2NENV));
    page_decref(pa2page(pa));
    /* Hint: return the environment to the free list. */
    LIST_INSERT_HEAD(&env_free_list, e, env_link);
}

/* Overview:
 *  Free env e, and schedule to run a new env if e is the current env.
 */
void
env_destroy(struct Env *e)
{
    /* Hint: free e. */
    env_free(e);

    /* Hint: schedule to run a new environment. */
    if (curenv == e) {
        curenv = NULL;
        /* Hint: Why this? */
        bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
              (void *)TIMESTACK - sizeof(struct Trapframe),
              sizeof(struct Trapframe));
        printf("i am killed ... \n");
        sched_yield();
    }
}

/* destroy a thead */
void thread_destroy(struct Tcb *t) {
	struct Env *e = (struct Env *)ROUNDDOWN(t, BY2PG);

	if (t->tcb_status == THREAD_RUNNABLE) {
		LIST_REMOVE(t, tcb_sched_link);
	}

	e->env_thread_count--;	
	if (t->tcb_detach == UNJOINABLE) {
		thread_free(t);
	} else {
		t->tcb_status = THREAD_ZOMBIE;
	}

	if (curtcb == t) {
        bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),
              (void *)TIMESTACK - sizeof(struct Trapframe),
              sizeof(struct Trapframe));
        printf("i am thread, i am killed ... \n");
        sched_yield();
	}
}


extern void env_pop_tf(struct Trapframe *tf, int id);
extern void lcontext(u_int contxt);

/* Overview:
 *  Restore the register values in the Trapframe with env_pop_tf, 
 *  and switch the context from 'curenv' to 'e'.
 *
 * Post-Condition:
 *  Set 'e' as the curenv running environment.
 *
 * Hints:
 *  You may use these functions:
 *      env_pop_tf , lcontext.
 */
/*** exercise 3.10 ***/
void
env_run(struct Tcb *t)
{
    /* Step 1: save register state of curenv. */
    /* Hint: if there is an environment running, 
     *   you should switch the context and save the registers. 
     *   You can imitate env_destroy() 's behaviors.*/
	struct Trapframe *old;
	if (curtcb != NULL) {
		old = (struct Trapframe *)(TIMESTACK - sizeof(struct Trapframe));
		bcopy(old, &(curtcb->tcb_tf), sizeof(struct Trapframe));
		curtcb->tcb_tf.pc = curtcb->tcb_tf.cp0_epc;
	}	

    /* Step 2: Set 'curenv' to the new environment and Set 'curtcb' to the new thread */
	curtcb = t;
	curenv = (struct Env *)ROUNDDOWN((u_int)t, BY2PG);
	curenv->env_runs++;

    /* Step 3: Use lcontext() to switch to its address space. */
	lcontext((u_int)curenv->env_pgdir);
    /* Step 4: Use env_pop_tf() to restore the environment's
     *   environment   registers and return to user mode.
     *
     * Hint: You should use GET_ENV_ASID there. Think why?
     *   (read <see mips run linux>, page 135-144)
     */
	env_pop_tf(&t->tcb_tf, GET_ENV_ASID(curenv->env_id));
}

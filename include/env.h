/* See COPYRIGHT for copyright information. */

#ifndef _ENV_H_
#define _ENV_H_

#include "types.h"
#include "queue.h"
#include "trap.h"
#include "mmu.h" 

#define LOG2NENV	10
#define NENV		(1<<LOG2NENV)
#define ENVX(envid)	((envid) & (NENV - 1))
#define GET_ENV_ASID(envid) (((envid)>> 11)<<6)
#define THREAD_MAX	8
#define _PTHREAD_ATFORK_MAX 16
#define PTHREAD_KEYS_MAX	10
#define STACKSIZE	32

// Values of env_status in struct Env
#define ENV_FREE	0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

//thread cancel state & type
#define THREAD_CANNOT_BE_CANCELED	1;
#define THREAD_CANCEL_IMI		0;

// thread status
#define THREAD_FREE			 0
#define THREAD_RUNNABLE		 1
#define THREAD_NOT_RUNNABLE  2
#define THREAD_ZOMBIE		 4
#define THREAD_CANCELED		99


// thread cancel status 
#define THREAD_CANCEL_ENABLE	0
#define THREAD_CANCEL_DISABLE	1

// thread cancel time 
#define THREAD_CANCEL_DEFERED	0
#define THREAD_CANCEL_ASYCHRONOUS	1

// thread detach state
#define JOINABLE	0
#define UNJOINABLE	1

struct Tcb {
	// as a sched substance, need to save the information 
	struct Trapframe tcb_tf;
	// Thread information
	u_int thread_id;
	u_int tcb_status;
	u_int tcb_pri;
	LIST_ENTRY(Tcb) tcb_sched_link;
	// for pthread_join()
	LIST_ENTRY(Tcb) tcb_joined_link;
	LIST_HEAD(Tcb_joined_list, Tcb);
	struct Tcb_joined_list tcb_joined_list;
	void **tcb_join_value_ptr;
	// for thread detachstate
	u_int tcb_detach;
	// for pthread_exit()
	void *tcb_exit_ptr;
	int tcb_exit_value;
	// for pthread_cancel()
	u_int tcb_cancel_state;
	u_int tcb_cancel_type;
	u_int tcb_canceled;

	void *tcb_tls[PTHREAD_KEYS_MAX];
};

typedef struct {
	int detachstate;
} pthread_attr_t;

typedef unsigned int pthread_key_t;
typedef struct tls_entry {
	LIST_ENTRY(tls_entry) link;
	void (*dtor)(void *);
} tls_entry_t;

LIST_HEAD(tls_entry_head, tls_entry);
extern tls_entry_t tls_entries[PTHREAD_KEYS_MAX];

typedef struct _pthread_handler_rec {
	void (*handler)(void *);
	void *arg;
	struct _pthread_handler_rec *pre;
} _pthread_handler_rec; 


struct Env {
	LIST_ENTRY(Env) env_link;       // Free list
	u_int env_id;                   // Unique environment identifier
	u_int env_pri;
	u_int env_parent_id;            // env_id of this env's parent
	Pde  *env_pgdir;                // Kernel virtual address of page dir
	u_int env_cr3;

	// Lab 4 IPC
	u_int env_ipc_value;            // data value sent to us 
	u_int env_ipc_waiting_thread;
	u_int env_ipc_from;             // envid of the sender  
	u_int env_ipc_recving;          // env is blocked receiving
	u_int env_ipc_dstva;		// va at which to map received page
	u_int env_ipc_perm;		// perm of page mapping received

	// Lab 4 fault handling
	u_int env_pgfault_handler;      // page fault state
	u_int env_xstacktop;            // top of exception stack

	// Lab 6 scheduler counts
	u_int env_runs;			// number of times been env_run'ed

	// Lab 4 chanllenge thread 
	u_int env_thread_count;

	u_int env_nop[495];                  // align to avoid mul instruction

	struct Tcb env_threads[8];
};

LIST_HEAD(Env_list, Env);
LIST_HEAD(Tcb_list, Tcb);
extern struct Env *envs;		// All environments
extern struct Env *curenv;	        // the current env
extern struct Tcb *curtcb;			// the current tcb
extern struct Tcb_list tcb_sched_list[2];

void env_init(void);
void mips_sem_init(void);
int env_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
void env_create_priority(u_char *binary, int size, int priority);
void env_create(u_char *binary, int size);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Tcb *t);

// for thread system 
int thread_alloc(struct Env *e, struct Tcb **new);
void thread_destroy(struct Tcb *t);
u_int mktcbid(struct Tcb *b);
int threadid2tcb(u_int threadid, struct Tcb **t);

// for the grading script
#define ENV_CREATE2(x, y) \
{ \
	extern u_char x[], y[]; \
	env_create(x, (int)y); \
}
#define ENV_CREATE_PRIORITY(x, y) \
{\
        extern u_char binary_##x##_start[]; \
        extern u_int binary_##x##_size;\
        env_create_priority(binary_##x##_start, \
                (u_int)binary_##x##_size, y);\
}
#define ENV_CREATE(x) \
{ \
	extern u_char binary_##x##_start[];\
	extern u_int binary_##x##_size; \
	env_create(binary_##x##_start, \
		(u_int)binary_##x##_size); \
}

#endif // !_ENV_H_

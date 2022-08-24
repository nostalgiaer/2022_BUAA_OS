#include "lib.h"
#include <mmu.h>
#include <env.h>

void
exit(void)
{
	//close_all();
	pthread_exit(0);
//	syscall_thread_destroy(0);
}

struct Env *env;
struct Tcb *tcb;
sem_t global_mutex;
sem_entry_t *usems = (sem_entry_t *)USEMS;
_pthread_handler_rec *heads[THREAD_MAX];
_pthread_handler_rec **handler_heads[THREAD_MAX];

static void global_sem_init(sem_t *mutex) {
	usems[0].val = 1;
	usems[0].ref = 1;
	usems[0].pshared = 1;
	usems[0].envid = NULL;
	LIST_INIT(&usems[0].wait_list);
	*mutex = (1 << 10);
}

void
libmain(int argc, char **argv)
{
	// set env to point at our env structure in envs[].
	env = 0;	// Your code here.
//	int r = sem_init(&global_mutex, 0, 1);
//	user_assert(!r);
	int envid;
	u_int tcbid;
	envid = syscall_getenvid();
	envid = ENVX(envid);
	env = &envs[envid];
	tcbid = syscall_gettcbid();
	tcb = &env->env_threads[tcbid & 0x7];

	global_sem_init(&global_mutex);
	pthread_init();	
	// call user main routine
	umain(argc, argv);
	// exit gracefully
	exit();
	//syscall_env_destroy(0);
}

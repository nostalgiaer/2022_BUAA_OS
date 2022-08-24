#include "lib.h"
#include <error.h>

/*int sem_init(sem_t *sem, int pshared, u_int value) {
	return syscall_sem_init(sem, pshared, value);
}*/
extern sem_t global_mutex;
extern sem_entry_t *usems;
extern struct Env *env;

int sem_init(sem_t *sem, int pshared, u_int value) {
	/* lock	*/
	user_assert(!sem_wait(&global_mutex));
	int i;
	for (i = 0; i < ENV_UNNAMED_SEM_MAX; i++) {
		if (!usems[i].ref) {
			usems[i].val = (u_short)value;
			usems[i].pshared = pshared;
			usems[i].ref = 1;
			usems[i].envid = env->env_id;
			*sem = (1 << 10) | i;
			LIST_INIT(&usems[i].wait_list);
			/* unlock */
			user_assert(!sem_post(&global_mutex));	
			return 0;
		}
	}
	/* unlock */
	user_assert(!sem_post(&global_mutex));
	return -E_NO_SEM;
}

int sem_wait(sem_t *sem) {
	return syscall_sem_wait(sem);
}

int sem_post(sem_t *sem) {
	return syscall_sem_post(sem);
}

int sem_getvalue(sem_t *sem, int *valp) {
	return syscall_sem_getvalue(sem, valp);
}

int sem_destroy(sem_t *sem) {
	return syscall_sem_destroy(sem);
}

int sem_trywait(sem_t *sem) {
	int now_val;
	int r;

	if ((r = sem_getvalue(sem, &now_val)) < 0) {
		return r;
	}
	if (now_val <= 0) {
		return -E_SEM_ZERO;
	} else {
		return sem_wait(sem);
	}
}

sem_t sem_open(const char *name, u_int oflag, ...) {
	if (strlen(name) > SEM_NAME_MAX) {
		return -E_SEM_OPEN_FAIL;
	}	

	u_short val = 0;
	u_short mode = 0;

	if (oflag & O_CREAT) {
		va_list ap;
		va_start(ap, oflag);
		// ignore the mode
		va_arg(ap, unsigned);
		val = (u_short)va_arg(ap, unsigned);
		va_end(ap);
	}
	
	return syscall_sem_open(name, oflag, mode, val);
}

int sem_unlink(const char *name) {
	return syscall_sem_unlink(name);
}

int sem_close(sem_t *sem) {
	return syscall_sem_destroy(sem);
}

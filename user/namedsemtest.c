#include "lib.h"

static void _test_for_named_sem_1(void) {
	char *name = "wzmwzmwzm";
	char *a = "synchronize";
	sem_t father_mutex, father_sync;
	father_mutex = sem_open(name, O_CREAT, 0, 1);
	father_sync = sem_open(a, O_CREAT, 0, 0);
	sem_t mutex;
	user_assert(!sem_init(&mutex, 1, 0));
	user_assert(father_mutex > 0);
	user_assert(father_sync > 0);
	u_int new_envid;
	if ((new_envid = fork()) < 0) {
		user_panic("fork fail!\n");
	} else if (!new_envid) {
		sem_t son_mutex = sem_open(name, O_EXCL);
		sem_t son_sync = sem_open(a, O_EXCL);
		user_assert(son_mutex == father_mutex);
		user_assert(son_sync == father_sync);
		int val;
		user_assert(!sem_getvalue(&son_mutex, &val));
		user_assert(val == 1);
		user_assert(!sem_wait(&son_mutex)); 	
		user_assert(!sem_post(&son_sync));		// free father
		user_assert(!sem_unlink(name));			// unlink
		user_assert(!sem_unlink(a));	
		int cnt = 0;
		user_assert(!sem_wait(&mutex)); 		//wait for father process;
		user_assert(sem_post(&son_mutex) == -E_SEM_NOT_INIT);
		user_assert(sem_post(&son_sync) == -E_SEM_NOT_INIT);
		writef("son over\n");
	} else {
		user_assert(!sem_wait(&father_sync));	// wait for son process
		user_assert(!sem_unlink(name));
		user_assert(!sem_unlink(a));	
		int val;
		user_assert(sem_post(&father_mutex) == -E_SEM_NOT_INIT);
		user_assert(sem_post(&father_sync) == -E_SEM_NOT_INIT);
		writef("father over\n");	
		user_assert(!sem_post(&mutex));		//free son process
	}
	
}

void umain() {
	_test_for_named_sem_1();
	writef("over!\n");
	while (1);
}


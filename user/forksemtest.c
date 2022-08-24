#include "lib.h"
#define MAX 4000

#define SHARE 1

void umain() {
	u_int new_envid;
	sem_t mutex;
	sem_t a;
	sem_t b;
	user_assert(!sem_init(&mutex, SHARE, 1));
	user_assert(!sem_init(&a, SHARE, 0));
	user_assert(!sem_init(&b, SHARE, 0));	
	if ((new_envid = fork()) < 0) {
		user_panic("no!!!!\n");
	} else if (!new_envid) {
		int cnt = 0;
		while (++cnt < MAX) {
			int r;
			r= sem_wait(&mutex);
			if (SHARE) {
				user_assert(!r);
			} else {
				user_assert(r == -E_SEM_NO_PERM);
				writef("son cannot use!\n");
				user_assert(!r); //stop it
			}
			//user_assert(!sem_wait(&mutex));
			writef("consume %d\n", cnt);
			user_assert(!sem_post(&mutex));
		}	
		user_assert(!sem_wait(&a));
		user_assert(!sem_post(&b));
		writef("son done\n");
	} else {
		int cnt = 0;
		while (++cnt < MAX) {
			user_assert(!sem_wait(&mutex));
			writef("produce %d\n", cnt);
			user_assert(!sem_post(&mutex));
		}
		user_assert(!sem_post(&a));
		user_assert(!sem_wait(&b));
		writef("father done\n");
	}
}

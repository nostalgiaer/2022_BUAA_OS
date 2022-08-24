#include "lib.h"

void first(void) {
	int a = 5;
	writef("execute! %d\n", a);
	return;
}


void umain() {
	pthread_t thread;
	u_int new_envid;
	int r;

	r = pthread_atfork(first, NULL, NULL);
	user_assert(!r);

	if ((new_envid = fork()) < 0) {
		user_panic("fork fail!\n");
	} else if (!new_envid) {
		writef("i'm son!\n'");
	} else {
		writef("i'm father!\n'");
	}

}

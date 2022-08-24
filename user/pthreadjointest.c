#include "lib.h"

static void* routine_circle(void *arg) {
	int cnt = 0;
	while (++cnt < 5000) { writef("cnt: %d\n", cnt); }
	pthread_exit((void *)5);
}

static void test_pthread_join_wait(void) {
	pthread_t thread;
	int r, cnt = 0;
	r = pthread_create(&thread, NULL, routine_circle, NULL);
	user_assert(!r);
	r = pthread_join(thread, NULL);
	if (r < 0) {
		user_panic("no!\n");
	}
	while (++cnt < 100) {
		writef("main thread cnt : %d\n", cnt);
	}
	return;
}

void umain() {
	test_pthread_join_wait();
	writef("over!\n");
	while (1);
}

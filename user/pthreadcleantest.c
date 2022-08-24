#include "lib.h"

static void clean_routine(void *arg) {
	pthread_t cur = pthread_self();
	writef("current thread : %d executing clean_up routine %d\n", cur, *(int *)arg);
}

static void* routine_clean_up(void *arg) {
	int cnt = 1;
	pthread_cleanup_push(clean_routine, (void *)&cnt);
	writef("son thread is over!\n");	
	pthread_exit(0);
	pthread_cleanup_pop(1);
}

static void test_clean_up(void) {
	pthread_t thread;
	user_assert(!pthread_create(&thread, NULL, routine_clean_up, NULL));
	writef("create son thread over!\n");
	user_assert(!pthread_join(thread, NULL));
}

void umain() {
	test_clean_up();
	writef("over!\n");
	while (1);
}

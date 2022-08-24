#include"lib.h"
#define TEST_TYPE 	1

static void* routine_pthread_cancel(void* arg) {
	int i;
	if (TEST_TYPE) {
		int r = pthread_setcanceltype(THREAD_CANCEL_ASYCHRONOUS, NULL);
		user_assert(!r);
	}
	writef("routine: ");
	for (i = 0; i < 2000; i++) {
		writef("%d ", i);
		if (i == 1500) {
			pthread_testcancel();
		}
	}
	return NULL;
}

static void test_pthread_cancel(void) {
	pthread_t thread;
	int r = pthread_create(&thread, NULL, routine_pthread_cancel, NULL);	
	int i;
	writef("main thread: ");
	for (i = 3000; i < 3050; i++) {
		writef("%d ", i);
	}
	r = pthread_cancel(thread);
	user_assert(!r);
	writef("have sent cancel signal to son thread!\n");
	void *ret;
	r = pthread_join(thread, &ret);
	user_assert(ret = (void *)THREAD_CANCELED);
	writef("main thread cancel over!\n");
}

void umain() {
	test_pthread_cancel();
	writef("over!\n");
	while (1);
}

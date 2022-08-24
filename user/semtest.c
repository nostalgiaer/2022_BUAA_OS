#include "lib.h"

// test sem_init sem_wait sem_post
static sem_t mutex;

static void* thread_first(void *arg) {
	writef("output 1\n");
	user_assert(!sem_post(&mutex));
	int i;
	user_assert(!sem_wait(&mutex));
	for (i = 3; i <= 5; i++) {
		writef("output %d\n", i);
	}
	user_assert(!sem_post(&mutex));
	writef("son thread is over!\n");
}

static void test_sem_synchro(void) {
	user_assert(!sem_init(&mutex, 0, 0));
	pthread_t thread;
	int r = pthread_create(&thread, NULL, thread_first, NULL);
	user_assert(!r);
	user_assert(!sem_wait(&mutex));	
	writef("output 2\n");
	user_assert(!sem_post(&mutex));
	int i;
	user_assert(!sem_wait(&mutex));
	for (i = 6; i <= 8; i++) {
		writef("output %d\n", i);
	}
	writef("main thread is over!\n");
	user_assert(!pthread_join(thread, NULL));
}

// test sem_getvalue
static void test_sem_getvalue(void) {
	user_assert(!sem_init(&mutex, 0, 10));
	int num;
	user_assert(!sem_getvalue(&mutex, &num));
	user_assert(num == 10);
	user_assert(!sem_wait(&mutex));
	user_assert(!sem_getvalue(&mutex, &num));
	user_assert(num == 9);
	int i;
	for (i = 1; i <= 5; i++) {
		user_assert(!sem_wait(&mutex));
	}
	user_assert(!sem_getvalue(&mutex, &num));
	user_assert(num == 4);
	writef("check over\n");
}

// test sem_destroy
static void* thread_sem_destroy(void *arg) {
	user_assert(!sem_wait(&mutex));
	writef("yes! over!\n");
}


static void test_sem_destroy(void) {
	user_assert(!sem_init(&mutex, 0, 1));
	user_assert(!sem_wait(&mutex));	
//	pthread_t thread;
//	int r = pthread_create(&thread, NULL, thread_sem_destroy, NULL);	
//	user_assert(!r);	
	user_assert(!sem_destroy(&mutex));
	int r = sem_wait(&mutex);
	user_assert(r == -E_SEM_NOT_INIT);
	user_assert(sem_post(&mutex) == -E_SEM_NOT_INIT);
	user_assert(sem_getvalue(&mutex, &r) == -E_SEM_NOT_INIT);
	writef("check over!\n");
}

// test trywait
static void* routine_sem_trywait1(void* arg) {
	sem_t* s = (sem_t*)arg;
	int c;
	user_assert(!sem_getvalue(s, &c));
	user_assert(c == 0);
	user_assert(sem_trywait(s) == -E_SEM_ZERO);
	user_assert(!sem_getvalue(s, &c));
	user_assert(c == 0);
	writef("1 over!\n");
}

static void* routine_sem_trywait2(void* arg) {
	sem_t* s = (sem_t*)arg;
	int c;
	sem_getvalue(s, &c);
	user_assert(c == 0);
	sem_post(s);
	sem_getvalue(s, &c);
	user_assert(c == 1);
	user_assert(sem_trywait(s) == 0);
	sem_getvalue(s, &c);
	user_assert(c == 0);
	writef("2 over!\n");
}

void test_sem_trywait(void) {
	sem_t s[2];
	pthread_t th1, th2;
	sem_init(s, 0, 0);
	sem_init(s + 1, 0, 0);
	pthread_create(&th1, NULL, routine_sem_trywait1, (void *)(s + 1));
	pthread_create(&th2, NULL, routine_sem_trywait2, (void *)s);
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	return;
}



void umain() {
	writef("********* testing sem_synchro *********\n");
	test_sem_synchro();
	writef("********* testing sem_getvalue *********\n");
	test_sem_getvalue();
	writef("********* testing sem_destroy *********\n");
	test_sem_destroy();
	writef("********* testing sem_trywait *********\n");
	test_sem_trywait();
	writef("over!\n");
	while (1);
}


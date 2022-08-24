#include "lib.h"

#define TEST_TYPE 1

#define MAGIC ((void *) 123)
#define MAGIC2 ((void *) 246)

static void* routine_pthread_create1(void* arg) {
	writef("enter! num:1\n");
	writef("MAGIC: %d\n", (u_int)arg);
	user_assert(arg == MAGIC);
	return NULL;
}

static void* routine_pthread_create2(void* arg) {
	writef("enter! num:2\n");
	user_assert(arg == MAGIC2);
	return NULL;
}

static void test_pthread_create(void) {
	pthread_t th1, th2;
	pthread_create(&th1, NULL, routine_pthread_create1, MAGIC);
	pthread_create(&th2, NULL, routine_pthread_create2, MAGIC2);
	user_assert(!pthread_join(th1, NULL));
	user_assert(!pthread_join(th2, NULL));
	return;
}

#undef MAGIC
#undef MAGIC2

// test for pthread_join
#define MAGIC ((void *) 456)

static void* routine_pthread_join(void* arg) {
	pthread_exit(MAGIC);
}

static void test_pthread_join(void) {
	pthread_t th;
	void* retval;
	pthread_create(&th, NULL, routine_pthread_join, NULL);
	pthread_join(th, &retval);
	writef("retval: %d\n", (u_int)retval);
	user_assert(retval == MAGIC);
	return;
}

#undef MAGIC

static sem_t cmutex;

// test share stack
static void* routine(void *arg) {
	int *a = (int *)0x7f3fdfb0;
	int *b = (int *)0x7f3fdfb4;
	int private_a = 8;	//0x7f3ddfec
	int private_b = 9;	//0x7f3ddff0
	writef("private_a address: 0x%x, private_b address: 0x%x\n", (u_int)&private_a, (u_int)&private_b);
	user_assert(*a == 2);
	user_assert(*b == -100);
	*a = 100;
	*b = 10;
	user_assert(!sem_post(&cmutex));

	user_assert(!sem_wait(&cmutex));
	user_assert(private_a == 1);
	user_assert(private_b == 2);
	writef("check over! nice\n");
}

static void test_share_stack() {
	sem_init(&cmutex, 0, 0);
	int a = 0, b = 0;
	a += 2;
	b = -100;	
	// a 0x7f3fdfb0
	// b 0x7f3fdfb4
	pthread_t thread;
	writef("a address 0x%x, b address 0x%x\n", (u_int)&a, (u_int)&b);
	int r = pthread_create(&thread, NULL, routine, NULL);
	user_assert(!r);
	sem_wait(&cmutex);	
	user_assert(a == 100);
	user_assert(b == 10);

	int *private_a = (int *)0x7f3ddfec;
	int *private_b = (int *)0x7f3ddff0;
	user_assert(*private_a == 8);
	user_assert(*private_b == 9);
	*private_a = 1;
	*private_b = 2;
	user_assert(!sem_post(&cmutex));
}

// test fork 
static void* _routine_test_fork(void *arg) {
	int a = 9;	//0x7f3ddfe4
	int *father_cnt = (int *)arg;
	writef("son: 0x%x\n", (u_int)&a);
	if (fork() == 0) {
//		int copy_a = *(int *)0x7f3ddfe4;	
//		int copy_cnt = *(int *)0x7f3fdfbc;
		int copy_a = a;
		int copy_cnt = *father_cnt;
		writef("copy_a : %d, copy_cnt : %d\n", copy_a, copy_cnt);
		user_assert(copy_cnt == 0);
		user_assert(copy_a == 9);
		writef("check over!\n");
	} else {
		writef("son thread over!\n");
	}
}

static void test_fork(void) {
	pthread_t thread;
	int cnt = 8;	//0x7f3fdfbc
	int r = pthread_create(&thread, NULL, _routine_test_fork, (void *)&cnt);
	writef("father: 0x%x\n", (u_int)&cnt);
	while (1);
}



void umain() {
	writef("************* testing pthread_create *****************\n");
	test_pthread_create();
	writef("************* testing pthread_join *****************\n");
	test_pthread_join();
	writef("************* testing pthread_share_stack *****************\n");
	test_share_stack();
	writef("************* testing pthread_fork *****************\n");
	test_fork();
//	test_pthread_join_wait();
//	test_pthread_cancel();
	writef("over!\n");
	while (1);
}

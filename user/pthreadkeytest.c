#include "lib.h"

#define TEST_DELETE 0
pthread_key_t key;

static void exit_function(void *arg) {
	pthread_t exe_thread = pthread_self();
	int a = *(int *)arg;
	writef("thread %d key exit! a is %d\n", exe_thread, a);	
}

static void son_1_thread(void *arg) {
//	char *str = "wzmwzm!";
	int a = 15;
	user_assert(!pthread_setspecific(key, (void *)&a));
	void *my_a_ptr = pthread_getspecific(key);
	user_assert(*(int *)my_a_ptr == 15);
	writef("son thread: %d\n", *(int *)my_a_ptr);	
}

static void main_thread(void) {
	user_assert(!pthread_key_create(&key, exit_function));
	int a = 5;
	pthread_t son_thread;
	user_assert(!pthread_create(&son_thread, NULL, son_1_thread, NULL));
	user_assert(!pthread_setspecific(key, (void *)&a));
	writef("key: %d\n", key);
	void *my_a_ptr = pthread_getspecific(key);
	user_assert(*(int *)my_a_ptr == 5);
	writef("main thread: %d\n", *(int *)my_a_ptr);
	pthread_join(son_thread, NULL);
	if (TEST_DELETE) {
		user_assert(!pthread_key_delete(key));
	}
	pthread_exit(0);
}


void umain() {
	main_thread();
	writef("over!\n");
//	while (1);
}

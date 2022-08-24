#include "lib.h"

void *print_msg(void *ptr);

umain() {
	pthread_t thread;
	void *a;
	
	pthread_create(&thread, NULL, print_msg, NULL);
	writef("thread1 open!\n");
	
	int r = pthread_join(thread, &a);

	writef("over! %d\n", r);
	writef("%s\n", (char *)a);
}


void *print_msg(void *ptr) {
	writef("thread 1 start!\n");
	pthread_exit((void *)"hello, world");
}

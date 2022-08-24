#include "lib.h"

#define MAGIC ((void *) 789)

static void* routine_pthread_join(void* arg) {
	return MAGIC;
}

static void* routine_pthread_exit(void* arg) {
	pthread_exit((void*)MAGIC);
	return NULL; // never executed
}

void umain() {
	pthread_t th;
	void* retval;
	pthread_create(&th, NULL, routine_pthread_join, NULL);
	pthread_join(th, &retval);
	writef("%d\n", (int)(void *)MAGIC);
	user_assert(retval == (void*)MAGIC);
}


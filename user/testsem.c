#include "lib.h"

#define MAX 2000
int remain = 0;
sem_t sem;

void *consume(void *args) {
	int cnt = 0;
	int r= 0;
//	sem_t *sem = (sem_t *)args;
	
	while (cnt < MAX) {
		r = sem_wait(&sem);
		user_assert(!r);
		int test = 0;
//		while (test < 50000) { test++; }
		remain--;
		writef("consume a new object! remain: %d\n", remain);
		sem_post(&sem);
		cnt++;
	}
}


void *produce(void *args) {
	int cnt = 0;
	int r;
//	sem_t *sem = (sem_t *)args;
	
	while (cnt < MAX) {
		r = sem_wait(&sem);
		user_assert(!r);
		int test = 0;
//		while (test < 50000) { test++; }
		remain++;
		writef("produce a new object! remain: %d\n", remain);
		sem_post(&sem);
		cnt++;
	}
}

void umain() {
//	sem_t sem;
	sem_init(&sem, 0, 1);
	writef("init over\n");
	pthread_t thread1, thread2, thread3, thread4, thread5;
	int r;
	r = pthread_create(&thread1, NULL, consume, (void *)&sem);
	user_assert(!r);
	r = pthread_create(&thread2, NULL, produce, (void *)&sem);
	user_assert(!r);
	r = pthread_create(&thread3, NULL, consume, (void *)&sem);
	user_assert(!r);
	r = pthread_create(&thread4, NULL, consume, (void *)&sem);
	user_assert(!r);
	r = pthread_create(&thread5, NULL, produce, (void *)sem);
	user_assert(!r);
	void *ret;
	pthread_join(thread1, &ret);
	pthread_join(thread2, &ret);
	pthread_join(thread3, &ret);
	pthread_join(thread4, &ret);
	pthread_join(thread5, &ret);
	writef("remain : %d\n", remain);
}

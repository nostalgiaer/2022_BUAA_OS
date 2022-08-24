#include "lib.h"
void *buy(void *args) {
	u_int *array = (u_int *)args;
	sem_t *mutex = (sem_t *)&array[0];
	int *b = (int *)((u_int *)args)[1];
	int son = (int)((u_int *)args)[2];
	int c;
	int exitflag = 0;
	while (1) {
		user_assert(!sem_wait(mutex));
		if (*b > 0) {
			c = *b;
			*b = *b - 1;
			writef("son%d buy ticket %d, now have %d tickets\n",son,c,*b);
		}
		if (*b == 0) {
			exitflag = 1;
		} else if (*b < 0) {
			user_panic("panic at son%d, tickets are %d\n",son,*b);
		}
		user_assert(!sem_post(mutex));
		if (exitflag) {
			break;
		}
	}
	pthread_exit(0);
}


void umain() {
	u_int arg1[3];
	u_int arg2[3];
	u_int arg3[3];
	sem_t test;
	user_assert(!sem_init(&test, 0, 1));
	sem_t mutex;
	user_assert(!sem_init(&mutex,0,1));
	arg1[0] = (u_int)mutex;
	arg2[0] = (u_int)mutex;
	arg3[0] = (u_int)mutex;
	int sum = 5000;
	arg1[1] = &sum;
	arg2[1] = &sum;
	arg3[1] = &sum;
	pthread_t thread1;
	pthread_t thread2;
	pthread_t thread3;
	int val;
	arg1[2] = 1;
	pthread_create(&thread1,NULL,buy,(void *)arg1);

	void *a;
	arg2[2] = 2;
	pthread_create(&thread2,NULL,buy,(void *)arg2);

	arg3[2] = 3;
	pthread_create(&thread3,NULL,buy,(void *)arg3);

	pthread_join(thread1, &a);
	pthread_join(thread2, &a);
	pthread_join(thread3, &a);
	writef("check over, main thread exit!\n");
}

#include "lib.h"

void *test1(void *arg) {
        sem_t *sem1 = (sem_t *)((int *)arg)[0];
        writef("\ntest1 wait sem1 %d \n", *sem1);
	    user_assert(!sem_wait(sem1));
        writef("\ntest1 get sem1\n");
        int i;
//      for (i = 0;i < 100000;i++);
}

void *test2(void *arg) {
        sem_t *sem1 = (sem_t *)((int *)arg)[0];
        writef("\ntest2 post sem1 %d\n", *sem1);
        user_assert(!sem_post(sem1));
        int i;
        //for(i = 0;i < 100000; i++);
}

void umain() {
        writef("-----sembasictest begin-----\n");
        sem_t sem1;
        sem_init(&sem1, 0, 0);
        pthread_t pt1;
        pthread_t pt2;
        int r;
        int a[2];
        a[0] = &sem1;
        //writef("a[0] is %d\n", a[0]);
        r = pthread_create(&pt1, NULL, test1, (void *)a);
	    //syscall_yield();
        r = pthread_create(&pt2, NULL, test2, (void *)a);
        //syscall_yield();
        int i;
		pthread_join(pt1, NULL);
		pthread_join(pt2, NULL);
        writef("sembasictest end!\n");
}

#include "lib.h"

#define MAX 5000

void umain() {
	char *str = "wzmwzm";
	char *a = "hello!a";
	char *b = "hello!b";
	sem_t new_sem = sem_open(str, O_CREAT, 0, 1);
	sem_t a_sem = sem_open(a, O_CREAT, 0, 0);
	sem_t b_sem = sem_open(b, O_CREAT, 0, 0);
	writef("new_sem: %d\n", new_sem);
	u_int new_envid;
	int cnt = 0;
	int flag = 0;

	if ((new_envid = fork()) < 0) {
		writef("no! error on %d\n", new_envid);
	} else if (new_envid == 0) {
		sem_t son_a_sem = sem_open(a, O_CREAT, 0, 0);
		sem_t son_b_sem = sem_open(b, O_CREAT, 0, 0);
		sem_t sem = sem_open(str, O_CREAT, 0, 1);
		writef("son sem: %d\n", sem);
		while (flag < MAX) {
			sem_wait(&sem);
			cnt++;
			writef("produce a new product! remain: %d\n", cnt);
			sem_post(&sem);
			flag++;
		}
		writef("over!\n");
		user_assert(!sem_wait(&son_a_sem));
		user_assert(!sem_post(&son_b_sem));
		sem_unlink(str);
	} else {
		while (flag < MAX) {
			sem_wait(&new_sem);
			cnt--;
			writef("consume a new product! remain: %d\n", cnt);
			sem_post(&new_sem);
			flag++;
		}
		user_assert(!sem_post(&a_sem));
		user_assert(!sem_wait(&b_sem));
		writef("over!\n");
		sem_unlink(str);
	}
}

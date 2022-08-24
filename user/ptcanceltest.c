#include "lib.h"

void *print(void *arg) {
	writef("enter into waiting!\n");
	int cnt = 0;
	while (cnt < 10000) {
//		writef("%d waiting\n", cnt);
		cnt++;
	}
	pthread_testcancel();
}

umain() {
	pthread_t thread;
	int cnt = 0;
	pthread_create(&thread, NULL, print, NULL);

/*	while (cnt <= 1000000) {
		cnt++;
	}
*/
	void *a;
	pthread_cancel(thread);
	writef("wait for thread1 ending \n");
	pthread_join(thread, &a);
	writef("0x%x\n", *(int *)a);
	writef("thread1 is over!\n");

}

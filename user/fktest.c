#include "lib.h" 

void umain()
{
	int a = 0;
	int id = 0;

	if ((id = fork()) == 0) {
		if ((id = fork()) == 0) {
			a += 3;
			writef("\t\tthis is child2 :a:%d\n", a);
			
			for (;;) {
				writef("\t\tthis is child2 :a:%d\n", a);
			}
		}

		a += 2;
		writef("env 2 address: 0x%x\n", (u_int)&a);
		writef("\tthis is child :a:%d\n", a);
		for (;;) {
			writef("\tthis is child :a:%d\n", a);
		}
	}

	a++;
	for (;;) {
		writef("this is father: a:%d\n", a);
	}
}

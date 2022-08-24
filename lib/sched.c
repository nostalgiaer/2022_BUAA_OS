#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */

void sched_yield() {
	static int count = 0;
	static int point = 0;

	extern struct Tcb *curtcb;
	
	if (curtcb != NULL && curtcb->tcb_status == THREAD_RUNNABLE) {
			if (count == 0) {
				LIST_REMOVE(curtcb, tcb_sched_link);
				LIST_INSERT_TAIL(&tcb_sched_list[1-point], curtcb, tcb_sched_link);
			} else {
				count--;
				env_run(curtcb);
			}
	} 

	struct Tcb *t;
	while (1) {
		int have = 0;
		LIST_FOREACH(t, &tcb_sched_list[point], tcb_sched_link) {
			if (t->tcb_status == THREAD_RUNNABLE) {
				have = 1;
				break;
			}
		}
		if (!have) {
			point = 1 - point;
		}

		LIST_FOREACH(t, &tcb_sched_list[point], tcb_sched_link) {
			if (t->tcb_status == THREAD_RUNNABLE) {
				count = t->tcb_pri - 1;
//				printf("%d\n", t->thread_id & 0x7);
				env_run(t);
			}
		}
	}
}

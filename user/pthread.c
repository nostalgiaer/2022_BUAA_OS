#include "lib.h"
#include <error.h>
#include <mmu.h>
#include <env.h>

extern struct Env *env;

tls_entry_t tls_entries[PTHREAD_KEYS_MAX];
struct tls_entry_head tls_entry_free_list;

void (*_pthread_prepare_hooks[_PTHREAD_ATFORK_MAX])();
void (*_pthread_parent_hooks[_PTHREAD_ATFORK_MAX])();
void (*_pthread_child_hooks[_PTHREAD_ATFORK_MAX])();
u_int _pthread_atfork_count = 0;

extern sem_t global_mutex;
extern _pthread_handler_rec *heads[THREAD_MAX];
extern _pthread_handler_rec **handler_heads[THREAD_MAX];
pthread_key_t _pthread_clean_handler_key;

void pthread_init(void) {
	int i;
	LIST_INIT(&tls_entry_free_list);
	for (i = PTHREAD_KEYS_MAX - 1; i >= 0; i--) {
		LIST_INSERT_HEAD(&tls_entry_free_list, &tls_entries[i], link);
	}
	
	for (i = 0; i < THREAD_MAX; i++) {
		heads[i] = NULL;
		handler_heads[i] = &heads[i];
	}
	/* 	alloc key to clean-up stack for threads */
	user_assert(!pthread_key_create(&_pthread_clean_handler_key, NULL));
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	user_assert(!sem_wait(&global_mutex));
	int thread_id;
	if ((thread_id = syscall_thread_alloc()) < 0) {
		user_assert(!sem_post(&global_mutex));
		return thread_id;
	}
	struct Tcb *t = &env->env_threads[thread_id & 0x7];
	
	t->tcb_tf.pc = (u_int)start_routine;
	t->tcb_detach = JOINABLE;
	t->tcb_tf.regs[31] = (u_int)exit;
	t->tcb_tf.regs[29] -= 4;
	t->tcb_tf.regs[4] = (u_int)arg;	

	syscall_set_thread_status(0, thread_id, THREAD_RUNNABLE);
	*thread = t->thread_id;

	_pthread_handler_rec **head = handler_heads[t->thread_id & 0x7];
	t->tcb_tls[_pthread_clean_handler_key] = (void *)head;
	user_assert(!sem_post(&global_mutex));
	
	return 0;
}

void pthread_exit(void *value_ptr) {
	int i;
	tls_entry_t *entry;
	void (*dtor)(void *);
	void *value;

	user_assert(!sem_wait(&global_mutex));
	u_int threadid = pthread_self();
	struct Tcb *t = &env->env_threads[threadid & 0x7];
	t->tcb_exit_ptr = value_ptr;
	user_assert(!sem_post(&global_mutex));

	void **ptr = pthread_getspecific(_pthread_clean_handler_key);
	if (ptr) {
		_pthread_handler_rec *head = *(_pthread_handler_rec **)ptr;
		while (head) {
			head->handler(head->arg);
			head = head->pre;
		}
	}

	for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
		user_assert(!sem_wait(&global_mutex));
		dtor = tls_entries[i].dtor;
		value = t->tcb_tls[i];
		t->tcb_tls[i] = NULL;
		user_assert(!sem_post(&global_mutex));

		if (dtor && value) {
			dtor(value);
		}
	}
	syscall_thread_destroy(threadid);
}

int pthread_join(pthread_t thread, void **retval) {
	int r = syscall_thread_join(thread, retval);
	return r;
}

int pthread_cancel(pthread_t thread) {
	user_assert(!sem_wait(&global_mutex));
	struct Tcb *t = &env->env_threads[thread & 0x7];

	if ((t->thread_id != thread) || (t->tcb_status == THREAD_FREE)) {
		user_assert(!sem_post(&global_mutex));
		return -E_THREAD_NOT_FOUND;
	}
	if (t->tcb_cancel_state == THREAD_CANCEL_DISABLE) {
		user_assert(!sem_post(&global_mutex));
		return -E_THREAD_CAN_NOT_CANCEL;
	}	

	t->tcb_canceled = 1;
	if (t->tcb_cancel_type == THREAD_CANCEL_ASYCHRONOUS) {
		t->tcb_exit_ptr = (void *)THREAD_CANCELED;
		syscall_thread_destroy(thread);
	} 
	user_assert(!sem_post(&global_mutex));
	return 0;
}

extern struct Tcb *tcb;

int pthread_setcancelstate(int state, int *oldstate) {
	u_int cur_thread_id = syscall_gettcbid();
	struct Tcb *t = &env->env_threads[cur_thread_id & 0x7];

	if (state != THREAD_CANCEL_ENABLE && state != THREAD_CANCEL_DISABLE) {
		return -E_INVAL;
	}
	user_assert(!sem_wait(&global_mutex));
	if (oldstate) {
		*oldstate = t->tcb_cancel_state;
	}
	t->tcb_cancel_state = state;
	user_assert(!sem_post(&global_mutex));
	return 0;
}

int pthread_setcanceltype(int type, int *oldtype) {
	u_int cur_thread_id = syscall_gettcbid();
	struct Tcb *t = &env->env_threads[cur_thread_id & 0x7];

	if (type != THREAD_CANCEL_DEFERED && type != THREAD_CANCEL_ASYCHRONOUS) {
		return -E_INVAL;
	}
	user_assert(!sem_wait(&global_mutex));
	if (oldtype) {
		*oldtype = t->tcb_cancel_type;
	}
	t->tcb_cancel_type = type;
	user_assert(!sem_post(&global_mutex));
	return 0;
}

void pthread_testcancel() {
	u_int cur_thread_id = syscall_gettcbid();
	struct Tcb *t = &env->env_threads[cur_thread_id & 0x7];
	
	user_assert(!sem_wait(&global_mutex));
	if (t->tcb_status != THREAD_RUNNABLE && t->tcb_status != THREAD_NOT_RUNNABLE) {
		user_assert(!sem_post(&global_mutex));
		return;
	}
	
	if (t->tcb_canceled && t->tcb_cancel_state == THREAD_CANCEL_ENABLE 
						&& t->tcb_cancel_type == THREAD_CANCEL_DEFERED) {
		t->tcb_exit_ptr = (void *)THREAD_CANCELED;
		syscall_thread_destroy(t->thread_id);
	}
	user_assert(!sem_post(&global_mutex));
	return;		
}

int pthread_atfork(void (* prepare)(void), void (* parent)(void), void (* child)(void)) {
	user_assert(!sem_wait(&global_mutex));
	if (_pthread_atfork_count == _PTHREAD_ATFORK_MAX) {
		user_assert(!sem_post(&global_mutex));
		return -E_INVAL;
	}

	_pthread_prepare_hooks[_pthread_atfork_count] = prepare;
	_pthread_parent_hooks[_pthread_atfork_count] = parent;
	_pthread_child_hooks[_pthread_atfork_count] = child;

	_pthread_atfork_count++;
	user_assert(!sem_post(&global_mutex));
	return 0;
 }

int pthread_detach(pthread_t thread) {
	return syscall_thread_detach(thread);
}

int pthread_attr_init(pthread_attr_t *attr) {
	attr->detachstate = JOINABLE;
	return 0;
}

int pthread_attr_getdetachstate(pthread_attr_t *attr, int *detachstate) {
	*detachstate = attr->detachstate;
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
	attr->detachstate = detachstate;
	return 0;
}

int pthread_key_create(pthread_key_t *key, void (*des)(void *)) {
	tls_entry_t *entry;
	int i;
	user_assert(!sem_wait(&global_mutex));
	if (LIST_EMPTY(&tls_entry_free_list)) {
		user_assert(!sem_post(&global_mutex));
		return -E_THREAD_NO_KEY;
	}

	entry = LIST_FIRST(&tls_entry_free_list);
	LIST_REMOVE(entry, link);
	*key = (pthread_key_t )(entry - tls_entries);
	entry->dtor = des;

	for (i = 0; i < THREAD_MAX; i++) {
		env->env_threads[i].tcb_tls[*key] = NULL;
	}
	
	user_assert(!sem_post(&global_mutex));
	return 0;
}

int pthread_key_delete(pthread_key_t key) {
	tls_entry_t *entry = &tls_entries[key];

	user_assert(!sem_wait(&global_mutex));
	entry->dtor = NULL;
	LIST_INSERT_HEAD(&tls_entry_free_list, entry, link);
	user_assert(!sem_post(&global_mutex)); 	

	return 0;
}

void *pthread_getspecific(pthread_key_t key) {
	pthread_t now_thread = pthread_self();
	struct Tcb *t = &env->env_threads[now_thread & 0x7];
	return t->tcb_tls[key];
}

int pthread_setspecific(pthread_key_t key, const void *val) {
	pthread_t now_thread = pthread_self();
	struct Tcb *t = &env->env_threads[now_thread & 0x7];
	t->tcb_tls[key] = (void *)val;

	return 0;
}

pthread_t pthread_self(void) {
	return syscall_gettcbid();
}

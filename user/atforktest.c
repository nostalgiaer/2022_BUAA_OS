#include "lib.h"

sem_t mutex;
static void prepare_routine_first() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing first! before produce son process env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void prepare_routine_second() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing second! before produce son process env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void prepare_routine_third() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing third! before produce son process env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void father_routine_first() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing first from father! env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void father_routine_second() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing second from father! env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void son_routine_first() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing first from son! env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void son_routine_third() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing third from son! env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

static void son_routine_second() {
	u_int thread_id = pthread_self();
	u_int env_id = syscall_getenvid();
	user_assert(!sem_wait(&mutex));
	writef("executing second from son! env: 0x%x, thread: 0x%x\n", env_id, thread_id);
	user_assert(!sem_post(&mutex));
}

void umain() {
	user_assert(!sem_init(&mutex, 1, 1));
	user_assert(!pthread_atfork(prepare_routine_first, father_routine_first, son_routine_first));
	user_assert(!pthread_atfork(prepare_routine_second, father_routine_second, son_routine_second));
	user_assert(!pthread_atfork(prepare_routine_third, NULL, son_routine_third));
	fork();
	user_assert(!sem_wait(&mutex));
	writef("execute over!, 0x%x\n", pthread_self());
	user_assert(!sem_post(&mutex));
}

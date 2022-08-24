#include "lib.h"
#include <unistd.h>
#include <mmu.h>
#include <env.h>
#include <trap.h>

void syscall_putchar(char ch)
{
	msyscall(SYS_putchar, (int)ch, 0, 0, 0, 0);
}


u_int
syscall_getenvid(void)
{
	return msyscall(SYS_getenvid, 0, 0, 0, 0, 0);
}

void
syscall_yield(void)
{
	msyscall(SYS_yield, 0, 0, 0, 0, 0);
}


int
syscall_env_destroy(u_int envid)
{
	return msyscall(SYS_env_destroy, envid, 0, 0, 0, 0);
}
int
syscall_set_pgfault_handler(u_int envid, void (*func)(void), u_int xstacktop)
{
	return msyscall(SYS_set_pgfault_handler, envid, (int)func, xstacktop, 0, 0);
}

int
syscall_mem_alloc(u_int envid, u_int va, u_int perm)
{
	return msyscall(SYS_mem_alloc, envid, va, perm, 0, 0);
}

int
syscall_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm)
{
	return msyscall(SYS_mem_map, srcid, srcva, dstid, dstva, perm);
}

int
syscall_mem_unmap(u_int envid, u_int va)
{
	return msyscall(SYS_mem_unmap, envid, va, 0, 0, 0);
}

int
syscall_set_env_status(u_int envid, u_int status)
{
	return msyscall(SYS_set_env_status, envid, status, 0, 0, 0);
}

int
syscall_set_trapframe(u_int envid, struct Trapframe *tf)
{
	return msyscall(SYS_set_trapframe, envid, (int)tf, 0, 0, 0);
}

void
syscall_panic(char *msg)
{
	msyscall(SYS_panic, (int)msg, 0, 0, 0, 0);
}

int
syscall_ipc_can_send(u_int envid, u_int value, u_int srcva, u_int perm)
{
	return msyscall(SYS_ipc_can_send, envid, value, srcva, perm, 0);
}

void
syscall_ipc_recv(u_int dstva)
{
	msyscall(SYS_ipc_recv, dstva, 0, 0, 0, 0);
}

int
syscall_cgetc()
{
	return msyscall(SYS_cgetc, 0, 0, 0, 0, 0);
}

int 
syscall_thread_alloc(void)
{
	return msyscall(SYS_thread_alloc, 0, 0, 0, 0, 0);
}

u_int
syscall_gettcbid(void) 
{
	return msyscall(SYS_gettcbid, 0, 0, 0, 0, 0);
}

int 
syscall_thread_destroy(u_int tcbid)
{
	return msyscall(SYS_thread_destroy, tcbid, 0, 0, 0, 0);
}

int 
syscall_set_thread_status(u_int envid, u_int threadid, u_int status) 
{
	return msyscall(SYS_set_thread_status, envid, threadid, status, 0, 0);
}

int
syscall_thread_join(u_int threadid, void **value_ptr)
{
	return msyscall(SYS_thread_join, threadid, (int)value_ptr, 0, 0, 0);
}

int
syscall_thread_detach(u_int threadid)
{
	return msyscall(SYS_thread_detach, threadid, 0, 0, 0, 0);
}

int 
syscall_sem_init(sem_t *sem, int pshared, u_int value) 
{
	return msyscall(SYS_sem_alloc, (u_int)sem, pshared, value, 0, 0);
}

int 
syscall_sem_destroy(sem_t *sem)
{
	return msyscall(SYS_sem_free, (u_int)sem, 0, 0, 0, 0);
}

int 
syscall_sem_wait(sem_t *sem)
{
	return msyscall(SYS_sem_wait, (u_int)sem, 0, 0, 0, 0);
}

int 
syscall_sem_post(sem_t *sem)
{
	return msyscall(SYS_sem_post, (u_int)sem, 0, 0, 0, 0);
}

int 
syscall_sem_getvalue(sem_t *sem, int *valuep) 
{
	return msyscall(SYS_sem_getvalue, (u_int)sem, (u_int)valuep, 0, 0, 0);
}

int
syscall_sem_open(const char *name, u_int oflag, u_short mode, u_short val)
{
	return msyscall(SYS_sem_open, (u_int)name, oflag, mode, val, 0);
}

int
syscall_sem_unlink(const char *name) 
{
	return msyscall(SYS_sem_unlink, (u_int)name, 0, 0, 0, 0);
}

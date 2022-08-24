//#ifdef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "env.h"
#include "mmu.h"
#include "types.h"
#include "error.h"


// for semaphore

#define SEMREQ_ALLOC	0

#define SEM_VALUE_MAX	65536
#define SEM_FAILED	NULL

#define SEM_WAIT_MAX	1024
#define SEM_NAME_MAX	224

#define SEMX(x) 	((x) & 0x3ff)
#define SEMID2SEM(x) ((x) | (1 << 10))

#define SHARED		1
#define PRIVATE	    0

#define ENV_UNNAMED_SEM_MAX		BY2PG/sizeof(sem_entry_t)
#define NAMED_SEM_MAX		1024
#define ENV_SEM_WAIT_MAX	BY2PG/sizeof(sem_wait_t)
#define SEM_UNNAMED(x)		((u_int)x & (1 << 10))
#define SEM_NAMED(x) 		((u_int)x & (1 << 11))

typedef u_int sem_t;

typedef struct sem_wait {
	LIST_ENTRY(sem_wait) sem_wait_link;
	u_int env_id;
	u_int thread_id;
} sem_wait_t;

LIST_HEAD(sem_wait_head, sem_wait);

extern sem_wait_t sem_waits[SEM_WAIT_MAX];
extern struct sem_wait_head sem_wait_free_list;

typedef struct sem_entry {
	LIST_ENTRY(sem_entry) sem_entry_link;
	u_short val;
	u_int pshared;
	u_int envid; 
	u_int ref;
	char name[SEM_NAME_MAX + 1];
	struct sem_wait_head wait_list;
} sem_entry_t;

LIST_HEAD(sem_head, sem_entry);

extern sem_entry_t sems[NAMED_SEM_MAX];
extern struct sem_head sem_free_list;

/*	Named Semaphore open modes */
#define O_CREAT		0x0100
#define O_EXCL		0x0400

//#endif

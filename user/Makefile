INCLUDES = -I../include/

#CROSS_COMPILE := /usr/eldk/usr/bin/mips_4KC-
#CC			  := $(CROSS_COMPILE)gcc
#CFLAGS		  := -O -G 0 -mno-abicalls -fno-builtin -Wa,-xgot -Wall -fPIC
#LD			  := $(CROSS_COMPILE)ld
# For embedding one program in another

USERLIB := printf.o \
		print.o \
		libos.o \
		fork.o \
		pgfault.o \
		syscall_lib.o \
		ipc.o \
		string.o \
		pthread.o \
		semaphore.o

CFLAGS += -nostdlib -static

all: forksemtest.x forksemtest.b \
	atforktest.x atforktest.b \
	pthreadcanceltest.x pthreadcanceltest.b \
	pthreadjointest.x pthreadjointest.b \
	pthreadcleantest.x pthreadcleantest.b \
	pthreadkeytest.x pthreadkeytest.b \
	pthreadtest.x pthreadtest.b \
	namedsemtest.x namedsemtest.b \
	semtest.x semtest.b \
	testsem.x testsem.b \
	seller.x seller.b \
	fktest.x fktest.b pingpong.x pingpong.b idle.x \
	$(USERLIB) entry.o syscall_wrap.o 

%.x: %.b.c
	echo cc1 $<
	$(CC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	echo create $@
	echo bintoc $* $< > $@~
	chmod +x ./bintoc
	./bintoc $* $< > $@~ && mv -f $@~ $@
#	grep \. $@

%.b: entry.o syscall_wrap.o %.o $(USERLIB)
	echo ld $@
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./user.lds $^

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	echo as $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: lib.h semaphore.h

.PHONY: clean

clean:
	rm -rf *~ *.o *.b.c *.x *.b

include ../include.mk

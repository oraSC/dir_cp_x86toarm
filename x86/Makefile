x86_client:x86_client.o pool.o
	gcc -o x86_client x86_client.o pool.o -lpthread
	rm x86_client.o pool.o
x86_client.o:x86_client.c ../lib/pthread_pool/pool.h
	gcc -c x86_client.c
pool.o:../lib/pthread_pool/pool.c ../lib/pthread_pool/pool.c
	gcc -c  ../lib/pthread_pool/pool.c

arm_server:arm_server.o mysocket.o
	gcc -o arm_server arm_server.o mysocket.o -lpthread 
	rm arm_server.o mysocket.o
arm_server.o:arm_server.c
	gcc -c arm_server.c
mysocket.o:../lib/socket/mysocket.c
	gcc -c ../lib/socket/mysocket.c


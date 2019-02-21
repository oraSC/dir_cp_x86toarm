arm_server:arm_server.o mysocket.o
	arm-linux-gcc -o arm_server arm_server.o mysocket.o -lpthread 
	rm arm_server.o mysocket.o
arm_server.o:arm_server.c
	arm-linux-gcc -c arm_server.c
mysocket.o:../lib/socket/mysocket.c
	arm-linux-gcc -c ../lib/socket/mysocket.c


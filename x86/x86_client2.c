#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "../lib/socket/mysocket.h"
#include "../lib/pthread_pool/pool.h"

int main()
{
	int ret;

	//创建线程池
	pPool_t ppool = pool_create(0);
	if(ppool == NULL)
	{
		perror("fail to create pthread pool");
		return -1
	}

	//创建客户端
	//int soc_fd = client_create(1000, "192.168.10.31");
	int soc_fd = client_create(1000, "202.192.32.48");
	if(soc_fd < 0)
	{
		perror("fail to create client");
		return -1;
	}
	
	//
	while(1)
	{
			
	
	}


	//关闭套接字
	ret = shutdown(soc_fd, SHUT_RDWR);
	if(ret < 0)
	{
		perror("fail to shutdown socketfd");
		return -1;
	}

	return 0;

}

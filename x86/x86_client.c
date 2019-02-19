#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <>
#include "../lib/socket/mysocket.h"

int main()
{
	int ret;

	//创建客户端
	int soc_fd = client_create(1000, "202.192.32.56");
	if(soc_fd < 0)
	{
		perror("fail to create client");
		return -1;
	}

	while(1)
	{
		send(soc_fd, "1111\n", strlen("1111\n"), 0);
		sleep(1);	
	
	
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

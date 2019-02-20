#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include "../lib/socket/mysocket.h"


int main()
{
	int ret;
	
	//创建服务器
	int client_port;
	unsigned char *client_ip;
	int acc_fd = server_create(1000, NULL, &client_port, &client_ip);
	if(acc_fd < 0)
	{
		perror("fail to create server");
		return -1;
	}
	printf("client ip:%s\tport:%d\n", client_ip, client_port);
	
	char buff[100] = {0};
	while(1)
	{
		bzero(buff, sizeof(buff));

		recv(acc_fd, buff, 1, 0);
		printf("%s", buff);
		
		
	}

	//关闭套接字
	ret = shutdown(acc_fd, SHUT_RDWR);
	if(ret < 0)
	{
		perror("fail to shudown acc_fd");
		return -1;
	}

	return 0;

}

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "../lib/socket/mysocket.h"
typedef struct FileInfo{

	int  name_size;
	long text_size;
	struct stat stat_buff;

}FileInfo_t;





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
	
	
	FileInfo_t fileinfo;
	
	while(1)
	{
		
		bzero(&fileinfo, sizeof(fileinfo));
		
		//接收文件长度信息
		ret = recv(acc_fd, &fileinfo, sizeof(fileinfo), 0);
		if(ret < 0)
		{
			perror("error exits in recv");
			goto error;
		}
		else if(ret == 0)
		{
			printf("client offlines\n");
			break;
		}
		printf("\nname size:%d, text size:%ld, mode:%o\n", fileinfo.name_size, fileinfo.text_size, fileinfo.stat_buff.st_mode);
		printf("%d\n", fileinfo.name_size * sizeof(char));	
		//接收文件名
		unsigned char *pdst_path = (unsigned char *)malloc(fileinfo.name_size * sizeof(char));
		ret = recv(acc_fd, pdst_path, sizeof(fileinfo.name_size * sizeof(char)), 0);
		if(ret < 0)
		{
			perror("error exits in recv");
			goto error;
		}
		else if(ret == 0)
		{
			printf("client offlines\n");
			break;
		}
		printf("dst path:%s\n", pdst_path);
		
		free(pdst_path);
		
	}

	//关闭套接字
	ret = shutdown(acc_fd, SHUT_RDWR);
	if(ret < 0)
	{
		perror("fail to shudown acc_fd");
		return -1;
	}

	return 0;
error:
	shutdown(acc_fd, SHUT_RDWR);
	return -1;


}

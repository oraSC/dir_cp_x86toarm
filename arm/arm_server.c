#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "../lib/socket/mysocket.h"
typedef struct FileInfo{

	int  name_size;
	int text_size;
	//struct stat stat_buff; //弃用，此结构体在64位系统大小为144,32位系统大小为88
	int mode;

}FileInfo_t;

/*功能：单次接收文件信息并创建写入文件
 *返回值：
 *	成功：		1
 *	失败：		-1;
 *	客户端下线：	0
 */
int recv_write_file(int acc_fd);
int tell_client_finish(int acc_fd);

//拷贝文件线程函数
void *copyfile_routine(void *arg);

//声明全局错误码
extern int errno;


int main()
{
	int ret;

	//采用多路传输
	//创建文件拷贝服务器
	int client_copyfile_port;
	unsigned char *client_copyfile_ip;
	int acc_fd_copyfile = server_create(3000, NULL, &client_copyfile_port, &client_copyfile_ip);
	if(acc_fd_copyfile < 0)
	{
		perror("fail to create copyfile server");
		return -1;
	}
	printf("client ip:%s\tport:%d\n", client_copyfile_ip, client_copyfile_port);

	//创建目录创建服务器
	int client_mkdir_port;
	unsigned char *client_mkdir_ip;
	int acc_fd_mkdir = server_create(4000, NULL, &client_mkdir_port, &client_mkdir_ip);
	if(acc_fd_mkdir < 0)
	{
		perror("fail to create mkdir server");
		return -1;
	}
	printf("client ip:%s\tport:%d\n", client_mkdir_ip, client_mkdir_port);
	
	
	//创建文件拷贝线程
	pthread_t pthid_copyfile;
	ret = pthread_create(&pthid_copyfile, NULL, copyfile_routine, &acc_fd_copyfile);

	/******************* 主线程负责创建目录 ******************************/
	while(1)
	{
		/****************** 接收目录长度 ***************************/
		int dir_size = 0;
		
		ret = recv(acc_fd_mkdir, &dir_size, sizeof(int), 0);
		if(ret == 0)
		{
			printf("mkdir client offline\n");
			break;
		
		}
		printf("dir size:%d\n", dir_size);


		/****************** 接收目录名称 ***************************/
		unsigned char *dir_name = (unsigned char *)malloc(dir_size * sizeof(char) + 1);
		//清空
		bzero(dir_name, dir_size * sizeof(char) + 1);
		ret = recv(acc_fd_mkdir, dir_name, dir_size, 0);
		if(ret < 0)
		{
			perror("fail to recv dir name");
			
			pthread_join(pthid_copyfile, NULL);
			shutdown(acc_fd_copyfile, SHUT_RDWR);
			shutdown(acc_fd_mkdir, SHUT_RDWR);
			return -1;
		}

		if(ret == 0)
		{
			printf("mkdir client offline\n");
			break;
		
		}
		printf("mkdir: %s\n", dir_name);

		/********************* 创建目录 ***************************/
		ret = mkdir(dir_name, 0777);
		if(ret < 0)
		{
			//如果错误为"file exit",继续执行
			if(strcmp(strerror(errno), "File exits"))	
			{
				continue;
			}

			perror("fail to mkdir");
			pthread_join(pthid_copyfile, NULL);
			shutdown(acc_fd_copyfile, SHUT_RDWR);
			shutdown(acc_fd_mkdir, SHUT_RDWR);
			return -1;

		}


		//释放资源
		free(dir_name);

		
	}


	char join_buff[10];
	//等待线程结束
	pthread_join(pthid_copyfile, (void *)&join_buff);
	if(strcmp(join_buff, "finish") == 0)
	{
		//发送结束消息
		ret = send(acc_fd_mkdir, "finish", strlen("finish"), 0);
		if(ret < 0)
		{
			printf("error exits when tell client finish\n");
			/*
			 *backlog:
			 *
			 */
			return -1;
		}
		printf("send finish\n");

	}

	//关闭套接字
	ret = shutdown(acc_fd_copyfile, SHUT_RDWR);
	if(ret < 0)
	{
		perror("fail to shudown acc_fd");
		return -1;
	}

	//关闭套接字
	ret = shutdown(acc_fd_mkdir, SHUT_RDWR);
	if(ret < 0)
	{
		perror("fail to shudown acc_fd");
		return -1;
	}

	return 0;

}

	
void *copyfile_routine(void *arg)
{
	
	int ret;
	
	//线程分离
	//pthread_detach(pthread_self());

	//转化参数
	int acc_fd = *((int *)arg);
	while(1)
	{
		ret = recv_write_file(acc_fd);
		if(ret < 0)
		{
			//出错
			perror("error exit in recv_write_file");
			pthread_exit("error");
		}
		else if(ret == 0)
		{
			//客户端下线
			pthread_exit("offline");
			break;
		
		}
		else if(ret == 2)
		{
			//拷贝结束
			pthread_exit("finish");
			break;
		
		}
	}
}




int recv_write_file(int acc_fd)
{

	int ret;

	//定义文件长度、用户操作权限信息结构体
	FileInfo_t fileinfo;
	
	
	bzero(&fileinfo, sizeof(fileinfo));
	
	/*********************** 1.接收文件长度、用户操作权限信息 ************************/
	ret = recv(acc_fd, &fileinfo, sizeof(fileinfo), 0);
	if(ret < 0)
	{
		perror("error exits in recv size information");
		return -1;
	}
	else if(ret == 0)
	{
		printf("client offlines\n");
		goto offline;
	}
	//判断是否为结束文件
	if(fileinfo.name_size == 0 && fileinfo.text_size == 0 && fileinfo.mode == 0)
	{
		printf("it is end_file\n");
		return 2;
	
	}

	printf("\nname size:%d, text size:%d, mode:%o\n", fileinfo.name_size, fileinfo.text_size, fileinfo.mode);




	/***************************** 2.接收文件名 ************************/
	//(+1 解决malloc分配内存按4倍数)
	unsigned char *pdst_path = (unsigned char *)malloc(fileinfo.name_size * sizeof(char) + 1);
	//清空
	bzero(pdst_path, fileinfo.name_size * sizeof(char) + 1);

	ret = recv(acc_fd, pdst_path, fileinfo.name_size * sizeof(char), 0);
	if(ret < 0)
	{
		perror("error exits in recv dst path");
		//释放资源
		free(pdst_path);
		return -1;
	}
	else if(ret == 0)
	{
		printf("client offlines\n");
		free(pdst_path);
		goto offline;
	}
	printf("dst path:%s\n", pdst_path);
	
	/*********************** 3.接受文件数据 *****************/
	unsigned char *pdata = (unsigned char *)malloc(fileinfo.text_size * sizeof(char) + 1);
	//清空
	bzero(pdata, fileinfo.text_size * sizeof(char) + 1);

	ret = recv(acc_fd, pdata, fileinfo.text_size, 0);
	if(ret < 0)
	{
		perror("error exits in recv data");
		//释放资源
		goto error_recv_finish;
	}
	else if(ret == 0)
	{
		printf("client offlines\n");
		//释放资源
		free(pdata);
		free(pdst_path);
		goto offline;
	}
	printf("data:\n%s\n", pdata);

	/************************ 4.创建目标文件，写入数据 ******************/
	FILE *fd_dst = fopen(pdst_path, "w");
	if(fd_dst == NULL)
	{
		printf("fail to create %s\n", pdst_path);
		goto error_recv_finish;
	
	}
	
	//attention: 必须按照fileinfo.text_size 大小写入，strlen()对于二进制文件将失效
	/*************************** 5.写文件数据 ***********************/
	ret = fwrite(pdata, fileinfo.text_size, 1, fd_dst); 
	if(ret < 0)
	{
		printf("fail to write data into %s\n", pdst_path);
		goto error_recv_finish;
	}

	/************************** 6.关闭文件 ******************************/
	ret = fclose(fd_dst);
	if(ret < 0)
	{
		printf("fail to close %s\n", pdst_path);
		goto error_recv_finish;
	}
	
	/************************** 7.修改用户操作权限 *************************/
	//ret = chmod(pdst_path, fileinfo.stat_buff.st_mode);
	ret = chmod(pdst_path, fileinfo.mode);
	if(ret < 0)
	{
		printf("fail to chmod %s\n", pdst_path);
		goto error_recv_finish;
	}


	/************************* 7.释放资源 ************************************/
	free(pdata);
	free(pdst_path);		

	//返回1,读取一次成功
	return 1;

error_recv_finish:
	//shutdown(acc_fd, SHUT_RDWR);
	//释放资源
	free(pdata);
	free(pdst_path);	
	return -1;

offline:
	return 0;


}




#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../lib/pthread_pool/pool.h"
#include "../lib/socket/mysocket.h"
typedef struct FileInfo{

	int name_size;;
	long text_size;
	struct stat stat_buff;

}FileInfo_t;





int copy_file(unsigned char *src_path, unsigned char *dst_path);
int copy_dir(unsigned char *src_dir, unsigned char *dir_dir);

pPool_t ppool = NULL;


int soc_fd;

int main(int argc, char *argv[])
{

	int ret; 
	//创建线程池
	ppool = pool_create(1);
	if(ppool == NULL)
	{
		perror("fail to create pthread pool");
		return -1;
	}
	
	//连接客户端
	soc_fd = client_create(1000, "202.192.32.48");
	if(soc_fd < 0)
	{
		perror("fail to create client");
		return -1;
	}
	
	//搜索dir,同时将创建任务节点添加到任务链表
	ret = copy_dir(argv[1], argv[2]);
	if(ret < 0)
	{
		printf("fail to copy dir:%s", argv[1]);
		/*
		 *backlog:错误处理
		 */
		return -1;
	}
		
	wait_task_finish(ppool);	

	pool_destroy(ppool);
	return 0;

}

int copy_file(unsigned char *src_path, unsigned char *dst_path)
{
	
	int ret;

	//打开文件
	FILE *fd_src = fopen(src_path, "r");
	if(fd_src == NULL)
	{
		perror("fail to open src file");
		return -1;
	}
	
	//获取用户权限
	struct stat stat_buff;
	struct stat *pstat_buff = &stat_buff;

	ret = stat(src_path, pstat_buff);
	if(ret < 0)
	{
		printf("fail to get %s stat \n", src_path);
		return -1;
	}
	//printf("mode: %o\n", pstat_buff->st_mode);


	//读取文件
	//a.测量文件大小
	fseek(fd_src, 0, SEEK_END);
	long size = ftell(fd_src); 
	//动态分配空间
	//printf("%ld\n",size);
	
	unsigned char *pdata = (unsigned char *)malloc(size * sizeof(char) + 1);
	//清空
	memset(pdata, 0, size * sizeof(char) + 1);

	fseek(fd_src, 0, SEEK_SET);
	
	ret = fread(pdata, size * sizeof(char) + 1, 1, fd_src);
	if(ret < 0)
	{
		perror("fail to read from src file");
		return -1;
	}
	
	//定义输出文件长度信息、用户操作权限信息
	FileInfo_t fileinfo;

	fileinfo.name_size = strlen(dst_path);
	fileinfo.text_size = size;
	fileinfo.stat_buff = stat_buff;
	printf("\nname size:%d, text size:%ld, mode:%o\n", fileinfo.name_size, fileinfo.text_size, fileinfo.stat_buff.st_mode);	
	printf("dst path:%s\n", dst_path);

	//发送文件长度信息	
	ret = send(soc_fd, &fileinfo, sizeof(fileinfo), 0);
	
	//发送文件名信息
	ret = send(soc_fd, dst_path, strlen(dst_path), 0);

	//发送文件数据	
	//ret = send(soc_fd, pdata, strlen(pdata), 0);
	
	//释放堆内存
	free(pdata);

	//关闭文件
	ret = fclose(fd_src);
	if(ret < 0)
	{
		perror("fail to close src file");
		return -1;
	}

	//printf("copy %s to %s\n", src_path, dst_path);
	return 0;
}




int copy_dir(unsigned char *src_dir, unsigned char *dst_dir)
{
	
	int ret;	
	//目录操作
	DIR *fd_dir = opendir(src_dir);
	if(fd_dir == NULL)
	{
		perror("fail to open dir");
		return -1;
	}
	
	
	struct dirent *dir_data = NULL;
	int i=0;
	while(dir_data = readdir(fd_dir))
	{
		//计算文件名长度
		int src_path_size, dst_path_size;
		src_path_size = strlen(src_dir) + dir_data->d_reclen/8 + 1;
		dst_path_size = strlen(dst_dir) + dir_data->d_reclen/8 + 1;
			
		unsigned char *src_path = (unsigned char *)malloc(src_path_size);
		/*
		 *
		 *size???
		 *
		 */
		
		//printf("%d\n", src_path_size);
		memset(src_path, 0, src_path_size);
		unsigned char *dst_path = (unsigned char *)malloc(dst_path_size);
		memset(dst_path, 0, dst_path_size);
		//赋值文件名
		sprintf(src_path, "%s/%s", src_dir, dir_data->d_name);
		sprintf(dst_path, "%s/%s", dst_dir, dir_data->d_name);
		

		//目录
		if(dir_data->d_type == DT_DIR)
		{ 	
			//跳过. 、..
			if(!strcmp(dir_data->d_name, ".") || !strcmp(dir_data->d_name, ".."))
			{
				//释放资源
				free(src_path);
				free(dst_path);
				src_path = NULL;
				dst_path = NULL;
				continue;
			}
			else 
			{
				//创建目录
				//ret = mkdir(dst_path, 0777);
				if(ret < 0)
				{
					printf("fail to mkdir %s\n", dst_path);
					return -1;
				}	


				ret = copy_dir(src_path, dst_path);
				if(ret < 0)
				{
					printf("fail to copy dir: %s\n", src_path);
					return -1;
				}
				//释放资源
				free(src_path);
				free(dst_path);
				src_path = NULL;
				dst_path = NULL;
				
			
			}
		
		}
		//文件
		else 
		{
			//添加任务到任务链表
			//printf("main here add arg:%s\t%s\n", src_path, dst_path);
			ret = pool_add_task(ppool, copy_file, src_path, dst_path);
			
			



			if(ret < 0)
			{
				printf("fail to add %s file to task list", src_path);
				return -1;
			}
			
			//遍历任务链表
			travel(ppool->ptask_head);	
		}
	
		//释放堆内存
		//free(src_path);
		//free(dst_path);
		//src_path = NULL;
		//dst_path = NULL;

	}

	//遍历任务链表
	//travel(ppool->ptask_head);
	
	ret = closedir(fd_dir);
	if(ret < 0)
	{
		perror("fail to close dir");
		return -1;
	}

	return 0;

}



/////////////////////////////////////////////////////////////////////////////////
int copy_file_sys(unsigned char *src_path, unsigned char *dst_path)
{
	
	int ret;

	//打开文件
	int fd_src = open(src_path, O_RDONLY);
	if(fd_src < 0)
	{
		perror("fail to open src file");
		return -1;
	}
	
	//读取文件
	//a.测量文件大小
	long size = lseek(fd_src, 0, SEEK_END);
	//动态分配空间
	unsigned char *pdata = (unsigned char *)malloc(size * sizeof(char) + 1);
	//清空
	memset(pdata, 0, size * sizeof(char) + 1);

	lseek(fd_src, 0, SEEK_SET);
	
	ret = read(fd_src, pdata, size * sizeof(char) + 1);
	if(ret < 0)
	{
		perror("fail to read from src file");
		return -1;
	}
	
	//printf("%s\n", pdata);
	
	//写文件
	int fd_dst = open(dst_path, O_RDWR | O_CREAT , 0776);
	if(fd_src < 0)
	{
		perror("fail to create dst file");
	
	}
	//???????
	//printf("sizeof size:%ld,strlen size:%ld", sizeof(pdata), strlen(pdata));	
	ret = write(fd_dst, pdata, strlen(pdata));
	if(ret < 0)
	{
		perror("fail to write into dst file");
		return -1;
	}

	//释放堆内存
	free(pdata);

	//关闭文件
	ret = close(fd_src);
	if(ret < 0)
	{
		perror("fail to close src file");
		return -1;
	}

	ret = close(fd_dst);
	if(ret < 0)
	{
		perror("fail to close dst file");
		return -1;
	}


	printf("copy %s to %s\n", src_path, dst_path);
	return 0;
}



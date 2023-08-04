/* select server.c */
#include <stdio.h>		
#include <stdlib.h>		//exit()函数相关
#include <unistd.h>		//c 和 c++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件
#include <sys/types.h>		//Unix/Linux系统的基本系统数据类型的头文件,含有size_t,time_t,pid_t等类型
#include <sys/socket.h>		//套接字基本函数相关
#include <netinet/in.h>		//IP地址和端口相关定义，比如struct sockaddr_in等
#include <arpa/inet.h>		//inet_pton()等函数相关
#include <string.h>		//bzero()函数相关
#include <signal.h>   //sigint信号
#include <errno.h>
#include <time.h>
#include <ctype.h>

#define MAXLINE 2000
#define SERV_PORT 12330
char send1[MAXLINE] = " lacus eu dignissim fringilla, vitae tincidunt elit tempus eu. Nam vehicula, nisl at tincidunt fermentum, velit mi lobortis dui, et viverra velit elit sit amet sem.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean ac tortor at metus ultrices semper. Fusce tincidunt justo at nibh rhoncus convallis. Donec efficitur tempus ipsum ac malesuada. Nulla id blandit lorem, id pellentesque sem. Suspendisse feugiat bibendum semper. Vestibulum venenatis risus at lobortis lacinia. Sed posuere lectus vitae elit feugiat, nec fringilla eros cursus. Praesent sed ligula eu sapien suscipit efficitur. Vivamus euismod nunc id dictum iaculis. Mauris consectetur venenatis ante vitae sagittis. Nulla quis ante a massa interdum maximus. Phasellus fermentum, leo at tempor consectetur, elit elit semper odio, nec dictum mauris est in velit. Quisque interdum nunc eu sem ultricies venenatis. Integer faucibus pellentesque ex sed bibendum. Sed vehicula, lacus eu dignissim fringilla, leo neque scelerisque neque, nec maximus nunc est eu sem. Aenean lobortis enim velit, vitae tincidunt elit tempus eu. Nam vehicula, nisl at tincidunt fermentum, velit mi lobortis dui, et viverra velit elit sit amet sem.\n";
int main(int argc, char *argv[])
{
    int i, maxi, maxfd;
	int listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    ssize_t n;
	
    char* IP = argv[1];
    int PORT = atoi(argv[2]);
	//两个集合
    fd_set rset, allset;
	
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN]; /* #define INET_ADDRSTRLEN 16 */
    socklen_t cliaddr_len;
    struct sockaddr_in cliaddr, servaddr;
	
	//创建套接字
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	//绑定
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &(servaddr.sin_addr));
    servaddr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	
	//监听
    listen(listenfd, 1024); /* 默认最大128 */
	
	//需要接收最大文件描述符
    maxfd = listenfd; 
	
	//数组初始化为-1
    maxi = -1; 
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;
	
	//集合清零
    FD_ZERO(&allset);
	
	//将listenfd加入allset集合
    FD_SET(listenfd, &allset);
	
    for ( ; ; ) 
	{
		//关键点3
        rset = allset; /* 每次循环时都重新设置select监控信号集 */
		
		//select返回rest集合中发生读事件的总数  参数1：最大文件描述符+1
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);	
        if (nready < 0)
            printf("select error");
		
		//listenfd是否在rset集合中
        if (FD_ISSET(listenfd, &rset))
		{
			//accept接收
            cliaddr_len = sizeof(cliaddr);
			//accept返回通信套接字，当前非阻塞，因为select已经发生读写事件
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
            printf("received from %s at PORT %d\n",
                   inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
                   ntohs(cliaddr.sin_port));
			
			//关键点1	
            for (i = 0; i < FD_SETSIZE; i++)
                if (client[i] < 0) 
				{
                    client[i] = connfd; /* 保存accept返回的通信套接字connfd存到client[]里 */
                    break;
                }
				
            /* 是否达到select能监控的文件个数上限 1024 */
            if (i == FD_SETSIZE) {
                fputs("too many clients\n", stderr);
                exit(1);
            }
			
			//关键点2
            FD_SET(connfd, &allset); /*添加一个新的文件描述符到监控信号集里 */
			
			//更新最大文件描述符数
            if (connfd > maxfd)
                maxfd = connfd; /* select第一个参数需要 */
            if (i > maxi)
                maxi = i; /* 更新client[]最大下标值 */
			
			/* 如果没有更多的就绪文件描述符继续回到上面select阻塞监听,负责处理未处理完的就绪文件描述符 */
            if (--nready == 0)
                continue; 
        }
		
        for (i = 0; i <= maxi; i++) 
		{ 
			//检测clients 哪个有数据就绪
            if ( (sockfd = client[i]) < 0)
                continue;
			
			//sockfd（connd）是否在rset集合中
            if (FD_ISSET(sockfd, &rset)) 
			{
				//进行读数据 不用阻塞立即读取（select已经帮忙处理阻塞环节）
                if ( (n = read(sockfd, buf, MAXLINE)) == 0)
				{
                    /* 无数据情况 client关闭链接，服务器端也关闭对应链接 */
                    close(sockfd);
                    FD_CLR(sockfd, &allset); /*解除select监控此文件描述符 */
                    client[i] = -1;
                } else 
				{
					//有数据
                    write(sockfd, send1, sizeof(send1));//写回客户端
                }
                if (--nready == 0)
                    break;
            }
		} 
			
	}
    close(listenfd);
    return 0;
}

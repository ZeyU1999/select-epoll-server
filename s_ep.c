#include <stdlib.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <stdio.h>		
#include <unistd.h>		//C 和 C++ 程序设计语言中提供对 POSIX 操作系统 API 的访问功能的头文件
#include <sys/types.h>		//Unix/Linux系统的基本系统数据类型的头文件,含有size_t,time_t,pid_t等类型
#include <sys/socket.h>		//套接字基本函数相关
#include <netinet/in.h>		//IP地址和端口相关定义，比如struct sockaddr_in等
#include <arpa/inet.h>		//inet_pton()等函数相关
#include <string.h>		//bzero()函数相关
#include <signal.h>   //sigint信号
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#define MAXLINE 2000
#define MAX_EVENTS 1024
char send1[MAXLINE] =" lacus eu dignissim fringilla, vitae tincidunt elit tempus eu. Nam vehicula, nisl at tincidunt fermentum, velit mi lobortis dui, et viverra velit elit sit amet sem.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean ac tortor at metus ultrices semper. Fusce tincidunt justo at nibh rhoncus convallis. Donec efficitur tempus ipsum ac malesuada. Nulla id blandit lorem, id pellentesque sem. Suspendisse feugiat bibendum semper. Vestibulum venenatis risus at lobortis lacinia. Sed posuere lectus vitae elit feugiat, nec fringilla eros cursus. Praesent sed ligula eu sapien suscipit efficitur. Vivamus euismod nunc id dictum iaculis. Mauris consectetur venenatis ante vitae sagittis. Nulla quis ante a massa interdum maximus. Phasellus fermentum, leo at tempor consectetur, elit elit semper odio, nec dictum mauris est in velit. Quisque interdum nunc eu sem ultricies venenatis. Integer faucibus pellentesque ex sed bibendum. Sed vehicula, lacus eu dignissim fringilla, leo neque scelerisque neque, nec maximus nunc est eu sem. Aenean lobortis enim velit, vitae tincidunt elit tempus eu. Nam vehicula, nisl at tincidunt fermentum, velit mi lobortis dui, et viverra velit elit sit amet sem.\n";

/*将文件描述符设置为非阻塞*/
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}



int main(int argc, char *argv[])
{
    struct epoll_event  ev, events[MAX_EVENTS];
    int nfds, epollfd;
	int listen_sock, conn_sock, sockfd,clilen,i;
    int n;
	
    char* IP = argv[1];
    int PORT = atoi(argv[2]);
    int link_num = atoi(argv[3]);
    int count = 0;

    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN]; /* #define INET_ADDRSTRLEN 16 */
    socklen_t cliaddr_len;
    struct sockaddr_in cliaddr, servaddr;

    //开始计时
    clock_t start,end;
    start =clock();
	
	//创建套接字
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	
	//绑定
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &(servaddr.sin_addr));
    servaddr.sin_port = htons(PORT);
    if(bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        puts("bind error!");
        return 0;
    }
	
	//监听
    listen(listen_sock, 1024); 
	
    //创建一个epoll
    epollfd = epoll_create1( 0 );
    if ( epollfd == -1 )
    {
        perror( "epoll_create1" );
        exit( EXIT_FAILURE );
    }
    //把监听套接字放进去
    ev.events   = EPOLLIN;
    ev.data.fd  = listen_sock;
    if ( epoll_ctl( epollfd, EPOLL_CTL_ADD, listen_sock, &ev ) == -1 )
    {
        perror( "epoll_ctl: listen_sock" );
        exit( EXIT_FAILURE );
    }
    
    for (;; ){
        //检测是否有目标状态发生了变化
        nfds = epoll_wait( epollfd, events, MAX_EVENTS, -1 );//返回是满足event条件的fd数量
        if ( nfds == -1 )
        {
            perror( "epoll_wait" );
            exit( EXIT_FAILURE );
        }

        for ( n = 0; n < nfds; ++n ){
            if ( events[n].data.fd == listen_sock )
            {
                //如果监听套接字可读则接收一个套接字
                clilen = sizeof(cliaddr);
                conn_sock = accept( listen_sock,(struct sockaddr *) &cliaddr, &clilen);
                if ( conn_sock == -1 )
                {
                    perror( "accept" );
                    exit( EXIT_FAILURE );
                }
                setnonblocking( conn_sock );
                //计数
                count++;
                printf("link_num:%d\n",count);
                if(count == link_num){
                    end = clock();
                    printf(">>>>>>>>>>>>>Link time=%f\n",difftime(end,start)/CLOCKS_PER_SEC);
                }
                ev.events   = EPOLLIN | EPOLLET;
                ev.data.fd  = conn_sock;
                if ( epoll_ctl( epollfd, EPOLL_CTL_ADD, conn_sock,&ev ) == -1 )
                {
                    perror( "epoll_ctl: conn_sock" );
                    exit( EXIT_FAILURE );
                }
            } else {
                //将connfd赋值给socket
                sockfd = events[n].data.fd;
                //读取数据
                int num = read(sockfd, buf, MAXLINE);
                printf("cli msg:%s",buf);
                bzero(buf,sizeof(buf));
                //无数据则删除该结点
                if (num == 0){
                    //删除树结点
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)printf("epoll_ctl");
                    close(sockfd);
                    printf("client[%d] closed connection\n", sockfd);
                }else {
                    //有数据则写回数据
                    printf("send to cli[%d]",sockfd);
                    write(sockfd, send1 , strlen(send1));
                }
            }
        }
    }
}


#include <stdio.h>		
#include <stdlib.h>		
#include <unistd.h>		
#include <sys/types.h>	
#include <sys/socket.h>	
#include <netinet/in.h>	
#include <arpa/inet.h>	
#include <string.h>		
#include <signal.h>   

#define MAX_MSG_LEN	200

char	buf[MAX_MSG_LEN];
int sigint_flag = 0;
char empty_str[] = "\n\0";
void handle_sigint(int sig){
  printf("[cli] SIGINT is coming!\n");
  sigint_flag = 1;
}

void load_sigint(){
  struct sigaction sa;
  sa.sa_flags = 0;
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT,&sa,NULL);
}

void cli_biz(int clientfd) {
 //打开文件
  FILE* fd = fopen("message.txt","r");
  //读入缓冲区
  char	buf[MAX_MSG_LEN];
  while (1) {
    sleep(1);
    bzero(buf, sizeof(buf));
    // 读取操作人员在命令行输入的一行字符
    // printf("[ECH_REQ]:");
    // scanf("%s",buf);

    //从文件中读入
    if((fgets(buf,sizeof(buf),fd)) == NULL){
      strcpy(buf,"EXIT\n");
      // 发送消息给服务器
      write(clientfd, buf, strlen(buf));
      return;
    }     

    printf("[ECH_REQ]:%s", buf);
    // 检查是否要断开连接
    if (strcmp(buf, "EXIT\n") == 0) {
      ssize_t num_bytes = write(clientfd, empty_str, 1);
      break;
    }

    // 发送消息给服务器
    if (write(clientfd, buf, strlen(buf)) == -1) {
      perror("write failed");
      exit(1);
    }

    //接收服务器的响应
    ssize_t num_bytes = read(clientfd, buf, sizeof(buf));
    if (num_bytes == -1) {
      perror("read failed");
      exit(1);
    } else if (num_bytes == 0) {
      // 服务器关闭了连接
      break;
    }
    // 打印服务器的响应
    printf("[ECH_REP]:%s", buf);
  }
  return;
}

int main(int argc, char *argv[]) {
  load_sigint();

  int clientfd;
  struct sockaddr_in server_addr;

  if (argc != 3) {
    printf("usage: %s <server IP address> <server port>\n", argv[0]);
    exit(1);
  }

  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Create socket failed.");
    exit(1);
  }
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) == 0) {
    perror("Server IP Address Error:\n");
    exit(1);
  }
  server_addr.sin_port = htons(atoi(argv[2]));

  //建立连接
  if (connect(clientfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Connect failed.");
    exit(1);
  }

  printf("[cli] server[%s:%s] is connected!\n", argv[1], argv[2]);

  //进入收发循环
  cli_biz(clientfd);

  close(clientfd);
  printf("[cli] connfd is closed!\n");
  printf("[cli] connfd is to return\n");
  return 0;
}

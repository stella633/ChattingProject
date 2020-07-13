#include "unp.h"
//클라이언트 리스트 구조체
struct cli_list {
    int num;
    int count;
    int sockfd[FD_SETSIZE];
    struct sockaddr_in cliaddr[FD_SETSIZE];
}list;

int main(int argc, char **argv) {
    int i,j,k, maxi, maxfd, listenfd, connfd, sockfd;
    int nready,client[FD_SETSIZE];
    ssize_t n;
    fd_set rset,allset;
    char buf[MAXLINE], buf2[MAXLINE], buf3[MAXLINE], buf4[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr, sock, sock2;
    struct hostent *host;
    char name[30];
    socklen_t len;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0 );

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    Listen(listenfd, LISTENQ);

    maxfd = listenfd;
    maxi = -1;
    list.num=0;
    for(i=0;i<FD_SETSIZE;i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
    //서버 주소 출력
    gethostname(name,sizeof(name));
    host = gethostbyname(name);
    printf("[server address is %s:%d]\n",inet_ntoa(*(struct in_addr*)*host->h_addr_list),ntohs(servaddr.sin_port));

    for( ; ; ) {
        rset = allset;
        nready = Select(maxfd +1,&rset,NULL,NULL,NULL);

        if(FD_ISSET(listenfd,&rset)) {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd,(SA *) &cliaddr, &clilen);
            printf("connected from %s:%d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,buf3,sizeof(buf3)),ntohs(cliaddr.sin_port));
            // 새로운 클라이언트 접속 모든 클라이언트에게 알리는 메세지 만들기
            sprintf(buf2,"[%s:%d is connected]\n",inet_ntop(AF_INET,&cliaddr.sin_addr,buf3,sizeof(buf3)),ntohs(cliaddr.sin_port));

            //새로운 클라이언트 접속 전달
            for(i=0;i<=maxi;i++){
                if((sockfd=client[i])<0)
                    continue;
                Writen(client[i],buf2,strlen(buf2));
            }
            //클라이언트 리스트에 추가
            if(list.sockfd[list.num]==0 && connfd!=0){
                list.sockfd[list.num]=connfd;
                list.cliaddr[list.num]=cliaddr;
                list.num++;
                list.count++;
            }


            for(i=0;i<FD_SETSIZE;i++){
                if(client[i]<0) {
                    client[i] = connfd;
                    break;
                }
            }

            if(i ==FD_SETSIZE)
                err_quit("too many clients");
            FD_SET(connfd,&allset);
            if(connfd>maxfd)
                maxfd = connfd;
            if(i>maxi)
                maxi = i;
            if(--nready <=0)
                continue;
        }
        for(i=0;i<=maxi;i++){
            if((sockfd=client[i])<0)
                continue;
            if(FD_ISSET(sockfd,&rset)) {
                if((n=Read(sockfd,buf,MAXLINE)) ==0){
                    Close(sockfd);
                    FD_CLR(sockfd,&allset);
                    client[i] = -1;
                    for(j=0;j<list.count;j++)
                    {
                        if(list.sockfd[j]==sockfd){
                            sock=list.cliaddr[j];
                            printf("leaved at %s:%d\n",inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)),ntohs(sock.sin_port));
                            //기존 클라이언트 종료 전달
                            sprintf(buf2,"[%s:%d is leaved]\n",inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)),ntohs(sock.sin_port));
                            list.sockfd[j]=0;
                            list.count--;
                        }
                    }
                    memset(buf,0,strlen(buf));
                    for(j=0;j<=maxi;j++){
                        if((sockfd=client[j])<0)
                            continue;
                        Writen(sockfd,buf2,strlen(buf2));
                    }

                }
                else if(!strcmp(buf,"/list\n"))
                {//리스트 출력
                    sprintf(buf2,"[the number of current user is %d]\n",list.count);
                    for(j=0;j<list.num;j++)
                    {
                        if(list.sockfd[j]!=0){
                            sock=list.cliaddr[j];
                            sprintf(buf4,"[%s:%d]\n",inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)),ntohs(sock.sin_port));
                            strcat(buf2,buf4);
                        }
                    }
                    Writen(client[i],buf2,strlen(buf2));
                    memset(buf4,0,MAXLINE);
                    memset(buf,0,MAXLINE);
                }
                else
                {
                    strcpy(buf4,buf);
                    //귓속말 기능
                    char *ptr = strtok(buf, " ");      
                    if(!strcmp(ptr,"/smsg"))
                    {
                        ptr = strtok(NULL, ":");
                        strcpy(buf2,ptr);
                        ptr=strtok(NULL," ");
                        for(j=0;j<FD_SETSIZE;j++)
                        {   
                            if(list.sockfd[j]!=0){
                                sock=list.cliaddr[j];
                                if(!strcmp(buf2,inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3))))
                                {
                                    if(atoi(ptr)==ntohs(sock.sin_port))
                                    {
                                        for(k=0;k<FD_SETSIZE;k++)
                                        {
                                            if(list.sockfd[k]==sockfd){
                                                sock2=list.cliaddr[k];
                                                ptr=strtok(NULL,"\n");
                                                sprintf(buf2,"[smsg from %s:%d]%s\n",inet_ntop(AF_INET,&sock2.sin_addr,buf3,sizeof(buf3)),ntohs(sock2.sin_port),ptr);
                                                Writen(list.sockfd[j],buf2,strlen(buf2));
                                                memset(buf,0,MAXLINE);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }else{
                        //메세지에 ip:port 추가
                        for(j=0;j<FD_SETSIZE;j++)
                        {   
                            if(list.sockfd[j]==sockfd){
                                sock=list.cliaddr[j];
                                sprintf(buf2,"[%s:%d] %s",inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)),ntohs(sock.sin_port),buf4);
                            }
                        }
                        for(j=0;j<=maxi;j++){
                            if((sockfd=client[j])<0)
                                continue;
                            if(i!=j)
                                Writen(sockfd,buf2,strlen(buf2));
                        }
                        memset(buf,0,MAXLINE);
                    }
                }

                if(--nready <=0)
                    break;
            }
        }
    }

}

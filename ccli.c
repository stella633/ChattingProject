#include "unp.h"
struct black_list {
    int num;
    int count;
    struct sockaddr_in addr[FD_SETSIZE];
}list;

void str_cli(FILE *fp, int sockfd) {
    int maxfdp1, stdineof;
    fd_set rset;
    struct sockaddr_in sock;
    char buf[MAXLINE], buf2[MAXLINE], buf3[MAXLINE],buf4[MAXLINE];
    char *ptr;
    int n,i,flag;

    stdineof = 0;
    FD_ZERO(&rset);
    for( ; ; ) {
        if(stdineof ==0)
            FD_SET(fileno(fp),&rset);
        FD_SET(sockfd,&rset);
        maxfdp1 = max(fileno(fp),sockfd)+1;
        Select(maxfdp1, &rset,NULL,NULL,NULL);

        if(FD_ISSET(sockfd,&rset)) {        
            if((n=Read(sockfd,buf,MAXLINE))==0)
                if(stdineof ==1)
                    return;
                else
                    err_quit("str_cli : server  terminated prematuerly");
            flag=0;
            for(i=0;i<FD_SETSIZE;i++)
            {   
                sock=list.addr[i];
                if(ntohs(sock.sin_port)!=0)
                {
                    sprintf(buf2,"%s:%d",inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)),ntohs(sock.sin_port));
                    if(strstr(buf,buf2)!=NULL && strstr(buf,"the number of current user is")==NULL)
                        flag=1;
                }   
            }   
            if(flag==0)
                Writen(fileno(stdout),buf,n);
            memset(buf,0,MAXLINE);
        }
        if(FD_ISSET(fileno(fp),&rset)) {
            if((n=Read(fileno(fp),buf,MAXLINE))==0) {
                stdineof=1;
                Shutdown(sockfd,SHUT_WR);
                FD_CLR(fileno(fp),&rset);
                continue;
            }
            strcpy(buf4,buf);
            ptr = strtok(buf, " ");
            if(!strcmp(buf,"/quit\n"))
                exit(0);
            else if(!strcmp(ptr,"/block"))
            {
                //차단 리스트 추가
                ptr = strtok(NULL, ":");
                sock.sin_family = AF_INET;
                sock.sin_addr.s_addr = inet_addr(ptr);
                ptr= strtok(NULL,"\n");
                sock.sin_port = htons(atoi(ptr));
                list.addr[list.num]=sock;
                list.num++;
                list.count++;
                memset(buf,0,MAXLINE);
            }
            else if(!strcmp(buf,"/blocklist\n")){
                //차단 리스트 출력
                sprintf(buf2,"[the number of block user is %d]\n",list.count);
                for(i=0;i<list.num;i++)
                {
                    sock=list.addr[i];
                    if(ntohs(sock.sin_port)!=0)
                    {
                        strcat(buf2,"[");
                        strcat(buf2,inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)));
                        strcat(buf2,":");
                        sprintf(buf3,"%d",ntohs(sock.sin_port));
                        strcat(buf2,buf3);
                        strcat(buf2,"]\n");
                    }
                }
                Writen(fileno(stdout),buf2,sizeof(buf2));
                memset(buf2,0,MAXLINE);
                memset(buf,0,MAXLINE);
            }
            else if(!strcmp(ptr,"/release"))
            {
                //차단 해제
                ptr = strtok(NULL, "\n");
                for(i=0;i<list.num;i++)
                {
                    sock=list.addr[i];
                    if(ntohs(sock.sin_port)!=0)
                    {   
                        strcpy(buf2,inet_ntop(AF_INET,&sock.sin_addr,buf3,sizeof(buf3)));
                        strcat(buf2,":");
                        sprintf(buf3,"%d",ntohs(sock.sin_port));
                        strcat(buf2,buf3);
                        if(strstr(ptr,buf2)!=NULL){
                            list.addr[i].sin_port=0;
                            list.count--;
                        }
                    }
                }
                memset(buf,0,MAXLINE);
            }
            else{
                Writen(sockfd,buf4,n);
                memset(buf,0,MAXLINE);
            }
        }
    }
}
int main(int argc, char **argv){
    int sockfd;
    struct sockaddr_in servaddr;

    if(argc != 3)
        err_quit("usage : tcpcli <IPaddress> <Port>");

    sockfd = Socket(AF_INET, SOCK_STREAM, 0); 

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
    str_cli(stdin, sockfd);

    exit(0);
}


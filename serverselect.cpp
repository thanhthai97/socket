#include<iostream>
//#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
//#include<errno.h>
#include<signal.h>
#include<sys/signalfd.h>
#include<inttypes.h>
#include<fcntl.h>
#include<stdarg.h>
#include<memory.h>
//#include<sys/socketvar.h>
//#include<fcntl.h>
#define SERVER_PORT 8000
#define MAX_BUFFER 1024
#define MAX_CLIENT 65525
#define IP3 "127.0.0.1"
#define IP4 "192.168.81.12"

int sockfd;
char buff[MAX_BUFFER], buffer[MAX_BUFFER];
FILE *filegeneral,*filerandom;

int setup_signalfd() {
	int sfd, ret;
	sigset_t sigset;

	ret = sigprocmask(SIG_SETMASK, NULL, &sigset);
	if (ret < 0)
    {
		perror("sigprocmask.1");
    }
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	ret = sigprocmask(SIG_SETMASK, &sigset, NULL);
	if (ret < 0)
    {
		perror("sigprocmask.2");
    }
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);

	sfd = signalfd(-1, &sigset, 0);
	if (sfd < 0)
    {
		perror("signalfd");
    }
	printf("sfd is %i\n", sfd);

	return sfd;
}

void read_sig(int sfd) {
	struct signalfd_siginfo info;
	int ret;

	ret = read(sfd, &info, sizeof info);
	if (ret != sizeof info)
    {
		perror("!?!?!?!");
    }
	printf("Recibida signal\n");
	printf("signo = %" PRIu32 "\n", info.ssi_signo);
	printf("pid   = %" PRIu32 "\n", info.ssi_pid);
	printf("uid   = %" PRIu32 "\n", info.ssi_uid);
    	printf("Exiting !!!\n");
    	exit(0);
}

void createfile()
{
    filegeneral = fopen("datageneral.txt","w");
    if(filegeneral == NULL)
    {
        printf("Create file Error !!!");
        exit(1);
    }
    fclose(filegeneral);
    filerandom = fopen("datarandom.txt","w");
    if(filerandom == NULL)
    {
        printf("Create file Error !!!");
        exit(1);
    }
    fclose(filerandom);
}

void writedata_random2file()
{
    read(sockfd,buffer,sizeof(buffer));
    filerandom = fopen("datarandom.txt","a");
    fprintf(filerandom,"%s\n",buffer);
    fclose(filerandom);
    printf("Wrote %s from fd %d to datarandom\n",buffer,sockfd);
}

void writedata_general2file()
{
    read(sockfd,buff,sizeof(buff));
    
    filegeneral = fopen("datageneral.txt","a");
    fprintf(filegeneral,"%s\n",buff);
    fclose(filegeneral);
    printf("Wrote %s from fd %d to datageneral\n",buff,sockfd);
}


int main()
{
    std::cout<<"PID process server running: "<<(int)getpid()<<std::endl;

    
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    int listenfd, connectfd;
    
    int nread, client3, client4;
    char str[INET_ADDRSTRLEN];
    
    // Initial socket------------------------------------------------------
    listenfd = socket(AF_INET,SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("Create socket");
        exit(0);
    }

    memset(&servaddr,0,sizeof(servaddr));

    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);
    
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    // non_block lệnh accept của socket khi có kết nối đến
    int status = fcntl(listenfd,F_SETFL,O_NONBLOCK);
    if(status == -1) perror("control fcnl");

    listen(listenfd,20);
    //-------------------------------------------------------------------
    ssize_t n_select, sfd;
    sfd = setup_signalfd();
    // Initial select--------------------------------------------------

    fd_set readfds, masterfds;
    FD_ZERO(&masterfds);
    FD_ZERO(&readfds);

    FD_SET(listenfd,&masterfds);
    FD_SET(sfd,&masterfds);

    // Initial timeout
    struct timeval timeout;
    timeout.tv_sec = 90;
    timeout.tv_usec = 0;

    int max_fd = listenfd;
    
    createfile();
    
    while(1)
    {
        printf("server waiting ....\n");

        memcpy(&readfds,&masterfds,sizeof(masterfds));
        n_select = select(max_fd+1,&readfds,NULL,NULL,&timeout);
               
        if(n_select < 0)
        {
            perror("SELECT");
            exit(0);
        }
        else
            if (n_select == 0)
            {
                printf("Time out \n");
            }
            else{
            for(int i = 0;i <= max_fd;i++)
            {
                if(FD_ISSET(i,&readfds))
                {
                    
                    if(i == listenfd)
                    {
                        clilen = sizeof(cliaddr);
                        connectfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
                        if (inet_addr(str)==inet_addr(IP3))
                        {
                            client3 = connectfd;
                        }
                        else
                        {
                            client4 = connectfd;
                        }
                        printf("received from %s at PORT %d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,str,INET_ADDRSTRLEN),ntohs(cliaddr.sin_port));
                        
                        FD_SET(connectfd,&masterfds);
                        if(connectfd > max_fd)
                        {
                            max_fd = connectfd;
                        }
                    }

                    else if(i == sfd)
                    {
                        read_sig(sfd);
                    }
                    else
                    {
                        sockfd = i;
                        
                        ioctl(sockfd,FIONREAD,&nread);
                        if(nread==0)
                        {
                            FD_CLR(i,&masterfds);
                            close(i);
                            printf("client [%d] closed connection\n",sockfd);
                        }
                        
                        else
                        {
                            if(sockfd == client3)
                            {
                                writedata_general2file();            
                            }
                            else if(sockfd == client4)
                            {
                                writedata_random2file();    
                            }
                        }
                        
                    }

                }
                
                
            }
        }
    }
    return 0;
}

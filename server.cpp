#include<iostream>
//#include<sys/types.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include<signal.h>
#include<sys/signalfd.h>
#include<inttypes.h>
#include<fcntl.h>
#include<stdarg.h>
#include<memory.h>
//#include<sys/socketvar.h>
//#include<fcntl.h>
#define SERV_PORT 8000
#define MAX_BUFFER 1024
#define MAX_CLIENT 65525

int setup_signalfd() {
	int sfd, ret;
	sigset_t sigset;

	ret = sigprocmask(SIG_SETMASK, NULL, &sigset);
	if (ret < 0)
		perror("sigprocmask.1");

	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	ret = sigprocmask(SIG_SETMASK, &sigset, NULL);
	if (ret < 0)
		perror("sigprocmask.2");

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);

	sfd = signalfd(-1, &sigset, 0);
	if (sfd < 0)
		perror("signalfd");

	printf("sfd is %i\n", sfd);

	return sfd;
}

void read_sig(int sfd) {
	struct signalfd_siginfo info;
	int ret;

	ret = read(sfd, &info, sizeof info);
	if (ret != sizeof info)
		perror("read sfd");

	printf("Recibida signal\n");
	printf("signo = %" PRIu32 "\n", info.ssi_signo);
	printf("pid   = %" PRIu32 "\n", info.ssi_pid);
	printf("uid   = %" PRIu32 "\n", info.ssi_uid);
    printf("Exiting !!!\n");
    exit(0);
}

int main()
{
    std::cout<<"PID process server running: "<<(int)getpid()<<std::endl;
    int i,listenfd, connfd, sockfd; // varibale for file disriptor
    int n, client3, client4;
     
    ssize_t nready, efd, res, sfd;
    char buff[MAX_BUFFER], buffer[MAX_BUFFER], str[INET_ADDRSTRLEN];
    
    
    socklen_t clilen; // lenght of socket client
    int client[MAX_CLIENT];
    struct sockaddr_in cliaddr, servaddr; // struct for socket server and client
    struct epoll_event tep, ep[MAX_CLIENT];

    listenfd = socket(AF_INET,SOCK_STREAM, 0); // create socket with stype sock stream
    memset(&servaddr,0,sizeof(servaddr));

    servaddr.sin_family=AF_INET; // socket stype AF_NET
    servaddr.sin_addr.s_addr = INADDR_ANY; //address for socket server apply any address of client
    servaddr.sin_port = htons(SERV_PORT); // port of socket server is 8000
    // bind socket server to server address
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    listen(listenfd,20); // always listen
    efd = epoll_create(MAX_CLIENT); //create epoll with 65525 element
    sfd = setup_signalfd();

    tep.events = EPOLLIN; tep.data.fd = listenfd; // create epoll event and add event of socket server
    res = epoll_ctl(efd,EPOLL_CTL_ADD,listenfd,&tep); // add event listen of socket server to epoll list
    tep.data.fd = sfd;
    tep.events = EPOLLIN;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &tep) != 0)
	perror("epoll_ctl");

    // create file to write data from clients
    FILE *file = fopen("data.txt","w");
    if(file == NULL)
    {
        printf("Open file Error !!!");
        exit(1);
    }
    fclose(file);
    FILE *file2 = fopen("data2.txt","w");
    if(file2 == NULL)
    {
        printf("Open file Error !!!");
        exit(1);
    }
    fclose(file2);
    //----------------------------------------------
    while(true)
    {
        printf("server waiting ...\n");
        // epoll wait
        nready = epoll_wait(efd,ep,65525,-1);
        //check event
        for(i =0;i<nready;i++)
        {
            // if there is a event happen if not exits on list
            if(ep[i].data.fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                // server accept the connect from client to server socket
                connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
                //nosocket.push_back(connfd);
                
                printf("received from %s at PORT %d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,str,INET_ADDRSTRLEN),ntohs(cliaddr.sin_port));
                // check the IP address of the client
                
                if (inet_addr(str)==inet_addr("127.0.0.1"))
                {
                    client3 = connfd;
                }
                else
                {
                    client4 = connfd;
                }
                
                

                printf("+ client3 =  %d client4 = %d\n",client3,client4);
                // add the new event to list
                tep.events = EPOLLIN; tep.data.fd = connfd; 
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd,&tep);

            }
	        else if(ep[i].data.fd == sfd)
            {
                read_sig(sfd);
            }
            else
            {
                sockfd = ep[i].data.fd;
                                
                //n = read(sockfd,buf,MAXLINE);
                ioctl(sockfd,FIONREAD,&n);
                // if don't have the data read from client server will erase the connect
                if(n==0)
                {
                    res = epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
                    close(sockfd);
                    printf("client [%d] closed connection\n",sockfd);
                }
                //printf("check IP: %s\n",inet_ntop(AF_INET,&cliaddr.sin_addr,str,INET_ADDRSTRLEN));
                // check fd of the socket to determind the client
                if(sockfd == client3)
                {
                    read(sockfd,buff,1024);
                    //ch=ch+1;
                    //write(client_sockfd,&ch,1);
                    
                    file = fopen("data.txt","a");
                    fprintf(file,"%s\n",buff);
                    fclose(file);
                    printf("Wrote %s from fd %d\n",buff,sockfd);
        
                }
                else if(sockfd == client4)
                {
                    read(sockfd,buffer,1024);
                    //ch=ch+1;
                    //write(client_sockfd,&ch,1);
                    
                    file2 = fopen("data2.txt","a");
                    fprintf(file2,"%s\n",buffer);
                    fclose(file2);
                    printf("Wrote %s from fd %d\n",buffer,sockfd);
                }
                
            }
            
        }
    }
    return 0;
}

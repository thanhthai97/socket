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
#define SIZE_MESSAGE 16

int n_data, client3, client4;
char clientname[SIZE_MESSAGE];
char buff[MAX_BUFFER], buffer[MAX_BUFFER], str[INET_ADDRSTRLEN];
int connfd, sockfd; // varibale for file disriptor
struct sockaddr_in servaddr;
int listenfd;
FILE *filedata;
FILE *filerand;

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

void create_file()
{
    filedata = fopen("data_sum.txt","w");
    if(filedata == NULL)
    {
        printf("Open file Error !!!");
        exit(1);
    }
    fclose(filedata);
    filerand = fopen("data_random.txt","w");
    if(filerand == NULL)
    {
        printf("Open file Error !!!");
        exit(1);
    }
    fclose(filerand);
}

void write_on_file_data()
{
                        
    read(sockfd,buff,MAX_BUFFER);
    
    filedata = fopen("data_sum.txt","a");
    fprintf(filedata,"%s\n",buff);
    fclose(filedata);
    printf("Wrote %s from fd %d\n",buff,sockfd);
                    
}
void write_on_file_random()
{
                        
    read(sockfd,buffer,MAX_BUFFER);
    
    filerand = fopen("data_random.txt","a");
    fprintf(filerand,"%s\n",buffer);
    fclose(filerand);
    printf("Wrote %s from fd %d\n",buffer,sockfd);
                    
}

void init_socket_server()
{
    listenfd = socket(AF_INET,SOCK_STREAM, 0); // create socket with stype sock stream
    memset(&servaddr,0,sizeof(servaddr));

    servaddr.sin_family=AF_INET; // socket stype AF_NET
    servaddr.sin_addr.s_addr = INADDR_ANY; //address for socket server apply any address of client
    servaddr.sin_port = htons(SERV_PORT); // port of socket server is 8000
    // bind socket server to server address
    bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

    listen(listenfd,20); // always listen
}

int main()
{
    std::cout<<"PID process server running: "<<(int)getpid()<<std::endl;
    
    ssize_t nready, efd, retepoll, sfd;
    
    socklen_t clilen; // lenght of socket client
    //int client[MAX_CLIENT];
    struct sockaddr_in cliaddr; // struct for socket server and client

    struct epoll_event tep, ep[MAX_CLIENT]; //struct list event of epoll
    // Initial SOCKET SERVER--------------------------------------------------------------------
    init_socket_server();
    //----INITIAL EPOLL------------------------------------------------------------------------
    efd = epoll_create(MAX_CLIENT); //create epoll with 65525 element
    sfd = setup_signalfd();

    tep.events = EPOLLIN; tep.data.fd = listenfd; // create epoll event and add event of socket server
    retepoll = epoll_ctl(efd,EPOLL_CTL_ADD,listenfd,&tep); // add event listen of socket server to epoll list
    tep.data.fd = sfd; // add event from signal
    tep.events = EPOLLIN;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &tep) != 0)
    {
	    perror("epoll_ctl");
    }
    //---------------------------------------------------------------------------------------------
    // create file to write data from clients
    create_file();
    //----------------------------------------------
    while(true)
    {
        printf("server waiting ...\n");
        // epoll wait
        nready = epoll_wait(efd,ep,65525,-1);
        //check event
        for(int i =0;i<nready;i++)
        {
            // if there is a event happen if not exits on list
            if(ep[i].data.fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                // server accept the connect from client to server socket
                connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
                            
                printf("received from %s at PORT %d\n",inet_ntop(AF_INET,&cliaddr.sin_addr,str,INET_ADDRSTRLEN),ntohs(cliaddr.sin_port));
                // check the name of the client
                read(connfd,clientname,SIZE_MESSAGE);
                
                printf("- message: %s\n",clientname);
                if ((std::string)clientname == (std::string)"CLIENT_SEND")
                {
                    client3 = connfd;
                }
                else if((std::string)clientname == (std::string)"CLIENT_RANDOM")
                {
                    client4 = connfd;
                }
                
                printf("+ client3 =  %d client4 = %d\n",client3,client4);
                // add the new event to list
                tep.events = EPOLLIN; tep.data.fd = connfd; 
                retepoll = epoll_ctl(efd, EPOLL_CTL_ADD, connfd,&tep);

            }
	        else if(ep[i].data.fd == sfd)
            {
                read_sig(sfd);
            }
            else
            {
                sockfd = ep[i].data.fd;
                                
                //n = read(sockfd,buf,MAXLINE);
                ioctl(sockfd,FIONREAD,&n_data);
                // if don't have the data read from client server will erase the connect
                if(n_data==0)
                {
                    retepoll = epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
                    close(sockfd);
                    printf("client [%d] closed connection\n",sockfd);
                }
                else
                {
                // check fd of the socket to determine the client
                    if(sockfd == client3)
                    {
                        write_on_file_data();
                    }
                    else if(sockfd == client4)
                    {
                        write_on_file_random();
                    }
                }
                
            }
            
        }
    }
    return 0;
}

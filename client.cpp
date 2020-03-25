#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<random>
#include<sys/time.h>
#include<pthread.h>
#include<arpa/inet.h>

#define MAX_BUFF 1024
#define PORT 8000

// Void create function itoa for linux----------------------------------------------
void reverse(char str[], int length) 
{ 
    int start = 0; 
    int end = length -1; 
    while (start < end) 
    { 
        std::swap(*(str+start), *(str+end)); 
        start++; 
        end--; 
    } 
}  

char* itoa(int num, char* str, int base) 
{ 
    int i = 0; 
    bool isNegative = false; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return str; 
    } 
  
    // In standard itoa(), negative numbers are handled only with  
    // base 10. Otherwise numbers are considered unsigned. 
    if (num < 0 && base == 10) 
    { 
        isNegative = true; 
        num = -num; 
    } 
  
    // Process individual digits 
    while (num != 0) 
    { 
        int rem = num % base; 
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
        num = num/base; 
    } 
  
    // If number is negative, append '-' 
    if (isNegative) 
        str[i++] = '-'; 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse(str, i); 
  
    return str; 
} 
//-------------------------------------------------------------------------
int data[2]={0,0};
pthread_mutex_t sum_lock; // struct for lock the data of the process
int sum=0;

// create void thread random from 1 t0 10 after 1 second is pointer
void *thread_random10(void *arg)
{
    
    while(true){
        pthread_mutex_lock(&sum_lock);
        sum += (rand()%10 +1);
        std::cout<<"Current sum = "<<sum<<std::endl;
       
        pthread_mutex_unlock(&sum_lock);
        sleep(1); 
   }
}
    
    
// create void thread calculate sum of random value after 1 second is pointer
void *thread_calculated_sum(void *arg)
{
    
    while(true){
    
    pthread_mutex_lock(&sum_lock);   // lock the data by mutex
    data[0]=data[1];
    data[1]=sum;
    std::cout<<"Average after 5  second = "<<(double)(data[1]-data[0])/5<<std::endl;
    pthread_mutex_unlock(&sum_lock); // unlock data
    sleep(5);
    }
    
}
// function for thread send data to server after 5 seconds is pointer
void *thread_share_data(void *arg)
{
    
    
    int sockfd;
    int lensock;
    struct sockaddr_in address;
    int result;
    
    // tao socket cho trinh khach. luu lai so mo ta socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    address.sin_family=AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");// address of client is 127.0.0.1
    address.sin_port = htons(PORT); // port connect is 8000
   
  
    lensock =sizeof(address);
    // request the connect to server
    result = connect(sockfd,(struct sockaddr *)&address,lensock);
    if(result == -1)
    {
        perror("oops: client share data problem");
        exit(EXIT_FAILURE);
    }
    // receive and send data to server after connect
    char buff[MAX_BUFF];
    while(true){
        
        pthread_mutex_lock(&sum_lock); // lock the data to use
        itoa(sum,buff,10);
        write(sockfd, buff,sizeof(buff));
        
        printf("thread share data send to server %d\n",sum);
        
        pthread_mutex_unlock(&sum_lock); // unlock data
        sleep(5);
    }
    close(sockfd); //close client
}
// create thread random from 1 to 100 and send data to server after 1 second is pointer of function
void *thread_random100(void *arg)
{
    
    //pthread_mutex_lock(&sum_lock);
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    

    // tao socket cho trinh khach. luu lai so mo ta socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    address.sin_family=AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.81.12"); // address IP of client
    address.sin_port = htons(PORT); // port connect to server is 8000
    // gan ten socket tren may chu can ket noi
    
    len = sizeof(address);
    // request the connect to server
    result = connect(sockfd,(struct sockaddr *)&address,len);
    if(result == -1)
    {
        perror("oops: client random 100 problem");
        exit(EXIT_FAILURE);
    }
    // after connect, read/write data to server
    char buffer[MAX_BUFF];
    // always send data 
    while(true){
        
        int random100 = rand()%100 +1;
        
        itoa(random100,buffer,10);
        write(sockfd, buffer,sizeof(buffer));
        
        printf("thread random 100 send data server %d\n",random100);
        
        //pthread_mutex_unlock(&sum_lock);
        sleep(1);
    }
    //close the client
    close(sockfd);
}

int main()
{
   std::cout<<"PID process client running: "<<(int)getpid()<<std::endl;
   // 
   pthread_t manager_thread_random10;
   pthread_t manager_thread_calculate;
   pthread_t manager_thread_sharedata;
   pthread_t manager_thread_random100;
   void *thread_retval;
   int thread_check;
   // init the mutex
   thread_check=pthread_mutex_init(&sum_lock,NULL);
   if(thread_check!=0)
   {
       perror("Thread mutex created error");
       exit(EXIT_FAILURE);
   }
   // create and call thread random from 1 to 100 and send data after 1 second
   thread_check=pthread_create(&manager_thread_random100,NULL,thread_random100,NULL);
   if(thread_check!=0)
   {
       perror("Thread 4 created error");
       exit(EXIT_FAILURE);
   }
   // create  and call thread share data to server after 5 seconds
   thread_check=pthread_create(&manager_thread_sharedata,NULL,thread_share_data,NULL);
   if(thread_check!=0)
   {
       perror("Thread 3 created error");
       exit(EXIT_FAILURE);
   }
   // create and call thread calculated sum of random 10 after 1 second
   thread_check=pthread_create(&manager_thread_calculate,NULL,thread_calculated_sum,NULL);
   if(thread_check!=0)
   {
       perror("Thread 2 created error");
       exit(EXIT_FAILURE);
   }
   //create and call thread random from 1 t0 10 after 1 second
   thread_check=pthread_create(&manager_thread_random10,NULL,thread_random10,NULL);
   if(thread_check!=0)
   {
       perror("Thread 1 created error");
       exit(EXIT_FAILURE);
   }
   
   // destroy threads was created
   pthread_join(manager_thread_random100,&thread_retval);
   pthread_join(manager_thread_sharedata,&thread_retval);
   pthread_join(manager_thread_calculate,&thread_retval);
   pthread_join(manager_thread_random10,&thread_retval);
   // destroy the mutex locked data
   pthread_mutex_destroy(&sum_lock);

}
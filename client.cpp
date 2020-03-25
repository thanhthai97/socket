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
#define SIZE_MESSAGE 16

// define function itoa --------------------------------------------------------
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
//----------------------------------------------------------------------
int data[2]={0,0};
pthread_mutex_t sum_lock; // struct for lock the data of the process
int sum=0;

// create main random from 1 to 10 after 1 second
void *thread_random10(void *arg)
{
    
    while(true)
    {
        pthread_mutex_lock(&sum_lock);
        sum += (rand()%10 +1);
        std::cout<<"Current sum = "<<sum<<std::endl;
       
        pthread_mutex_unlock(&sum_lock);
        sleep(1); 
   }
   return 0;
}
    
    
// create main thread calculate sum the random value after 5 seconds
void *thread_calculated(void *arg)
{
    while(true)
    {
    
        pthread_mutex_lock(&sum_lock);   // lock the data by mutex
        data[0]=data[1];
        data[1]=sum;
        std::cout<<"Average = "<<(double)(data[1]-data[0])/5<<std::endl;
        pthread_mutex_unlock(&sum_lock); // unlock data
        sleep(5);
    }
    return 0;
}
// create main send the sum to server after 5 seconds
void *thread_send_data(void *arg)
{
    
    
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    
    while(true)
    {
        // create socket for client
        sockfd=socket(AF_INET,SOCK_STREAM,0);
        address.sin_family=AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");// address of client is localhost
        address.sin_port = htons(PORT); // port connect is 8000
        
        len =sizeof(address);
        // request the connect to server
        result = connect(sockfd,(struct sockaddr *)&address,len);
        if(result == -1)
        {
            perror("oops: client send the sum value problem");
            exit(EXIT_FAILURE);
        }
        write(sockfd,"CLIENT_SEND",SIZE_MESSAGE);
        // receive and send data to server after connect
        char buff[MAX_BUFF];
        
        pthread_mutex_lock(&sum_lock); // lock the data to use
        itoa(sum,buff,10);
        write(sockfd, buff,sizeof(buff));
        
        printf("thread send the sum value write on server %d\n",sum);
        
        pthread_mutex_unlock(&sum_lock); // unlock data
        close(sockfd); //close client
        sleep(5);
    }
    
    return 0;
}
// create main random from 1 to 100 after 1 second
void *thread_random100(void *arg)
{
    
    //pthread_mutex_lock(&sum_lock);
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;
    
    while(true)
    {
        // create socket for client
        sockfd=socket(AF_INET,SOCK_STREAM,0);
        address.sin_family=AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1"); // address of client is 192.168.1.10
        address.sin_port = htons(PORT); // port connect to server is 8000
                
        len = sizeof(address);
        // request the connect to server
        result = connect(sockfd,(struct sockaddr *)&address,len);
        if(result == -1)
        {
            perror("oops: client random 100 problem");
            exit(EXIT_FAILURE);
        }
        write(sockfd,"CLIENT_RANDOM",SIZE_MESSAGE); // send the name of client
        // after connect, read/write data to server
        char buffer[MAX_BUFF];
        // always send data 
        
        int ran100 = rand()%100 +1;
        
        itoa(ran100,buffer,10);
        write(sockfd, buffer,sizeof(buffer));
        
        printf("thread random 100 write on server %d\n",ran100);
        
        //pthread_mutex_unlock(&sum_lock);
        close(sockfd); //close the client
        sleep(1);
    }
    return 0;
}

int main()
{
   std::cout<<"PID process client running: "<<(int)getpid()<<std::endl;
    
   pthread_t manager_thread_rand10;
   pthread_t manager_thread_calculated;
   pthread_t manager_thread_send;
   pthread_t manager_thread_rand100;
   void *thread_retval;
   int thread_check;
   // init the mutex
   thread_check=pthread_mutex_init(&sum_lock,NULL);
   if(thread_check!=0)
   {
       perror("Thread mutex created error");
       exit(EXIT_FAILURE);
   }
   // create and call thread random 100
   thread_check=pthread_create(&manager_thread_rand100,NULL,thread_random100,NULL);
   if(thread_check!=0)
   {
       perror("Thread random 100 created error");
       exit(EXIT_FAILURE);
   }
   // create  and call thread send the sum value
   thread_check=pthread_create(&manager_thread_send,NULL,thread_send_data,NULL);
   if(thread_check!=0)
   {
       perror("Thread send the sum value created error");
       exit(EXIT_FAILURE);
   }
   // create and call thread 2
   thread_check=pthread_create(&manager_thread_calculated,NULL,thread_calculated,NULL);
   if(thread_check!=0)
   {
       perror("Thread calculated sum value created error");
       exit(EXIT_FAILURE);
   }
   //create and call thread 1
   thread_check=pthread_create(&manager_thread_rand10,NULL,thread_random10,NULL);
   if(thread_check!=0)
   {
       perror("Thread 1 created error");
       exit(EXIT_FAILURE);
   }
   
   // destroy threads was created
   pthread_join(manager_thread_rand100,&thread_retval);
   pthread_join(manager_thread_send,&thread_retval);
   pthread_join(manager_thread_calculated,&thread_retval);
   pthread_join(manager_thread_rand10,&thread_retval);
   // destroy the mutex locked data
   pthread_mutex_destroy(&sum_lock);
    
}
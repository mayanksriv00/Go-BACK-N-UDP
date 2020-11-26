// Server of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <pthread.h>

  
#define PORT     9090 
#define MAXLINE 1024 
#define MAXPACK 1034
#define Qlen    20

typedef struct {
    char buffer[MAXPACK];
    socklen_t len;
    struct sockaddr_in clntaddr;
} Query;

typedef struct {
    int process_id;
    char type;
    int seq_no;
    int ack_no;
    char data[MAXLINE];
} Packet;

/* functions prototype*/
void * server_thd(void * client);       /* a function to handle communication with clients */
//pthread_mutex_t  mut; /* Mutex to prevent multiple write. */
int size =  0;
char *list = "Hello from server.txt";
//int thread_no = 0;
int sockfd;   
struct sockaddr_in servaddr; 
void * pack_packet(int id, char type, int seq, int ack, char* buffer);
void * packet_to_string(void* pack, char* packstr);
void * string_to_packet(void* pack, char* raw_string);

int main() { 

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    //pthread_t thrd_id;
    //pthread_mutex_init(&mut, NULL); 
 
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(PORT); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
   
    while(1){
	Query *client = (Query*)malloc(sizeof(Query)); 
        memset(client, 0, sizeof(Query));
	client->len = sizeof(client->clntaddr);

        int n = recvfrom(sockfd, client->buffer, MAXLINE,  
                0, ( struct sockaddr *) &client->clntaddr, 
                &client->len); 
	if(n < 0){
	    perror("receive failed"); 
            exit(EXIT_FAILURE);
	}
        printf("message received.\n");

	//int td = pthread_create(&thrd_id, NULL, server_thd, (void *)client);
	server_thd((void *)client);
         
    }
    return 0; 
} 


void* server_thd(void * clnt){
    //printf("New thread created.\n");

    Query *client = (Query*)clnt;
    Packet* recv_pack = (Packet*)malloc(sizeof(Packet));
    string_to_packet((void*) recv_pack, (char*) client->buffer);
    printf("Client : %s\n", recv_pack->data);
    if(recv_pack->type == 'D'){
        if(strcmp(recv_pack->data, "list") == 0){
	    sendto(sockfd, (const char *)list, strlen(list),  
                MSG_CONFIRM, (const struct sockaddr *) 
	        &client->clntaddr, client->len); 
            printf("List message sent.\n");
        } else {
	    char arg1[MAXLINE], arg2[MAXLINE];
	    int n = sscanf(recv_pack->data, "%s %s", arg1, arg2);
	    if(strcmp(arg1, "get") == 0){

	    }
	}

    } 
    //pthread_exit(0);
}

void * pack_packet(int id, char type, int seq, int ack, char* buffer){

    Packet* pack = (Packet*)malloc(sizeof(Packet));
    memset(pack, 0, sizeof(Packet));
    
    pack->process_id = id;
    pack->type = type;
    pack->seq_no = seq;
    pack->ack_no = ack;
    strcpy(pack->data, buffer);
    
    return ((void*) pack);
}


void* packet_to_string(void * pack, char* packstr){
    Packet* cur = (Packet* )pack;

    char type_str[2], id_str[5], seq_str[5], ack_str[5];
	sprintf(id_str, "%d", cur->process_id);
	sprintf(type_str, "%c", cur->type);
	sprintf(seq_str, "%d", cur->seq_no);
	sprintf(ack_str, "%d", cur->ack_no);

	strcat(packstr, id_str);
	strcat(packstr, ";");
	strcat(packstr, type_str);
	strcat(packstr, ";");
	strcat(packstr, seq_str);
	strcat(packstr, ";");
	strcat(packstr, ack_str);
	strcat(packstr, ";");
	strcat(packstr, cur->data);
	//printf("Packet: %s\n", packstr);

    //return (char* )packstr;
}

void * string_to_packet(void* pack, char* raw_string){
    Packet* cur = (Packet* )pack;
    //printf("%s\n", raw_string);
    int n = sscanf(raw_string, "%d;%c;%d;%d;%s", &cur->process_id, &cur->type, &cur->seq_no, &cur->ack_no, &cur->data);

    //printf("data: %s\n", cur->data);
    //return ((void*) pack);

}
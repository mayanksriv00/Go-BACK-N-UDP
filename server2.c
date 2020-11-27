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

typedef struct Process{
    int process_id;
    int process_type; //1 for list, 2 for get
    int window[5];
    char * cur_pack1;
    char * cur_pack2;
    char * cur_pack3;
    char * cur_pack4;
    char * cur_pack5;
    int acked_seq;
    int expected_total_seq;
    struct Process * next;
} Process;

/* functions prototype*/
void * server_thd(void * client);       /* a function to handle communication with clients */
//pthread_mutex_t  mut; /* Mutex to prevent multiple write. */
int process_newest =  0;
Process* process_list = NULL;
char *list = "Hello from server.txt";
//int thread_no = 0;
int sockfd;   
struct sockaddr_in servaddr; 
void * pack_packet(int id, char type, int seq, int ack, char* buffer);
void * packet_to_string(void* pack, char* packstr);
void * string_to_packet(void* pack, char* raw_string);
void * ls(void* recv_pack, char* list_str, void* process);
void free_process(int process_id);
void * new_process();
void* search_process(int process_id);

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
	client = NULL;
         
    }
    return 0; 
} 


void* server_thd(void * clnt){
    //printf("New thread created.\n");

    Query *client = (Query*)clnt;
    Packet* recv_pack = (Packet*)malloc(sizeof(Packet));
    //memset(recv_pack, 0, sizeof(recv_pack));
    string_to_packet((void*) recv_pack, (char*) client->buffer);
    printf("Client : %s\n", recv_pack->data);
    if(recv_pack->type == 'D'){
	Process* cur = (Process* )new_process();
        if(strcmp(recv_pack->data, "list") == 0){
	    char list_str[MAXPACK];
	    memset(&list_str, 0, sizeof(list_str)); 
	    ls((void *)recv_pack, (char *)list_str, (void *)cur);
	    sendto(sockfd, (const char *)list_str, MAXPACK,  
                MSG_CONFIRM, (const struct sockaddr *) 
	        &client->clntaddr, client->len); 
            printf("List message sent.\n");
        } else {
	    char arg1[MAXLINE], arg2[MAXLINE];
	    int n = sscanf(recv_pack->data, "%s %s", arg1, arg2);
	    //if(strcmp(arg1, "get") == 0){

	    //}
	}

    } else if(recv_pack->type == 'A') {
	Process* cur = (Process*) search_process(recv_pack->process_id);
	if(cur == NULL){
	    //error, no such process on going
	    printf("Error, no such process.\n");
	    
	} else {
	    if(cur->process_type == 1 && recv_pack->seq_no == cur -> expected_total_seq){
		printf("ACK for the list packet received.\n");
		printf("Now delete the %d process.\n", recv_pack->process_id);
		free_process(recv_pack->process_id);
	    }
	}
    }

    free(client);
    free(recv_pack);
    client = NULL;
    recv_pack = NULL;
    //pthread_exit(0);
}

void * ls(void* recv_pack, char* list_str, void* process){
    Packet * recv = (Packet*) recv_pack;
    Process * cur_process = (Process *) process;
    cur_process->process_type = 1;
    cur_process->acked_seq = 0;
    cur_process->expected_total_seq = 1;
    Packet * send = pack_packet(cur_process->process_id, 'L', 1, 0, list); 
    packet_to_string((void* )send, list_str);
    cur_process->cur_pack1 = list_str;
    printf("new process %d, the %dth packet, content: %s\n", cur_process->process_id, send->seq_no, cur_process->cur_pack1);
}

void * new_process(){
    Process* cur;
    if(process_list == NULL){
	process_list = (Process*)malloc(sizeof(Process));
	process_list->process_id = 1;
    	process_list->cur_pack1 = NULL;
    	process_list->cur_pack2 = NULL;
    	process_list->cur_pack3 = NULL;
    	process_list->cur_pack4 = NULL;
    	process_list->cur_pack5 = NULL;
    	process_list->next = NULL;
	cur = process_list;
    } else {
    	cur = process_list;
	while(cur->next != NULL){
	    cur = cur->next;
	}
	cur->next = (Process*)malloc(sizeof(Process));
	cur->next->process_id = cur->process_id + 1;
    	cur->next->cur_pack1 = NULL;
    	cur->next->cur_pack2 = NULL;
    	cur->next->cur_pack3 = NULL;
    	cur->next->cur_pack4 = NULL;
    	cur->next->cur_pack5 = NULL;
    	cur->next->next = NULL;
	cur = cur->next;
    }
    return (void*) cur;
}

void* search_process(int process_id){
    Process* cur = process_list;
    while(cur->process_id != process_id){
	cur = cur->next;
	if(cur == NULL) {
	    return (void*) cur;
	}
    }
    return (void*) cur;
}

void free_process(int process_id){
    if(process_list->process_id == process_id){
	if(process_list->next == NULL){
	    free(process_list);
	    process_list = NULL;
	} else {
	    Process* tmp = process_list;
	    process_list = tmp->next;
	    free(tmp);
	    //tmp = NULL;
	}
	return;
    }
    Process* pre = process_list;
    Process* cur = pre->next;
    while(cur->process_id != process_id){
	pre = pre->next;
	cur = cur->next;	
    }
    pre->next = cur->next;
    free(cur);
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

	free(cur);
	pack = NULL;
    //return (char* )packstr;
}

void * string_to_packet(void* pack, char* raw_string){
    Packet* cur = (Packet* )pack;
    //printf("%s\n", raw_string);
    int n = sscanf(raw_string, "%d;%c;%d;%d;%s", &cur->process_id, &cur->type, &cur->seq_no, &cur->ack_no, &cur->data);

    //printf("data: %s\n", cur->data);
    //return ((void*) pack);

}
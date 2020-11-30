// Server of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
//#include <signal.h>
//#include <errno.h>
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
  
#define PORT     9090 
#define MAXLINE 1024 
#define MAXPACK 1034
#define Qlen    20
#define TIMEOUT_SECS 4

typedef struct {
    char buffer[MAXPACK];
    socklen_t len;
    struct sockaddr_in clntaddr;
    int clnt_id;
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
    int seq_base;
    int expected_total_seq;
    struct Process * next;
} Process;

/* functions prototype*/
     /* a function to handle communication with clients */
//pthread_mutex_t  mut; /* Mutex to prevent multiple write. */
int process_newest =  1;
int client_id = 1;
Process* process_list = NULL;
char *list = "Hello from server.txt";
//int thread_no = 0;
int sockfd;   
struct sockaddr_in servaddr; 
pthread_mutex_t mut;

void * server_thd(void * client);  
void * pack_packet(int id, char type, int seq, int ack, char* buffer);
void * packet_to_string(void* pack, char* packstr);
void * string_to_packet(void* pack, char* raw_string);
void * ls(void* recv_pack, char* list_str, void* process);
void free_process(int process_id);
void * new_process();
void* search_process(int process_id);
void timeout(void* process, void* clnt);

int main() { 

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    pthread_t thrd_id;

    pthread_mutex_init(&mut, NULL);
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

    //Query *client = (Query*)malloc(sizeof(Query));  
   
    while(1){
	Query *client = (Query*)malloc(sizeof(Query)); 
    memset(client, 0, sizeof(Query));
	client->len = sizeof(client->clntaddr);
    client->clnt_id = client_id;

    int n = recvfrom(sockfd, (char*)client->buffer, MAXLINE,  
                0, ( struct sockaddr *) &client->clntaddr, 
                &client->len); 
	if(n < 0){
	    perror("receive failed"); 
        exit(EXIT_FAILURE);
	}
    printf("message received.\n");
    printf("%s\n", client->buffer);
    client->buffer[n] = '\0';

	//int td = pthread_create(&thrd_id, NULL, server_thd, (void *)client);
	pthread_create(&thrd_id, NULL, server_thd, (void *)client);
    client_id++;
	client = NULL;
         
    }

    close(sockfd);
    return 0; 
} 


void* server_thd(void * clnt){
    printf("New thread created.\n");

    //run in isolate threads

    Query *client = (Query*)clnt;
    printf("current client_id = %d\n", client->clnt_id);
    printf("%s\n", client->buffer);
    Packet* recv_pack = (Packet*)malloc(sizeof(Packet));
    memset(recv_pack, 0, sizeof(recv_pack));
    string_to_packet((void*) recv_pack,(char*) client->buffer);
    printf("Client : %s\n", recv_pack->data);
    if(recv_pack->type == 'D'){
        pthread_mutex_lock(&mut);
	    Process* cur = (Process* )new_process();
        pthread_mutex_unlock(&mut);
        if(strcmp(recv_pack->data, "list") == 0){
	        char list_str[MAXPACK];
	        memset(&list_str, 0, sizeof(list_str)); 
            pthread_mutex_lock(&mut);
	        ls((void *)recv_pack, (char *)list_str, (void *)cur);
            pthread_mutex_unlock(&mut);
	        sendto(sockfd, (const char *)list_str, MAXPACK, MSG_CONFIRM, (const struct sockaddr *) &client->clntaddr, client->len); 
            printf("List message sent.\n");
            cur->seq_base ++;
                  
            /*Query *client1 = (Query*)malloc(sizeof(Query)); 
            memset(client1, 0, sizeof(Query));
	        client1->len = sizeof(client1->clntaddr);
            int n;*/
            struct timeval tv;
            int cur_time = tv.tv_sec;
            while(cur->seq_base > cur->acked_seq){
                gettimeofday(&tv, NULL);
                if(tv.tv_sec > cur_time + TIMEOUT_SECS){
                    printf("time out!\n");
                    sendto(sockfd, (const char *)list_str, MAXPACK, MSG_CONFIRM, (const struct sockaddr *) &client->clntaddr, client->len); 
                    cur_time = tv.tv_sec;
                }
            }
	    }

    } else if(recv_pack->type == 'A') {
        printf("ACK\n");
	    Process* cur = (Process*) search_process(recv_pack->process_id);
	    if(cur == NULL){
	        //error, no such process on going
	        printf("Error, no such process.\n");
	    
	    } else {
	        if(cur->process_type == 1 && recv_pack->seq_no == cur -> expected_total_seq){
                pthread_mutex_lock(&mut);
                cur->acked_seq ++;
                pthread_mutex_unlock(&mut);
		        printf("ACK for the list packet received.\n");
		        printf("Now delete the %d process.\n", recv_pack->process_id);
                pthread_mutex_lock(&mut);
	    	    free_process(recv_pack->process_id);
                pthread_mutex_unlock(&mut);
	        } //else if()
	    }
    }

    free(client);
    free(recv_pack);
    client = NULL;
    recv_pack = NULL;
    printf("end process\n");
    pthread_exit(0);
}

void * ls(void* recv_pack, char* list_str, void* process){
    Packet * recv = (Packet*) recv_pack;
    Process * cur_process = (Process *) process;
    cur_process->process_type = 1;
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
	process_list->process_id = process_newest;
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
	cur->next->process_id = process_newest;
    	cur->next->cur_pack1 = NULL;
    	cur->next->cur_pack2 = NULL;
    	cur->next->cur_pack3 = NULL;
    	cur->next->cur_pack4 = NULL;
    	cur->next->cur_pack5 = NULL;
    	cur->next->next = NULL;
	cur = cur->next;
    }
    cur->acked_seq = 0;
    cur->seq_base = 0;
    process_newest ++;
    return (void*) cur;
}

void* search_process(int process_id){
    Process* cur = process_list;
    if(cur == NULL) {
	    return NULL;
	}
    while(cur->process_id != process_id){
	cur = cur->next;
	if(cur == NULL) {
	    return NULL;
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
    
    //int n = sscanf(raw_string, "%d;%c;%d;%d;%s", &cur->process_id, &cur->type, &cur->seq_no, &cur->ack_no, &cur->data);
    int field = 0, counter = 0, copy = 0;
    char temp;
    char id_str[5], seq_str[5], ack_str[5];
   //printf("%s\n", raw_string);
    while(raw_string[counter] != '\0'){
        
        temp = raw_string[counter];
        if(raw_string[counter] == ';' && field < 4){
            field++;
            counter++;
            copy = 0;
            continue;
        }
        if(field == 0) {
            id_str[copy] = raw_string[counter];
        }
        if(field == 1) {
            cur->type = raw_string[counter];
        }
        if(field == 2) {
            seq_str[copy] = raw_string[counter];
        }
        if(field == 3) {
            ack_str[copy] = raw_string[counter];
        }
        if(field == 4) {
            cur->data[copy] = raw_string[counter];
            //printf("%c", cur->data[copy]);
        }
        counter++;
        copy++;
    }
    cur->process_id = (int)atof(id_str);
    cur->seq_no = (int)atof(seq_str);
    cur->ack_no = (int)atof(ack_str);
    cur->data[copy] = '\0';

    //printf("data: %s\n", cur->data);
    //printf("here\n");
    //return ((void*) pack);

}

// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define PORT     9090 
#define MAXLINE 1024 
#define MAXPACK 1044

typedef struct {
    int process_id;
    char type;
    int seq_no;
    int ack_no;
    char data[MAXLINE];
} Packet;
  
void * serverConn();
void print_guide();
void * pack_packet(int id, char type, int seq, int ack, char* buffer);
void * packet_to_string(void* pack, char* packstr);
void * string_to_packet(void* pack, char* raw_string);

int sockfd;
struct sockaddr_in servaddr;
int passed_process = 0;

// Driver code 

int main() { 

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 

    serverConn();
    close(sockfd); 
    return 0; 
}

void print_guide(){
    /* Print use instructions */
    printf("**************************************************************************************\n"
           "use one of the following commands: \n"
           "----------------------------------\n"
           "list \t\t  %% list all files offered \n"
           "get file_name \t  %% download the claimed file\n"
           "leave \t\t  %% quite\n"
           "**************************************************************************************\n");
} 

void * serverConn(){
    int n, len; 
    print_guide();
    char buffer[MAXLINE + 1];

    while(1){
	    fgets(buffer, sizeof(buffer), stdin);
	    buffer[strlen(buffer)-1] = '\0';
	    char arg1[MAXLINE], arg2[MAXLINE];
	    int n = sscanf(buffer, "%s %s", arg1, arg2);

	    if(strcmp(buffer, "list") == 0){
	        char packstring[MAXPACK];
	        memset(&packstring, 0, sizeof(packstring)); 
	        packet_to_string(pack_packet(0,'D', 0, 1, (char*)buffer), (char* )packstring);
	        printf("%s\n", packstring);
            sendto(sockfd, (const char *)packstring, MAXPACK, 
                MSG_CONFIRM, (const struct sockaddr *) 			&servaddr, sizeof(servaddr)); 
            printf("List command sent.\n"); 
          
	        /*printf("%s\n", packstring);
	        Packet* recv_pack = (Packet*)malloc(sizeof(Packet));
	        string_to_packet((void*) recv_pack, 			(char*)packstring);
	        printf("content: %s\n", recv_pack->data);
	        */
            len = sizeof(servaddr);
	        char recvbuffer[MAXLINE];
            memset(&recvbuffer, 0, sizeof(recvbuffer));

            while(1){
                n = recvfrom(sockfd, (char *)recvbuffer, MAXLINE,  0, (struct sockaddr *)&servaddr, &len);
                printf("message:%s\n", recvbuffer);
                srand((unsigned)time(NULL));
                int a = rand() %10;
                printf("random receive: %d\n", a);
                if(a > 1){ //20%lost percentage
                //printf("message received \n");
                    recvbuffer[n] = '\0'; 
	                Packet* recv_pack = (Packet*)malloc(sizeof(Packet));
	                //memset(recv_pack, 0, sizeof(recv_pack));
    	            string_to_packet((void*) recv_pack, (char*) recvbuffer);
    	            if(recv_pack->process_id > passed_process && recv_pack->type == 'L' && recv_pack->seq_no == 1 && recv_pack->ack_no == 0){
                        printf("List of files : %s\n", recv_pack->data);
        	            char ackstring[MAXPACK];
	                    memset(&ackstring, 0, sizeof(ackstring));
        		        char* ackbuffer = "get list";
	                    packet_to_string(pack_packet(recv_pack->process_id, 'A', recv_pack->seq_no, recv_pack->ack_no,  (char*)ackbuffer), (char* )ackstring);
	                    sendto(sockfd, (const char *)ackstring, MAXPACK, MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
                        passed_process = recv_pack->process_id ;
                    break;
                    }
                }
	            memset(&recvbuffer, 0, sizeof(recvbuffer));
	            //else timeout! resend request!
            }
            printf("list command end\n");
	    } //else if(strcmp(buffer, "") == 0)
    }
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
    //int n = sscanf(raw_string, "%d;%c;%d;%d;%s", &cur->process_id, &cur->type, &cur->seq_no, &cur->ack_no, &cur->data);

    int field = 0, counter = 0, copy = 0;
    char temp;
    char id_str[5], seq_str[5], ack_str[5];
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
        }
        counter++;
        copy++;

    }
    cur->process_id = (int)atof(id_str);
    cur->seq_no = (int)atof(seq_str);
    cur->ack_no = (int)atof(ack_str);
    //printf("data: %s\n", cur->data);
    //return ((void*) pack);

}
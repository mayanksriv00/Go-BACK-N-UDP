//Server program

//headers
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<iostream>
using namespace std;
struct UDP_Packet{              //Packet receiving form the client
    int packet_type;
    int sequence_number;
    int packet_size;
    char buffer[512];
};

struct UDP_ACK_Packet{          //ACK packet
    int packet_type;
    int ack_no;
};

int main(int argc,char *argv[]){
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int socke;
    unsigned int clinet_address_length;
    unsigned short server_port_no;
    int message_size;
    int chunk_size;
    float loss_error_percentage;

    char buffer[10000];     //data buffer
    int sequence_number=0;
    int bss=0;          //base

    if(argc<3 || argc<4)
    {
        perror("Follow the format [ ./filename <Port_NO[UDP]> <Chunk_size> <Loss_Error_percentage>");
        exit(1);
    }
    server_port_no=atoi(argv[1]);
    chunk_size=atoi(argv[2]);
    loss_error_percentage=(atof(argv[3]))/100.0;

    //cout<<server_port_no<<endl;
    //cout<<chunk_size<<endl;
    //cout<<loss_error_percentage<<endl;

    if((socke=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)            //creating socket
    {
        perror("Error creating socket");
        exit(1);
    }

    memset(&server_address,0,sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port_no);

    if(bind(socke,(struct sockaddr *)&server_address,sizeof(server_address))<0)     //Binding
    {
        perror("Error in binding");
        exit(1);
    }
    clinet_address_length=sizeof(client_address);

    char buffer_msg[1024];
    if((message_size=recvfrom(socke,(char *)buffer_msg,1024,MSG_WAITALL,(struct sockaddr *)&client_address,&clinet_address_length))<0)
    {
        perror("No msg received");
        exit(1);
    }
    cout<<"Message received"<<buffer_msg<<endl;
    return 0;
}


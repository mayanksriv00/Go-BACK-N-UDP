#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include<iostream>
using namespace std;

#define STRINGSIZE 511      //longest string to echo
#define TIMEOUT 3           //in sec
#define DATAMSGTYPE 1
#define LASTMSGTYPE 4
#define MAXNOTRIES 16

int tries=0;
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

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    //printf("In Alarm\n");
}

int main(int argc,char *argv[])
{
    int socke;
    struct sockaddr_in receiving_server_address;
    struct sockaddr_in server_address_from; //fromADDR
    unsigned short receiving_server_port;
    unsigned int size_from;
    struct sigaction signal_handle;
    int datagram_len;
    char *serverIP;
    int window_size=5;
    int chunk_size;     //try to maintain chuck size less than 512
    int trys=0;
    float loss_error_percentage;
    if(argc!=5)
    {
        perror("Plese use format to send [ ./filename <server_ip> <server_port> <chunk_size> <lost_error_percentage>");
        exit(1);
    }

    serverIP=argv[1];
    receiving_server_port=atoi(argv[2]);
    chunk_size=atoi(argv[3]);
    loss_error_percentage=(atof(argv[4]))/100.0;

    //creating socket
    if((socke=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)
    {
        perror("error creating socket");
        exit(1);
    }

    memset(&receiving_server_address,0,sizeof(receiving_server_address));
    receiving_server_address.sin_family=AF_INET;
    receiving_server_address.sin_addr.s_addr=inet_addr(serverIP);
    receiving_server_address.sin_port=htons(receiving_server_port);

    //handling signal
    signal_handle.sa_handler=CatchAlarm;
    if(sigemptyset(&signal_handle.sa_mask)<0)
    {
        perror("Sinal handler exit");
        exit(1);
    }
    signal_handle.sa_flags=0;
    //Press 1 to send ls and receive msg
    //press 2 to enter file mode
    //press 3 to exit
    int choice=0;
    cout<<"Press 1 to use list command"<<endl;
    cout<<"Press 2 to enter file sending mode"<<endl;
    cout<<"Press 3 to exit"<<endl;
    cin>>choice;
    while(choice!=3)
    {
        if(choice==1)
        {
            //sending the list message to the server
            char *msg="list";
            sendto(socke,(const char*)msg,strlen(msg),MSG_CONFIRM,(const struct sockaddr *)&receiving_server_address,sizeof(receiving_server_address));
            cout<<"Sending list to server"<<endl;
        }


        cout<<"Press 1 to use list command"<<endl;
        cout<<"Press 2 to enter file sending mode"<<endl;
        cout<<"Press 3 to exit"<<endl;
        cin>>choice;
    }
    close(socke);
    return 0;
}
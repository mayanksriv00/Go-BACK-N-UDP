#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include<iostream>
#include<fstream>
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
    char buffer[2048];
};

struct UDP_ACK_Packet{          //ACK packet
    int packet_type;
    int ack_no;
};

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    //printf("In Alarm\n");
}
int packet_loss(float rate)
{
    double t;
    t=drand48();
    if(t<rate)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
struct UDP_ACK_Packet ACK_createpacket(int ackType,int start_base)
{
    struct UDP_ACK_Packet ack;
    ack.packet_type=ackType;
    ack.ack_no=start_base;
    return ack;
}
int main(int argc,char *argv[])
{
    int socke;
    struct sockaddr_in receiving_server_address;
    struct sockaddr_in server_address_from; 
    unsigned short receiving_server_port;
    unsigned int size_from;
    struct sigaction signal_handle;
    int datagram_len;
    char *serverIP;
    int window_size=5;
    int chunk_size;     //try to maintain chuck size less than 512
    int trys=0;
    float loss_error_percentage;
    int receive_size;
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
            cout<<"Sending list command to server "<<endl;

            //Now client has to receive msg from the server
            //Response of the list command that was execited at the server side

            char buff[8192];
            int start_base=-2;
            int sequence_number=0;
            for(;;)
            {
                size_from=sizeof(server_address_from);
                struct UDP_Packet packet_data;
                struct UDP_ACK_Packet ack_k;
                if((receive_size=recvfrom(socke,&packet_data,sizeof(packet_data),0,(struct sockaddr *)&server_address_from,&size_from))<0)
                {
                    perror("recvfrom() error");
                    exit(1);
                }
                sequence_number=packet_data.sequence_number;
                if(!packet_loss(loss_error_percentage))
                {
                    if(packet_data.sequence_number==0 && packet_data.packet_type==1)
                    {
                        cout<<"Received Initial packet "<<inet_ntoa(server_address_from.sin_addr)<<endl;
                        //cout<<"Debug-0:"<<buff<<endl;
                        packet_data.buffer[chunk_size]='~';
                        packet_data.buffer[chunk_size+1]='~';
                        //cout<<"Debug:  "<<packet_data.buffer<<endl;
                        memset(buff,0,sizeof(buff));
                       // cout<<"Debug-1:"<<buff<<endl;
                        strcpy(buff,packet_data.buffer);
                        start_base=0;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    else if(packet_data.sequence_number==start_base+1)
                    {
                        cout<<"Received: Sequence No "<<packet_data.sequence_number;
                       // cout<<endl<<<<"Debug:"<<packet_data.buffer<<endl;
                        packet_data.buffer[chunk_size]='~';
                        packet_data.buffer[chunk_size+1]='~';
                        //cout<<"Debug:  "<<packet_data.buffer<<endl;
                        strcat(buff,packet_data.buffer);
                        start_base=packet_data.sequence_number;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    else if(packet_data.packet_type==1 && packet_data.sequence_number!=start_base+1)
                    {
                        cout<<"OUT OF SEQ: Packet received "<<packet_data.sequence_number<<endl;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    if(packet_data.packet_type==4 && sequence_number==start_base)
                    {
                        start_base=-1;
                        ack_k=ACK_createpacket(8,start_base);
                    }
                    if(start_base>=0)
                    {
                        cout<<"SENDING acknowledgement "<<start_base<<endl;
                        if(sendto(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&server_address_from,sizeof(server_address_from))!=sizeof(ack_k))
                        {
                            perror("sending bits different from expected - 1");
                            exit(1);
                        }
                    }
                    else if(start_base==-1){
                        cout<<"Recived LAST packet"<<endl;
                        cout<<"Sending last ACK"<<endl;
                        if(sendto(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&server_address_from,sizeof(server_address_from))!=sizeof(ack_k))
                        {
                            perror("sending bits different from expected - 2");
                            exit(1);
                        }
                    }

                    if(packet_data.packet_type==4 && start_base==-1){
                        char temp_storage[8192];
                        int x=0;
                        for(int i=0;i<strlen(buff);i++)
                        {
                            if(buff[i]!='~')
                            {
                                temp_storage[x]=buff[i];
                                x++;
                            }
                        }
                        temp_storage[x]='\0';
                        cout<<"Message received: "<<temp_storage<<endl;
                        memset(buff,0,sizeof(buff));
                        break;
                    }
                }
                else
                {
                    cout<<"lose: SIMULATED"<<endl;
                }
            }
        }
        if(choice==2)
        {
            char *msg="get";
            sendto(socke,(const char*)msg,strlen(msg),MSG_CONFIRM,(const struct sockaddr *)&receiving_server_address,sizeof(receiving_server_address));
            cout<<"Sending get command to server "<<endl;
            char buf4[1024];
            unsigned int len3;
            int n;
            n=recvfrom(socke,(char *)buf4,1024,MSG_WAITALL,(struct sockaddr *)&receiving_server_address,&len3);
            //printf("%s\n",buf4);
            cout<<"Enter the file name you want(get) from the server"<<endl;
            char *msg1="";
            char str3[1024];
            memset(str3,0,sizeof(str3));
            scanf("%s",str3);
            //printf("%s",str3);
            msg1=str3;
            sendto(socke,(const char*)msg1,strlen(msg1),MSG_CONFIRM,(const struct sockaddr *)&receiving_server_address,sizeof(receiving_server_address));
            //cout<<"NOw begin"<<endl;
            memset(buf4,0,sizeof(buf4));
            //n=recvfrom(socke,(char *)buf4,1024,MSG_WAITALL,(struct sockaddr *)&receiving_server_address,&len3);
            while((recvfrom(socke,(char *)buf4,1024,MSG_WAITALL,(struct sockaddr *)&receiving_server_address,&len3))>0)
            {
                //cout<<"DEBUG"<<buf4<<endl;
                if(strcmp(buf4,"RECEIVED")!=0)
                {
                   // cout<<"DEBUG: Inside iii"<<endl;
                    cout<<"Enter correct file name"<<endl;
                    scanf("%s",str3);
                    //memset(str3,0,sizeof(str3));
                    char *msg1="";
                    msg1=str3;
                    cout<<"CHECK"<<msg1<<endl;
                    sendto(socke,(const char*)msg1,strlen(msg1),MSG_CONFIRM,(const struct sockaddr *)&receiving_server_address,sizeof(receiving_server_address));
                    memset(buf4,0,sizeof(buf4));
                }
                else
                {
                    break;
                }
            }
            char buff[5000000];
            int start_base=-2;
            int sequence_number=0;
            //FILE *fp;
            //fp = fopen(str3, "w");
            for(;;)
            {
                size_from=sizeof(server_address_from);
                struct UDP_Packet packet_data;
                struct UDP_ACK_Packet ack_k;
                if((receive_size=recvfrom(socke,&packet_data,sizeof(packet_data),0,(struct sockaddr *)&server_address_from,&size_from))<0)
                {
                    perror("recvfrom() error");
                    exit(1);
                }
                sequence_number=packet_data.sequence_number;
                if(!packet_loss(loss_error_percentage))
                {
                    if(packet_data.sequence_number==0 && packet_data.packet_type==1)
                    {
                        cout<<"Received Initial packet "<<inet_ntoa(server_address_from.sin_addr)<<endl;
                        memset(buff,0,sizeof(buff));
                        packet_data.buffer[chunk_size]='~';
                        packet_data.buffer[chunk_size+1]='~';
                        strcpy(buff,packet_data.buffer);
                        start_base=0;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    else if(packet_data.sequence_number==start_base+1)
                    {
                        cout<<"Received: Sequence No "<<packet_data.sequence_number;
                       // cout<<endl<<<<"Debug:"<<packet_data.buffer<<endl;
                        packet_data.buffer[chunk_size]='~';
                        packet_data.buffer[chunk_size+1]='~';
                        strcat(buff,packet_data.buffer);
                        start_base=packet_data.sequence_number;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    else if(packet_data.packet_type==1 && packet_data.sequence_number!=start_base+1)
                    {
                        cout<<" OUT OF SEQ: Packet received "<<packet_data.sequence_number<<endl;
                        ack_k=ACK_createpacket(2,start_base);
                    }
                    if(packet_data.packet_type==4 && sequence_number==start_base)
                    {
                        start_base=-1;
                        ack_k=ACK_createpacket(8,start_base);
                    }
                    if(start_base>=0)
                    {
                        cout<<" SENDING acknowledgement "<<start_base<<endl;
                        if(sendto(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&server_address_from,sizeof(server_address_from))!=sizeof(ack_k))
                        {
                            perror("sending bits different from expected - 1");
                            exit(1);
                        }
                    }
                    else if(start_base==-1){
                        cout<<"Recived LAST packet"<<endl;
                        cout<<"Sending last ACK"<<endl;
                        if(sendto(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&server_address_from,sizeof(server_address_from))!=sizeof(ack_k))
                        {
                            perror("sending bits different from expected - 2");
                            exit(1);
                        }
                    }

                    if(packet_data.packet_type==4 && start_base==-1){
                        //cout<<"Message received: "<<buff<<endl;
                        cout<<"File Received: Check your folder file is stored at Client_folder location"<<endl;
                        char temp_storage[8192];
                        int x=0;
                        for(int i=0;i<strlen(buff);i++)
                        {
                            if(buff[i]!='~')
                            {
                                temp_storage[x]=buff[i];
                                x++;
                            }
                        }
                        temp_storage[x]='\0';
                        //cout<<"Message received: "<<temp_storage<<endl;
                        x=0;
                        ofstream fp(str3);
                        if(fp.is_open())
                        {
                            for(int i=0;i<strlen(temp_storage);i++)
                            {
                                if(temp_storage[i]!='~')
                                {
                                    fp<<temp_storage[x];
                                    x++;
                                }
                            }
                           fp.close();
                        }
                        memset(buff,0,sizeof(buff));
                        break;
                    }
                }
                else
                {
                    cout<<"lose: SIMULATED"<<endl;
                }
            }
            //fclose(fp);

        }
        if(choice==3)
        {
            cout<<"Client exit successfully"<<endl;
            exit(0);
        }

        cout<<"Press 1 to use list command"<<endl;
        cout<<"Press 2 to enter file sending mode"<<endl;
        cout<<"Press 3 to exit"<<endl;
        cin>>choice;
    }
    close(socke);
    return 0;
}
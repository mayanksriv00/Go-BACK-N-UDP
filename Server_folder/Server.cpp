//Server program

//headers
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<iostream>
#include<signal.h>
#include<errno.h>
#include<dirent.h>
#include<sys/types.h>


#define TIMEOUT_SECS 3


using namespace std;
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

void CatchAlarm(int p)
{
    //cout<<
}

struct UDP_Packet creat_data_packet(int sequence_number,int len,char* data)
{
    struct UDP_Packet packet;
    packet.packet_type=1;
    packet.packet_size=len;
    packet.sequence_number=sequence_number;
    memset(packet.buffer,0,sizeof(packet.buffer));
    strcpy(packet.buffer,data);
    return packet;
}

struct UDP_Packet createLastPacketT(int sequence_number,int len)
{
    struct UDP_Packet packet;
    packet.packet_type=4;
    packet.sequence_number=sequence_number;
    packet.packet_size=0;
    memset(packet.buffer,0,sizeof(packet.buffer));
    return packet;
}
int main(int argc,char *argv[]){
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    int socke;
    unsigned int clinet_address_length;
    unsigned short server_port_no;
    int message_size;
    int chunk_size;
    float loss_error_percentage;
    struct sigaction signal_handle;       
    char buffer[10000];     
    int sequence_number=0;
    int bss=0;          //base
    int window_size=5;
    int receive_string_length;   
    int tries=0;

    if(argc<3)
    {
        perror("Follow the format [ ./filename <Port_NO[UDP]> <Chunk_size>");
        exit(1);
    }
    server_port_no=atoi(argv[1]);
    chunk_size=atoi(argv[2]);
    //loss_error_percentage=(atof(argv[3]))/100.0;

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
    cout<<"Server started successfully..."<<endl;
    clinet_address_length=sizeof(client_address);

    while(true)
    {
        char buffer_msg[1024];
        memset(buffer_msg,0,sizeof(buffer_msg));
        if((message_size=recvfrom(socke,(char *)buffer_msg,1024,MSG_WAITALL,(struct sockaddr *)&client_address,&clinet_address_length))<0)
        {
            perror("No msg received");
            perror("TRY again restarting serve and client");
            exit(1);
        }
        cout<<"Message received "<<buffer_msg<<endl;        //getting messaege from the client to perform request
        //Here have to check for the type of the message
        if(strcmp(buffer_msg,"list")==0)
        {
            //list part
            //char *data_generated="Featured articles are considered to be some of the best articles Wikipedia has to offer, as determined by Wikipedia's editors. They are used by editors as examples for writing other articles. Before being listed here, articles are reviewed as featured article candidates for accuracy, neutrality, completeness, and style according to our featured article criteria. There are 5,884 featured articles out of 6,196,135 articles on the English Wikipedia (about 0.1% or one out of every 1,050 articles).";
            //Calculating segments of message to be sent
            struct dirent *de;
            DIR *dr;
            char list_result[1024];
            memset(list_result,0,sizeof(list_result));
            if ((dr = opendir("./")) == NULL)
            perror("opendir() error");
            while((de=readdir(dr))!=NULL)
            {
                //printf("%s\n",de->d_name);
                strcat(list_result,de->d_name);
               // cout<<list_result<<endl;;
                //list_result[strlen(list_result)]='\n';
                strcat(list_result,"\n");
            }
            // printf("%s",list_result);
            //printf("%s",fi);
            char *data_generated=list_result;
        // printf("Debug: %s",data_generated);
            
            int total_data_size=strlen(data_generated);
            int num_of_segments=total_data_size/chunk_size;
            if(strlen(data_generated)%chunk_size>0)
                num_of_segments++;
            
            int start_base=-1;      //base
            int start_seqno=0;
            int start_datalength=0;

            //setting alarm siginal
            signal_handle.sa_handler=CatchAlarm;
            if(sigemptyset(&signal_handle.sa_mask)<0)
            {
                perror("Failed signal");
                exit(1);
            }
            signal_handle.sa_flags=0;

            if(sigaction(SIGALRM,&signal_handle,0)<0)
            {
                perror("sigaction failed for sigalarm");
                exit(1);
            }
            //Boolean to allow ruuning progarm till last ACK received
            int noLastACK=1;       //noTearDownACK
            while(noLastACK)
            {
                while(start_seqno<=num_of_segments && (start_seqno-start_base)<=window_size)
                {
                    struct UDP_Packet pacData;
                    if(start_seqno==num_of_segments){       //Handeling terminal situation
                        pacData=createLastPacketT(start_seqno,0);
                        cout<<"Sending Last packet "<<endl;
                    }
                    else{
                        char data_seg[chunk_size];
                        strncpy(data_seg,(data_generated+start_seqno*chunk_size),chunk_size);
                        pacData=creat_data_packet(start_seqno,start_datalength,data_seg);
                        //cout<<"Debug"<<pacData.buffer<<endl;
                        cout<<"Packet Sending"<<start_seqno<<endl;
                    }
                    if(sendto(socke,&pacData,sizeof(pacData),0,(struct sockaddr *)&client_address,sizeof(client_address))!=sizeof(pacData))
                    {
                        perror("Send to send different number if bytes than expected");
                        exit(1);
                    }
                    start_seqno++;
                }
                //setting timer
                alarm(TIMEOUT_SECS);       //Must set timout sec here
                cout<<"Window is full...Waiting for the acknowledgement "<<endl;
                struct UDP_ACK_Packet ack_k;
                while((receive_string_length=recvfrom(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&client_address,&clinet_address_length))<0)
                {
                    if(errno==EINTR)
                    {
                        start_seqno=start_base+1;
                        cout<<"TIMEOUT: Starting to resend "<<endl;
                        if(tries>=15)
                        {
                            cout<<"Total tries increased: Closing"<<endl;
                            exit(1);
                        }
                        else
                        {
                            alarm(0);
                            while(start_seqno<=num_of_segments && (start_seqno-start_base)<=window_size){
                                struct UDP_Packet pacData;
                                if(start_seqno==num_of_segments)
                                {
                                    pacData=createLastPacketT(start_seqno,0);
                                    cout<<"Sending Last packet "<<endl;
                                }
                                else{
                                    char data_seg[chunk_size];
                                    strncpy(data_seg,(data_generated+start_seqno*chunk_size),chunk_size);
                                    pacData=creat_data_packet(start_seqno,start_datalength,data_seg);
                                    cout<<"Packet Sending "<<start_seqno<<endl;
                                }
                                if(sendto(socke,&pacData,sizeof(pacData),0,(struct sockaddr *)&client_address,sizeof(client_address))!=sizeof(pacData))
                                {
                                    perror("Send to send different number if bytes than expected ");
                                    exit(1);
                                }
                                start_seqno++;
                            }
                            alarm(TIMEOUT_SECS);
                        }
                        tries++;
                    }
                    else
                    {
                        perror("recfrom() failed");
                        exit(1);
                    }
                }
                if(ack_k.packet_type!=8)
                {
                    cout<<"********* ACK RECEIVED: "<<ack_k.ack_no<<endl;
                    if(ack_k.ack_no>start_base){
                        start_base=ack_k.ack_no;
                    }
                }
                else{
                    cout<<"Last ACK received"<<endl;
                    noLastACK=0;
                }
                alarm(0);
                tries=0;
            }
            //close(socke);
            //exit(0);

        }
        if(strcmp(buffer_msg,"get")==0)
        {   cout<<"Inside file get command"<<endl;
            char *f_msg="Plese send the file name to fetch\n";
            char *f_msg11="RECEIVED";
            if(sendto(socke,(const char *)f_msg,sizeof(f_msg),0,(struct sockaddr *)&client_address,sizeof(client_address))<0)
            {
                perror("SendTo failed at server side");
                exit(1);
            }
            char buffer_msg1[1024];
            memset(buffer_msg,0,sizeof(buffer_msg));
            if((message_size=recvfrom(socke,(char *)buffer_msg1,1024,MSG_WAITALL,(struct sockaddr *)&client_address,&clinet_address_length))<0)
            {
                perror("No msg received: waiting...");
                exit(1);
            }
            //cout<<buffer_msg1<<endl;
            //Getting file name from the client, now open file and perform the oeration 
            FILE *fptr=NULL;
            char c;
            //fptr = fopen(buffer_msg1, "r"); 
            if(fptr==NULL)
            {
                //cout<<"File name not present"<<endl;
                while(fptr==NULL)
                {
                    fptr = fopen(buffer_msg1, "r"); 
                    if(fptr==NULL)
                    {
                        cout<<"File name not present"<<endl;
                        char *er="ERROR";
                        memset(buffer_msg1,0,sizeof(buffer_msg1));
                        if(sendto(socke,(const char *)er,sizeof(er),0,(struct sockaddr *)&client_address,sizeof(client_address))<0)
                        {
                            perror("SendTo failed at server side");
                            exit(1);
                        }
                        if((message_size=recvfrom(socke,(char *)buffer_msg1,1024,MSG_WAITALL,(struct sockaddr *)&client_address,&clinet_address_length))<0)
                        {
                            perror("No msg received: waiting...");
                            exit(1);
                        }
                       // cout<<"DEBUG"<<buffer_msg1<<endl;
                    }
                    
                }
                //exit(0);
            }
            if(sendto(socke,(const char *)f_msg11,sizeof(f_msg11),0,(struct sockaddr *)&client_address,sizeof(client_address))<0)
            {
                perror("SendTo failed at server side");
                exit(1);
            }
            c=fgetc(fptr);
            char str_f[5000000];
            int x=0;
            while (c != EOF) 
            { 
                //printf ("%c", c); 
                str_f[x]=c;
                x++;
                c = fgetc(fptr); 
            } 
            fclose(fptr); 
            str_f[x]='\n';
            //cout<<str_f<<endl;
            char *data_generated=str_f;
        // printf("Debug: %s",data_generated);
            
            int total_data_size=strlen(data_generated);
            int num_of_segments=total_data_size/chunk_size;
            if(strlen(data_generated)%chunk_size>0)
                num_of_segments++;
            
            int start_base=-1;      //base
            int start_seqno=0;
            int start_datalength=0;

            //setting alarm siginal
            signal_handle.sa_handler=CatchAlarm;
            if(sigemptyset(&signal_handle.sa_mask)<0)
            {
                perror("Failed signal");
                exit(1);
            }
            signal_handle.sa_flags=0;

            if(sigaction(SIGALRM,&signal_handle,0)<0)
            {
                perror("sigaction failed for sigalarm");
                exit(1);
            }
            //Boolean to allow ruuning progarm till last ACK received
            int noLastACK=1;       //noTearDownACK
            while(noLastACK)
            {
                while(start_seqno<=num_of_segments && (start_seqno-start_base)<=window_size)
                {
                    struct UDP_Packet pacData;
                    if(start_seqno==num_of_segments){       //Handeling terminal situation
                        pacData=createLastPacketT(start_seqno,0);
                        cout<<"Sending Last packet "<<endl;
                    }
                    else{
                        char data_seg[chunk_size];
                        strncpy(data_seg,(data_generated+start_seqno*chunk_size),chunk_size);
                        pacData=creat_data_packet(start_seqno,start_datalength,data_seg);
                        cout<<"Packet Sending"<<start_seqno<<endl;
                    }
                    if(sendto(socke,&pacData,sizeof(pacData),0,(struct sockaddr *)&client_address,sizeof(client_address))!=sizeof(pacData))
                    {
                        perror("Send to send different number if bytes than expected");
                        exit(1);
                    }
                    start_seqno++;
                }
                //setting timer
                alarm(TIMEOUT_SECS);       //Must set timout sec here
                cout<<"Window is full...Waiting for the acknowledgement "<<endl;
                struct UDP_ACK_Packet ack_k;
                while((receive_string_length=recvfrom(socke,&ack_k,sizeof(ack_k),0,(struct sockaddr *)&client_address,&clinet_address_length))<0)
                {
                    if(errno==EINTR)
                    {
                        start_seqno=start_base+1;
                        cout<<"TIMEOUT: Starting to resend "<<endl;
                        if(tries>=15)
                        {
                            cout<<"Total tries increased: Closing"<<endl;
                            exit(1);
                        }
                        else
                        {
                            alarm(0);
                            while(start_seqno<=num_of_segments && (start_seqno-start_base)<=window_size){
                                struct UDP_Packet pacData;
                                if(start_seqno==num_of_segments)
                                {
                                    pacData=createLastPacketT(start_seqno,0);
                                    cout<<"Sending Last packet "<<endl;
                                }
                                else{
                                    char data_seg[chunk_size];
                                    strncpy(data_seg,(data_generated+start_seqno*chunk_size),chunk_size);
                                    pacData=creat_data_packet(start_seqno,start_datalength,data_seg);
                                    cout<<"Packet Sending "<<start_seqno<<endl;
                                }
                                if(sendto(socke,&pacData,sizeof(pacData),0,(struct sockaddr *)&client_address,sizeof(client_address))!=sizeof(pacData))
                                {
                                    perror("Send to send different number if bytes than expected ");
                                    exit(1);
                                }
                                start_seqno++;
                            }
                            alarm(TIMEOUT_SECS);
                        }
                        tries++;
                    }
                    else
                    {
                        perror("recfrom() failed");
                        exit(1);
                    }
                }
                if(ack_k.packet_type!=8)
                {
                    cout<<"********* ACK RECEIVED: "<<ack_k.ack_no<<endl;
                    if(ack_k.ack_no>start_base){
                        start_base=ack_k.ack_no;
                    }
                }
                else{
                    cout<<"Last ACK received"<<endl;
                    noLastACK=0;
                }
                alarm(0);
                tries=0;
            }
            //close(socke);

        }
        if(strcmp(buffer_msg,"exit server")==0)
        {
            //not possible logically used CTR+C to exit the program
            close(socke);
            exit(0);
            break;
        }
    }

    return 0;
}


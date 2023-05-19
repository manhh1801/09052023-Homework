#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

void main()
{
    /*<Setting up socket>*/

    printf("\n");

    int server=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sv_addr;
    sv_addr.sin_family=AF_INET;
    sv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    sv_addr.sin_port=htons(1280);

    if(bind(server, (struct sockaddr*)&sv_addr, sizeof(sv_addr)))
    {
        perror("Bind address to socket failed");
        printf("\n");
        return;
    }

    /*<>*/


    /*<Allowing connection>*/

    if(listen(server, 10))
    {
        perror("Connect failed");
        printf("\n");
        return;
    }

    /*<>*/


    /*<Communicating>*/

    struct pollfd* fds=NULL;
    nfds_t fd_count=2;

    typedef struct client
    {
        int client;
        struct sockaddr_in cl_addr;
        char** name;
        unsigned int name_count;
        char state;
        struct client *next;
    }
    client;
    client** clients=malloc(sizeof(client*));
    *clients=NULL;

    while(1)
    {
        fds=(struct pollfd*)malloc(fd_count*sizeof(struct pollfd));
        client* temp=*clients;
        client* prev=NULL;
        nfds_t new_fd_count=fd_count;

        fds[0].fd=server;
        fds[0].events=POLLIN;
        int index=1;
        while(temp!=NULL)
        {
            fds[index].fd=temp->client;
            fds[index].events=POLLIN;
            temp=temp->next;
            index+=1;
        }
        fds[fd_count-1].fd=STDIN_FILENO;
        fds[fd_count-1].events=POLLIN;

        if(poll(fds, fd_count, -1)<0)
        {
            printf("Exception occured!\n");
            break;
        }

        temp=*clients;
        for(int index=1; index<fd_count-1; index++)
        {
            if(fds[index].revents&POLLIN)
            {
                unsigned int buf_count=0;
                char **buf_storage=NULL;

                if(read(temp->client, &buf_count, 4)<=0)
                {
                    printf("%s:%d - ", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));
                    if(temp->name_count==0) {printf("(unknown)");}
                    else
                    {
                        for(int index=0; index<temp->name_count; index++) {printf("%s", (temp->name)[index]);}
                    }
                    printf(" disconnected.\n");

                    client* delete=temp;
                    temp=temp->next;
                    free(delete);
                    new_fd_count--;
                    if(prev!=NULL) {prev->next=temp;}
                    else {*clients=temp;}
                    continue;
                }

                buf_storage=(char**)malloc(8*buf_count);
                for(int index=0; index<buf_count; index++)
                {
                    char *buf=malloc(64);
                    read(temp->client, buf, 64);
                    buf_storage[index]=buf;
                }

                if(temp->state==1)
                {
                    char message_type=1;

                    client* temp2=*clients;
                    while(temp2!=NULL)
                    {
                        if((temp2->state==0)||(temp2->client==fds[index].fd))
                        {
                            temp2=temp2->next;
                            continue;
                        }
                        write(temp2->client, &message_type, 1);
                        if(temp->name_count==0)
                        {
                            unsigned int name_count=1;
                            write(temp2->client, &name_count, 4);
                            char unknown_name[64]="(unknown)";
                            write(temp2->client, unknown_name, 64);
                        }
                        else
                        {
                            write(temp2->client, &(temp->name_count), 4);
                            for(int index=0; index<temp->name_count; index++)
                            {
                                write(temp2->client, (temp->name)[index], 64);
                            }
                        }

                        write(temp2->client, &buf_count, 4);
                        for(int index=0; index<buf_count; index++) {write(temp2->client, buf_storage[index], 64);}
                        temp2=temp2->next;
                    }

                    printf("From %s:%d - ", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));
                    if(temp->name_count==0) {printf("(unknown)");}
                    else
                    {
                        for(int index=0; index<temp->name_count; index++) {printf("%s", (temp->name)[index]);}
                    }
                    printf(": ");
                    for(int index=0; index<buf_count; index++) {printf("%s", buf_storage[index]);}
                }

                else
                {
                    char verified=0;

                    int buf_index=0;
                    int char_index=0;
                    char found=0;
                    for(; buf_index<buf_count; buf_index++)
                    {
                        char_index=0;
                        for(; char_index<64; char_index++)
                        {
                            if(buf_storage[buf_index][char_index]==':')
                            {
                                found=1;
                                break;
                            }
                        }
                        if(found==1) {break;}
                    }
                    if(found==1)
                    {
                        if(buf_index!=0||char_index!=0)
                        {
                            if(char_index!=63)
                            {
                                if(buf_storage[buf_index][char_index+1]!=0) {verified=1;}
                            }
                            else if(buf_index!=buf_count-1)
                            {
                                if(buf_storage[buf_index][0]!=0) {verified=1;}
                            }
                        }
                    }
                    if(verified==1)
                    {
                        printf("Connection %s:%d verified: ", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));
                        temp->name_count=buf_index+1;
                        temp->name=(char**)malloc(8*(temp->name_count));
                        for(int index=0; index<buf_index; index++)
                        {
                            char* buf=malloc(64);
                            memcpy(buf, buf_storage[index], 64);
                            (temp->name)[index]=buf;
                            printf("%s", buf);
                        }
                        char* buf=malloc(64);
                        memcpy(buf, buf_storage[buf_index], char_index);
                        (temp->name)[buf_index]=buf;
                        printf("%s", buf);
                        temp->state=1;
                    }
                    else {printf("Connection %s:%d cannot be verified.", inet_ntoa((temp->cl_addr).sin_addr), ntohs((temp->cl_addr).sin_port));}

                    write(temp->client, &verified, 1);
                }

                printf("\n");
            }

            prev=temp;
            temp=temp->next;
        }

        temp=*clients;
        if(fds[fd_count-1].revents&POLLIN)
        {
            char message_type=0;
            unsigned int buf_count=0;
            char** buf_storage=NULL;

            while(1)
            {
                char* input_recv=malloc(65);
                fgets(input_recv, 65, stdin);
                int buf_length=strlen(input_recv);
                if(buf_length==64)
                {
                    char* buf=malloc(64);
                    memcpy(buf, input_recv, 64);
                    buf_storage=(char**)realloc(buf_storage, 8*(buf_count+1));
                    buf_storage[buf_count++]=buf;
                    if(buf[63]==10)
                    {
                        buf[63]='\0';
                        break;
                    }
                }
                else
                {
                    if(buf_length!=1)
                    {
                        char* buf=malloc(64);
                        memcpy(buf, input_recv, 64);
                        buf_storage=(char**)realloc(buf_storage, 8*(buf_count+1));
                        buf_storage[buf_count++]=buf;
                        for(int index=buf_length-1; index<64; index++) {buf[index]='\0';}
                    }
                    break;
                }
            }

            while(temp!=NULL)
            {
                if(temp->state==0)
                {
                    temp=temp->next;
                    continue;
                }
                write(temp->client, &message_type, 1);
                write(temp->client, &buf_count, 4);
                for(int index=0; index<buf_count; index++) {write(temp->client, buf_storage[index], 64);}
                temp=temp->next;
            }
            printf("Sent: ");
            for(int index=0; index<buf_count; index++) {printf("%s", buf_storage[index]);}
            printf("\n");
        }

        if(fds[0].revents&POLLIN)
        {
            client* new_client=malloc(sizeof(client));
            int cl_addr_length=sizeof(new_client->cl_addr);
            new_client->client=accept(server, (struct sockaddr*)&(new_client->cl_addr), &cl_addr_length);
            new_client->name=NULL;
            new_client->name_count=0;
            new_client->state=0;
            new_client->next=*clients;
            *clients=new_client;
            new_fd_count+=1;
            printf("Accepted connection: %s:%d\n", inet_ntoa((new_client->cl_addr).sin_addr), ntohs((new_client->cl_addr).sin_port));
        }

        fd_count=new_fd_count;
        free(fds);
    }

    printf("\n");

    /*<>*/


    /*<Close connection>*/

    close(server);
    printf("Server closed.\n");
    printf("\n");
    return;

    /*<>*/

}
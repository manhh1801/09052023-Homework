#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

int main()
{

    /*<Setting up socket and connection>*/

    printf("\n");

    int client=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in cl_addr;
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    cl_addr.sin_port=htons(1280);

    if(connect(client, (struct sockaddr*)&cl_addr, sizeof(cl_addr)))
    {
        perror("Connect failed");
        printf("\n");
        return 1;
    }
    printf("Connected: %s:%d\n", inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
    printf("\n");

    /*<>*/


    /*<Communicating>*/

    struct pollfd* fds=NULL;
    char state=0;

    while(1)
    {
        fds=(struct pollfd*)malloc(2*sizeof(struct pollfd));
        fds[0].fd=client;
        fds[0].events=POLLIN;
        fds[1].fd=STDIN_FILENO;
        fds[1].events=POLLIN;

        if(poll(fds, 2, -1)<0)
        {
            printf("Exception occured!\n");
            break;
        }

        if(fds[0].revents&POLLIN)
        {
            if(state==0)
            {
                char verified=0;
                if(read(client, &verified, 1)<=0)
                {
                    printf("Server closed.\n");
                    break;
                }
                if(verified==0)
                {
                    printf("Cannot verify connection, please type 'name:pass' with no more than 64 characters in each parameter.\n");
                }
                else
                {
                    printf("Connection verified.\n");
                    state=1;
                }
            }

            else
            {
                char message_type=0;
                if(read(client, &message_type, 1)<=0)
                {
                    printf("Server closed.\n");
                    break;
                }

                if(message_type==0)
                {
                    printf("From server: ");
                    unsigned int buf_count=0;
                    read(client, &buf_count, 4);
                    for(int index=0; index<buf_count; index++)
                    {
                        char *buf=malloc(64);
                        read(client, buf, 64);
                        printf("%s", buf);
                    }
                }

                else
                {
                    unsigned int name_count=0;
                    read(client, &name_count, 4);
                    printf("From ");
                    for(int index=0; index<name_count; index++)
                    {
                        char *buf=malloc(64);
                        read(client, buf, 64);
                        printf("%s", buf);
                    }
                    printf(": ");
                    unsigned int buf_count=0;
                    read(client, &buf_count, 4);
                    for(int index=0; index<buf_count; index++)
                    {
                        char *buf=malloc(64);
                        read(client, buf, 64);
                        printf("%s", buf);
                    }
                }

                printf("\n");
            }
        }

        if(fds[1].revents&POLLIN)
        {
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

            printf("Sent: ");
            write(client, &buf_count, 4);
            for(int index=0; index<buf_count; index++)
            {
                write(client, buf_storage[index], 64);
                printf("%s", buf_storage[index]);
            }
            printf("\n");
        }

        free(fds);
    }

    printf("\n");

    /*<>*/


    /*<Close connection>*/

    close(client);
    printf("Client closed.\n");
    printf("\n");
    return 1;

    /*<>*/

}
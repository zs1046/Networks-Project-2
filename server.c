/******************************
 * Zach Sotak
 * CS 4310 - Fall 2017
 * Project 2
 *
 ******************************
 * A component of a Command and Control (C2C)
 * server that is used to allow agents in a multi-agent
 * system to become aware of each other.
 * C2C server is able to maintain a list of active
 * agents and respond to various actions
 * as issued by the agents.
 ******************************/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#define MAXBUF 1024
#define MAXHOSTNAME 200
#define MAXRESPONSE  20
#define MAXACTIVEMEM 20
#define MAXACTION  10
#define MAXPORT 6
#define MAXLISTEN 6
#define filename "log.txt"
#define fName "list.txt"
#define TRUE 1

struct connection
{
    char agent[MAXHOSTNAME];
    struct connection *nextConnection;
    unsigned long long time;
};

int main (int argc, char *argv[])
{
    int opt = TRUE;
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    pid_t childpid;
    socklen_t ADDRLEN;
    fd_set readfds;

    char cPort[MAXPORT];
    char cResponse [MAXRESPONSE];
    char cAgentIP[MAXHOSTNAME];
    char cAction [MAXACTION];
    char cServerIP[MAXHOSTNAME];
    char buffer[MAXBUF];
    int  sockfd, networkSockfd, numBytesRead, numBytesWrite, portNum;
    struct sockaddr_in my_addr, agent_addr;

    FILE *fptr;
    FILE *fptrList;
    unsigned long fileSize;

    struct connection *connectionList;
    connectionList = NULL;
    int connectionCounter = 0;
    bool isConnection;

    //checks for port errors
    if (argc < 2)
    {
        fprintf(stderr,"ERROR: no port provided\n");
        return 1;
    }

    //creates socket and checks for errors
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr,"ERROR: couldn't create socket\n");
        return 1;
    }



    //initialize portNum to zero, and then sets it with input
    memset(cPort, 0, MAXPORT);
    sprintf(cPort,"%s",argv[1]);
    portNum = atoi(cPort);

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(portNum);

    //clear cServer
    memset(cServerIP, 0, MAXHOSTNAME);

    //converts addr into text string
    inet_ntop(AF_INET, &my_addr.sin_addr.s_addr, cServerIP, sizeof(my_addr));
    printf("Server: %s \n", cServerIP);

    //binds socket to port and checks to make sure it works properly
    if (bind(sockfd, (struct sockaddr *) &my_addr,sizeof(my_addr)) < 0)
    {
        fprintf(stderr,"ERROR: bind failed\n");
        return 1;
    }

        printf("Listening on ip %s and port %d\n", cServerIP, ntohs(my_addr.sin_port));



    //creates listening queue with a max of 6 elements

     if (listen(sockfd,MAXLISTEN) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    ADDRLEN = sizeof(my_addr);
    puts("Waiting for connections...");

    while (true)
    {
        
        memset(cAgentIP, 0, MAXHOSTNAME);
        memset(cResponse, 0, MAXRESPONSE);
        memset(buffer, 0, MAXBUF);

        //flag if connection is active
        bool isConnection = false;

        //open log.file
        if ((fptr = fopen(filename,"a")) == NULL)
        {
            printf("Error: couldn't open file");
            return 1;
        }
        else
        {
            fseek(fptr, 0, SEEK_END);
            fileSize = ftell(fptr);
            rewind(fptr);
        }

        if ((networkSockfd = accept(sockfd, (struct sockaddr *) &agent_addr, &ADDRLEN)) < 0)
        {
            fprintf(stderr,"ERROR: accept failed\n");
            return 1;

        }

        //converts addr into text string
        inet_ntop(AF_INET, &agent_addr.sin_addr.s_addr, cAgentIP, sizeof(agent_addr));

        //reading from agent
        if ( (numBytesRead = read(networkSockfd, buffer, MAXBUF)) < 0)
        {
            fprintf(stderr,"ERROR: reading data from agent \n");
            return 1;
        }


        //set action to hold request from agent
        memset(cAction,0,MAXACTION);
        sprintf(cAction,"%s",buffer);

        //writes to log file
        fprintf(fptr,"%s : recieved a %s action from agent %s\n", asctime(timeinfo), cAction, cAgentIP);

        //checks if agent is active
        struct connection *traverse;
        traverse = connectionList;
        while (traverse != NULL && isConnection == false)
        {
            if ((strncmp(traverse->agent, cAgentIP, MAXHOSTNAME)) == 0)
            {
                isConnection = true;
            }
            traverse = traverse->nextConnection;
        }

        //conditions for actions coming from agent #JOIN, #LEAVE, #LIST, #LOG

        if ((strncmp(cAction, "#JOIN", MAXACTION)) == 0)
        {
            //check if connection is active
            if( isConnection == true)
            {
                //sent to agent
                sprintf(cResponse,"$ALREADY CONNECTED");
                numBytesWrite = write (networkSockfd, cResponse, sizeof(cResponse));

                //sent to log file
                fprintf(fptr,"%s : Responded to agent %s with %s\n",
                        asctime(timeinfo), cAgentIP, cResponse);
            }

            //respond with "$OK"
            else
            {
                //sent to agent
                sprintf(cResponse,"$OK");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));

                if (connectionList == NULL)
                {
                    connectionList = malloc( sizeof(struct connection) );
                    strncpy(connectionList->agent, cAgentIP, MAXHOSTNAME);
                    connectionList->nextConnection = NULL;
                }

                else //( connectionList != NULL )
                {
                    struct connection *p1;
                    struct connection *p2;

                    //set second pointer to head of list
                    p2 = connectionList;

                    // Create a node at the beginning of the list
                    p1 = malloc( sizeof(struct connection) );

                    //initialize a new connection
                    strncpy(p1->agent, cAgentIP, MAXHOSTNAME);

                    p1->nextConnection = p2;
                    connectionList = p1;
                }

                connectionCounter++;

                //sent to log file
                fprintf(fptr,"%s : Responded to agent %s with %s\n",
                        asctime(timeinfo), cAgentIP, cResponse);
            }
            fclose(fptr);
        }

        if ((strncmp(cAction, "#LEAVE", MAXACTION)) == 0)
        {
            //check list if connection is active
            //if connection is active -> respond with "$OK"
            if(isConnection == true)
            {
                //sent to agent
                sprintf(cResponse,"$OK");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));

                //sent to log file
                fprintf(fptr,"%s : Responded to agent %s with %s\n",
                        asctime(timeinfo), cAgentIP, cResponse);

                //traverse list to find connection
                struct connection *p1;
                struct connection *p2;

                p1 = connectionList;

                //if connection is at the beginning of the list
                if((strncmp(p1->agent, cAgentIP, MAXHOSTNAME)) == 0)
                {
                    connectionList = connectionList->nextConnection;
                    free(p1); //deletes connection from active list
                }

                else
                {
                    bool delete = false;
                    p2 = connectionList->nextConnection;

                    //if connection is in the middle of the list
                    while(p2 != NULL && p1 != NULL && delete == false)
                    {
                        if((strncmp(p2->agent, cAgentIP, MAXHOSTNAME)) == 0)
                        {
                            struct connection *temp;
                            temp = p2;
                            p2 = p2->nextConnection;
                            p1->nextConnection = p2;
                            free(temp);
                            delete = true;
                        }
                        else
                        {
                            //Traverse list
                            p2 = p2->nextConnection;
                            p1 = p1->nextConnection;
                        }
                    }
                    //if connection is at the end of the list
                    if(p1 != NULL && p2 == NULL)
                    {
                        if((strncmp(p1->agent, cAgentIP, MAXHOSTNAME)) == 0)
                        {
                            free(p1);
                        }
                    }
                }
                connectionCounter--;
            }

            //respond with "$NOT FOUND"
            else
            {
                //sent to agent
                sprintf(cResponse,"$NOT FOUND");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));

                //sent to log file
                fprintf(fptr,"%s : Responded to agent %s with %s\n",
                        asctime(timeinfo), cAgentIP, cResponse);
            }
            fclose(fptr);
        }

        if ((strncmp(cAction, "#LIST", MAXACTION)) == 0)
        {
            if(isConnection == true)
            {
                //send copy of active connection list to agent
                //clear buffer
                memset(buffer, 0, MAXBUF);

                struct connection *traversal;
                traversal = connectionList;
                char tempAgent[MAXHOSTNAME];
                memset(tempAgent, 0, MAXHOSTNAME);

                //writing to list.txt
                if ((fptrList = fopen(fName,"w")) == NULL)
                {
                    printf("Error opening file");
                    return 1;
                }

                while (traversal != NULL)
                {
                    //get values of active connection list
                    strncpy(tempAgent, traversal->agent, MAXHOSTNAME);

                    //send values to list.txt
                    fprintf(fptrList," %s", tempAgent);
                    traversal = traversal->nextConnection;
                }
                fclose(fptrList);

                //read file into buffer
                fptrList = fopen(fName,"r");

                memset(buffer, 0, MAXBUF);
                numBytesRead = fread(buffer, sizeof(char), sizeof(buffer), fptrList);
                if(numBytesRead == 0) break;
                if(numBytesRead < 0)
                {
                    printf("Error in reading list.txt to buffer\n");
                }

                void *pointer = buffer;
                //send list file pieces at a time
                while (numBytesRead > 0)
                {
                    numBytesWrite = write(networkSockfd, buffer, numBytesRead);
                    if(numBytesWrite <=0)
                    {
                        printf("Error in writing list.txt to buffer");
                        return 1;
                    }

                    //keep track of how many bytes are needed to write
                    numBytesRead -= numBytesWrite;
                    pointer += numBytesWrite;

                }

                sprintf(cResponse,"$OK");
                fprintf(fptr,"%s : Responded to agent %s with list (%s)\n",
                        asctime(timeinfo), cAgentIP, cResponse);

            }


            else
            {
                //sent to agent
                sprintf(cResponse,"$NOT CONNECTED");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));

                //sent to log file
                fprintf(fptr,"%s : No response is supplied to agent %s ( %s )\n",
                        asctime(timeinfo), cAgentIP, cResponse);
            }

            fclose(fptr);
        }

        if ((strncmp(cAction, "#LOG", MAXACTION)) == 0)
        {
            //check list if connection is active
            //if connection is  not active -> ignore request

            if (isConnection == false)
            {

                //sent to agent
                sprintf(cResponse,"$NOT CONNECTED");
                numBytesWrite = write (networkSockfd, cResponse, strlen(cResponse));

                //sent to log file
                fprintf(fptr,"%s : No response supplied to agent %s ( %s )\n",
                        asctime(timeinfo), cAgentIP, cResponse);

                fclose(fptr);

            }

            //send 'log.txt' to agent
            else
            {
                //Clear buffer
                memset(buffer, 0, MAXBUF);

                sprintf(cResponse,"$OK");
                fprintf(fptr,"%s : supplied log file to agent %s ( %s )\n",
                        asctime(timeinfo), cAgentIP, cResponse);

                fclose(fptr);
                fptr = fopen(filename,"r");
                //read file into buffer
                numBytesRead = fread(buffer, sizeof(char), sizeof(buffer), fptr);
                if(numBytesRead == 0) break;
                if(numBytesRead < 0)
                {
                    printf("Error reading log.txt to buffer\n");
                }

                void *pointer = buffer;
                //send log file pieces at a time
                while (numBytesRead > 0)
                {
                    numBytesWrite = write(networkSockfd, buffer, numBytesRead);
                    if(numBytesWrite <=0)
                    {
                        printf("Error writing log.txt to buffer");
                    }

                    //keep track of how many bytes needed to write
                    numBytesRead -= numBytesWrite;
                    pointer += numBytesWrite;

                }
            }
        }
        close(networkSockfd);


    }
    return 0;

}


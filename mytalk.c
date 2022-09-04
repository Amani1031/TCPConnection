#include <stdio.h>
#include <talk.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netdb.h>


void chat(int impfd, int v, int n){
    if (n == 0){
        start_windowing();
    }
    set_verbosity(v);

    struct pollfd pfda[2];
    int i, mlen, len;
    char buf[100];
    int done = 0;
    do{
        /* set up array of pollfd structs */
        pfda[0].fd = STDIN_FILENO;
        pfda[0].events = POLLIN;
        pfda[1].fd = impfd;
        pfda[1].events = POLLIN;
        poll(pfda, 2, -1);
        if (((pfda[0].revents) & POLLIN) != 0){
            /* read a line and send it to remote */
            if (has_hit_eof() != 0){
                update_input_buffer();
            }
            else{
                while((i = read_from_input(buf, 100 > 0))){
                    if (n == 1){
                        /* If windowing is off */
                        fputs(buf, stdout);
                    }
                    else{
                        /* If windowing is on */
                        write_to_output(buf, i);
                    }
                    mlen = send(impfd, buf, i, 0);
                }
            }
        }
        if (((pfda[1].revents) & POLLIN) != 0){
            /* recieve a line and print it */
            mlen = recv(impfd, buf, sizeof(buf), 0);
            if (n == 1){
                /* If windowing is off */
                fputs(buf, stdout);
            }
            else{
                /* If windowing is on */
                write_to_output(buf, mlen);
            }
        }
        if( !strncmp(buf, "bye", 3)){
            done = 1;
        }
    }while(done == 0);
    
    stop_windowing();
}

void readLine(char *dest){
    do {
        scanf("%c", dest++);
    } while (*(dest - 1) != '\n');
    *(dest - 1) = '\0';
}

int serverSide(int v, int port, int a){
    int sockfd, newsock, mlen;
    socklen_t len;
    struct hostent *peerhostent;
    struct sockaddr_in sa, newsockinfo, peerinfo;

    char peeraddr[80];
    char input[4];
    char buf[80];

    if (v > 0){
        printf("Server is creating socket.\n");
    }
    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port); 
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    /* bind */
    bind(sockfd, (struct sockaddr *)&sa, sizeof(sa));

    /* listen */
    if (v > 0){
        printf("Server is now listening.\n");
    }

    listen(sockfd, 1);

    /* accept */
    if (v > 1){
        printf("Server is in the process of accepting a connection.\n");
    }
 
    len = sizeof(newsockinfo);

    newsock = accept(sockfd, (struct sockaddr *)&peerinfo, &len);

    mlen = recv(newsock, buf, sizeof(buf), 0);

    if (a == 0){
        printf("Mytalk request from %s. Accept? (y/n)?\n", buf);
    
        readLine(input); /* The input should be 'y' for the server to connect*/

        printf("The input is: %s\n", input);

        if ((strcmp(input, "y") == 0) || (strcmp(input, "Y") == 0) ||
         (strcmp(input, "yes") == 0) || (strcmp(input, "YES") == 0)){
            mlen = send(newsock, "ok", strlen("ok"), 0);
            if (v > 0){
                printf("Server has successfully accepted the connection.\n");
            }
        }

        else{
            mlen = send(newsock, "bye", sizeof("bye"), 0);
            exit(EXIT_SUCCESS);
        }
    }

    if (v > 0){
        printf("Your chat is now starting.\n");
    }
    
    return newsock;

}

int clientSide(int v, int port, char *hostname){
    int sockfd, mlen;
    struct sockaddr_in sa;
    struct hostent *hostent;
    char buf[80];
    char uname[80];
        
    printf("Waiting for response from %s", hostname);

    /* Get host details */
    hostent = gethostbyname(hostname);
    if (v > 0){
        printf("Getting host details.\n");
    }

    /* Create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (v > 0){
        printf("Creating a socket.\n");
    }

    /* Assign the socket */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = *(uint32_t*)(hostent->h_addr_list[0]);

    if (v > 0){
        printf("Waiting for server permission to connect.\n");
    }
    /* Connect */
    if (connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == 0){
        uid_t t;
        t = getuid();
        struct passwd *pw = getpwuid(t);
        strcpy(uname, pw->pw_name);
        mlen = send(sockfd, uname, sizeof(uname), 0);

        mlen = recv(sockfd, buf, sizeof(buf), 0);
        if (strcmp(buf, "ok") != 0){
            printf("%s declined connection.", hostname);
            exit(EXIT_SUCCESS);
        }
    }

    if (v > 0){
        printf("Your chat is now starting.\n");
    }

    return sockfd;

}


int main(int argc, char **argv){
    int port;
    char hostname[80];
    char buf[80];
    int serverfd;
    int clientfd;
    int passfd;

    if (argc == 1){
        perror("No arguments have been inputted.");
        exit(EXIT_FAILURE);
    }

    /* Boolean values repersenting
    if user is server or the client. */
    int server = 0, client = 0;

    /* Boolean values repersenting
    if -v, -a, and -N options have been
    inputted */
    int v = 0, a = 0, n = 0;

    /* Port is the last command line input */
    port = atoi(argv[argc - 1]);

    int i;
    if (argc > 2){
        for (i = 1 ; i < (argc - 1) ; i++){
            if (strcmp(argv[i], "-v") == 0){
                v += 1;
            }
            if (strcmp(argv[i], "-a") == 0){
                printf("%d\n", a);
                a = 1;
            }
            if (strcmp(argv[i], "-N") == 0){
                n = 1;
            }
        }
    }

    if (v > 0){
        printf("The vebrosity level has been set to %d\n", v);
    }
    
    /* Find whether hostname has been
    inputted */
    if ((argc - 2 - v - a - n) == 1){
        client += 1;
        strcpy(hostname, argv[argc - 2]);
        if (v > 0){
            printf("The host is, %s.", hostname);
        }
    }
    else{
        server += 1;
        strcpy(hostname, "localhost");
        if (v > 0){
            printf("The host is, %s.", hostname);
        }
    }

    if (server == 1){
        serverfd = serverSide(v, port, a);
        chat(serverfd, v, n);
        close(serverfd);
    }
    else if (client == 1){
        clientfd = clientSide(v, port, hostname);
        chat(clientfd, v, n);
        close(clientfd);
    }

    return 0;
}
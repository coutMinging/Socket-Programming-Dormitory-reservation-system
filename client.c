/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "24394" // the port client will be connecting to

#define MAXDATASIZE 1000 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    struct sockaddr_in my_addr;
    socklen_t addr_len = sizeof(my_addr);
    int getsock_check;

    const char *hostname = "localhost";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("Client is up and running.\n");

    freeaddrinfo(servinfo); // all done with this structure
    getsock_check = getsockname(sockfd, (struct sockaddr *)&my_addr, &addr_len);
    if (getsock_check == -1)
    {
        perror("getsockname");
        exit(1);
    }
    // printf("Client is using dynamic port number %d\n", ntohs(my_addr.sin_port));
    int dynamic_port = ntohs(my_addr.sin_port);
    while (1)
    {
        printf("Enter Department Name: ");
        char department_name[500];
        fgets(department_name, sizeof(department_name), stdin);
        department_name[strcspn(department_name, "\n")] = '\0';
        if (send(sockfd, department_name, strlen(department_name), 0) == -1)
        {
            perror("send");
            exit(1);
        }
        printf("Client has sent Department %s to Main Server using TCP over port %d\n", department_name, dynamic_port);

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';

        printf("%s", buf);
    }

    close(sockfd);

    return 0;
}
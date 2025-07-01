#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#define MAX_DEPARTMENTS 100
#define MAX_DEPARTMENTS_NAME_LEN 50
#define MAX_LINE_LEN 1024
#define MAX_SIZE 1000
int client_ID = 0;

typedef struct
{
    int campus_id;
    char departments[MAX_DEPARTMENTS][MAX_DEPARTMENTS_NAME_LEN];
    int department_cout;
} CampusServer;

/*
    READ THE FILE AND PROESS THE DATA
*/
void trim_whitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    // Trim trailing space
    if (*str == 0) // All spaces?
        return;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    *(end + 1) = 0;
}

//to check if there has same department in one serverid
int is_department_unique(const CampusServer *server, char *department)
{
    trim_whitespace(department);
    for (int i = 0; i < server->department_cout; i++)
    {
        if (strcasecmp(server->departments[i], department) == 0)
        {
            return 0;
        }
    }
    return 1;
}

//process the txt file
void read_list_file(const char *filename, CampusServer *server, int *serverCount)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Main Server has read the department list from list.txt.\n");
    }

    char line[MAX_LINE_LEN];
    int index = -1;
    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = 0;
        if (isdigit(line[0]))
        {
            index++;
            server[index].campus_id = atoi(line);
            server[index].department_cout = 0;
        }
        else
        {
            char *token = strtok(line, ";");
            while (token != NULL)
            {
                if (is_department_unique(&server[index], token))
                {
                    strcpy(server[index].departments[server[index].department_cout], token);
                    // int flag = is_department_unique(&server[index],token);
                    // printf("flag %d\n",flag);
                    server[index].department_cout++;
                }

                token = strtok(NULL, ";");
            }
        }
    }
    *serverCount = index + 1;
    fclose(file);
}

void print_servers(const CampusServer *server, int server_count)
{
    printf("Total number of Campus Servers:%d\n", server_count);
    for (int i = 0; i < server_count; i++)
    {
        printf("Campus Server %d contains %d distinct departments\n", server[i].campus_id, server[i].department_cout);
    }
}

/*
    SOCKET PART
*/

//process the request from client
int find_campus_server_ID(const CampusServer *server, int server_cout, const char *deparment)
{
    for (int i = 0; i < server_cout; i++)
    {
        for (int j = 0; j < server[i].department_cout; j++)
        {
            if (strcmp(server[i].departments[j], deparment) == 0)
            {
                return server[i].campus_id;
            }
        }
    }
    return -1;
}

/*
* !!!!!!!!!!!!!!!!!!!!!The code below are copy from beej's networking programming, and modified!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
#define PORT "24394"
#define BACKLOG 100
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
int main()
{

    // print_servers(servers, server_count);
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("Main server is up and running.\n");
    CampusServer servers[100];
    int server_count = 0;
    read_list_file("list.txt", servers, &server_count);
    print_servers(servers, server_count);

    while (1)
    { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        //printf("server: got connection from %s\n", s);
        client_ID++;

        if (!fork())
        {                  // this is the child process
            close(sockfd); // child doesn't need the listener
            while (1)
            {
                char buf[512];
                int byte_count = recv(new_fd, buf, sizeof buf, 0);
                if (byte_count == -1)
                {
                    perror("recv");
                    close(new_fd);
                    exit(1);
                }
                buf[byte_count] = '\0';
                printf("Main server has received the request on Department %s from client %d using TCP over port %s\n", buf, client_ID, PORT);
                int campus_id = find_campus_server_ID(servers, server_count, buf);
                char response[MAX_SIZE];
                if (campus_id != -1)
                {
                    snprintf(response, MAX_SIZE, "Client has received results from Main Server: %s is associated with Campus server %d\n", buf, campus_id);
                    printf("Main Server has sent searching result to client %d using TCP over port %s\n", client_ID, PORT);
                }
                else
                {
                    char server_ids[MAX_SIZE] = "";
                    for (int i = 0; i < server_count; i++)
                    {
                        char id[100];
                        snprintf(id, sizeof(id), "%d", servers[i].campus_id);
                        strcat(server_ids, id);
                        if (i < server_count - 1)
                        {
                            strcat(server_ids, ", ");
                        }
                    }
                    printf("Department %s does not show up in Campus server %s\n", buf, server_ids);
                    snprintf(response, MAX_SIZE, "%s Not found\n",buf);
                    printf("The Main Server has sent \"Department Name: Not found\" to client %d using TCP over port %s\n", client_ID, PORT);
                }
                if (send(new_fd, response, strlen(response) + 1, 0) == -1)
                {
                    perror("send");
                }
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }

    return 0;
}
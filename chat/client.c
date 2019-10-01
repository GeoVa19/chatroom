#include "client.h"

int main(int argc, char *argv[]) {
    int sockfd, portno, n, m, maxfd, leave = FALSE;

    fd_set rfds;

    struct sockaddr_in server_addr;
    struct hostent *server;

    const char *token;
    char buffer[BUFFER_SIZE];
    char name[NAMESIZE];
    char message[MESSAGE_SIZE];

    if (argc < 3)
        error("Usage: client hostname port_number");

    printf("%s\n", "Please give a name (max "NAME_SIZE(NAMESIZE)" characters): ");
    fgets(name, NAMESIZE /*+ 1*/, stdin);
    strtok(name, "\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error opening socket!");

    server = gethostbyname(argv[1]);
    if (server == NULL)
        error("Error resolving host name!");

    bzero((char *) &server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET; //IPv4

    bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);

    portno = atoi(argv[2]);
    server_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        error("Error connecting to the server!");

    printf("\n%s\n\n", "To exit the chat room, press 'q' (without the single quotes!).");

    strcat(name, ": ");

    while (TRUE) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        maxfd = 0;

        FD_SET(sockfd, &rfds);

        if (sockfd > maxfd)
            maxfd = sockfd;

        n = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (n == -1)
            error("ERROR: Select failed!");
        else if (n == 0)
            continue;
        else if (FD_ISSET(0, &rfds)) {
            strcat(message, name);
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, BUFFER_SIZE, stdin);
            strcat(message, buffer);

            if(buffer[0] == 'q') {
                token = strtok(name, ":");
                strcpy(message, token);
                strcat(message, " left.\n");
                leave = TRUE;
            }

            m = send(sockfd, message, strlen(message), 0);
            if (m < 0) {
                close(sockfd);
                error("Failure to send message!");
            }

            if (leave == TRUE)
                break;
        }

        if (FD_ISSET(sockfd, &rfds)) {
            memset(buffer, 0, sizeof(buffer));

            m = recv(sockfd, message, sizeof(message), 0);
            if (m <= 0) {
                close(sockfd);
                error("Failure to receive message!");
            }

            message[m] = '\0';
            printf("%s", message);
        }

        memset(message, 0, sizeof(message));
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

void error(const char *error_message) {
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE);
}


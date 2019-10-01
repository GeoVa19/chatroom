#include "server.h"

int main(void) {
    fd_set master; //master file descriptor
    fd_set read_fds; //file descriptors έτοιμοι για ανάγνωση
    int fdmax; //max file descriptors

    int sockfd; //server's socket
    int newsockfd; //clients' socket
    struct sockaddr_in client_addr;
    int clilen;

    char message[MESSAGE_SIZE];
    char history[HISTORY_SIZE];

    int isFull;

    int nbytes;

    int optval = 1;
    int i, j, error_code;

    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; //stream socket
    hints.ai_protocol = IPPROTO_TCP; //TCP protocol
    hints.ai_flags = AI_PASSIVE;

    /*http://man7.org/linux/man-pages/man3/getaddrinfo.3.html*/
    if ((error_code = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        error(gai_strerror(error_code)); //print a human-readable error message
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd < 0) {
            continue;
        }

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL)
        error("Server failed to bind.\n");

    freeaddrinfo(res);

    //listen
    if (listen(sockfd, MAX_CLIENTS) == -1)
        error("ERROR: listen");

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(sockfd, &master);

    fdmax = sockfd;

    memset(message, 0, sizeof(message));
    memset(history, 0, sizeof(history));

    strcpy(history, message);

    while (TRUE) {
        read_fds = master;
        //http://man7.org/linux/man-pages/man2/select.2.html
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            close(sockfd);
            error("ERROR: select");
        }

        //for each connection
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                    clilen = sizeof(client_addr);
                    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);

                    if (newsockfd == -1) {
                        error("ERROR: accept");
                    } else {
                        FD_SET(newsockfd, &master);
                        if (newsockfd > fdmax) {
                            fdmax = newsockfd;
                        }

                        isFull = is_full(history);
                        if (isFull)
                            memset(history, 0, sizeof(history));
                        else {
                            if (send(newsockfd, history, strlen(history), 0) == -1) {
                                close(sockfd);
                                close(newsockfd);
                                error("Failure to send old conversations.");
                            }
                        }
                    }
                } else {
                    nbytes = recv(i, message, sizeof(message), 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("Socket %d closed.\n", i);
                        } else {
                            close(sockfd);
                            close(newsockfd);
                            error("Failure to receive message!");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        strcat(history, message);

                        for (j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master)) {
                                if (j != sockfd && j != i) {
                                    if (send(j, message, nbytes, 0) == -1) {
                                        close(sockfd);
                                        close(newsockfd);
                                        error("Failure to send message!");
                                    }
                                }
                            }
                        }

                        memset(message, 0, sizeof(message));
                    }
                }
            }
        }
    }

    close(sockfd);
    close(newsockfd);

    return EXIT_SUCCESS;
}

void error(const char *error_message) {
    fprintf(stderr, "%s\n", error_message);
    exit(EXIT_FAILURE);
}

int is_full(const char *history) {
    if (history[HISTORY_SIZE - 1] != '\0')
        return TRUE;

    return FALSE;
}


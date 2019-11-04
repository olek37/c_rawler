#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char* extractText(char* html, int length) {
    return("4431");
}

int main(int argc, char** argv) {
    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024};
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET / HTTP/1.1\r\nHost: %s\r\n\r\n";
    struct protoent *protoent;
    
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;
    

    char *hostname = argv[1];
    unsigned short server_port = 80;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length large: %d\n", request_len);
        exit(EXIT_FAILURE);
    }

    /* Build the socket. */
    protoent = getprotobyname("tcp");
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);

    /* Build the address. */
    hostent = gethostbyname(hostname);

    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    /* Actually connect. */
    connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in));

    /* Send HTTP request. */
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        nbytes_total += nbytes_last;
    }

    /* Read the response. */
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
        write(STDOUT_FILENO, buffer, nbytes_total);
    }

    close(socket_file_descriptor);
    exit(EXIT_SUCCESS);
}
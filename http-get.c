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

#define MAX_REQUEST_LEN 1024
#define BUF_SIZE 1024
#define MAX_HOSTNAME_LEN 256
#define MAX_DEPTH 5
#define MAX_BREADTH 5

bool isEndOfHtml(char* buffer, int size) {
    char* tag = "</html>";

    for(int i = 0; i < size - 7; i++) {
        if( strncmp(buffer+i, tag, 7) == 0) {
            return true;
        }
    }

    return false;
}

char* getFilenameFromHostname(char* hostname) {
    char* filename = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    char* dir = "raw/";
    char* ext = ".txt";
    strcat(filename, dir);
    strcat(filename, hostname);
    strcat(filename, ext);

    return filename;
}

char* getLinksFilenameFromHostname(char* hostname) {
    char* filename = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    char* dir = "raw/";
    char* ext = "-links.txt";
    strcat(filename, dir);
    strcat(filename, hostname);
    strcat(filename, ext);

    return filename;
}

void lineLinksToFile(char* line, int len, FILE * fp_write) {
    char* tag = "href=";
    for(int i = 0; i < len - 5; i++) {
        
        if( strncmp(line+i, tag, 5) == 0) {
            // NEXT CHAR IS "
            int j = i + 6;
            // 34 -> "
            char* temp = (char*)malloc(sizeof(char) * len);
            while(*(line + j) != 34) { 
                *(temp+j-i-6) = *(line + j);
                j++;
            }
            write(fileno(fp_write), temp, j-i-6);
            write(fileno(fp_write), "\n", 1);

            i = j;
        }
        
    }
    return;
}

void linksToFile(char* hostname) {
    FILE * fp_read = fopen(getFilenameFromHostname(hostname), "r");
    FILE * fp_write = fopen(getLinksFilenameFromHostname(hostname), "w+");
    char * line = NULL;
    char * line_links = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    int len = 0;
    int read;
    
    while (true) {
        read = getline(&line, &len, fp_read);
        if(read == -1) {
            break;
        }
        lineLinksToFile(line, read, fp_write);
    }

    fclose(fp_read);
    fclose(fp_write);

    return;
}

void httpGetToFile(char* hostname) {
    char* filename = getFilenameFromHostname(hostname);
    FILE * fp = fopen(filename, "w+");

    char * buffer = (char*)malloc(sizeof(char) * BUF_SIZE);
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET / HTTP/1.1\r\nHost: %s\r\n\r\n";
    struct protoent *protoent;
    
    in_addr_t in_addr;
    int request_len;
    int socket_file_descriptor;
    int nbytes_total, nbytes_last;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;    
    bool html_end;

    unsigned short server_port = 80;

    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);

    protoent = getprotobyname("tcp");
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    hostent = gethostbyname(hostname);

    printf("Hello: %s\n", hostname);
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    printf("Hello: %s\n", hostname);
     
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in));

    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        nbytes_total += nbytes_last;
    }

    html_end = false;

    while (!html_end) {
        nbytes_total = read(socket_file_descriptor, buffer, BUF_SIZE);

        write(fileno(fp), buffer, nbytes_total);

        html_end = isEndOfHtml(buffer, BUF_SIZE);
    }

    close(socket_file_descriptor);
    if (buffer)
        free(buffer);
    if (filename)
        free(filename);
    fclose(fp);
    return;
}

void recursiveCrawl(char * hostname, int current_depth) {
    printf("Crawling: %s at depth: %d\n", hostname, current_depth);
    if(current_depth >= MAX_DEPTH) {
        return;
    }

    httpGetToFile(hostname);
    linksToFile(hostname);


    FILE * fp_read = fopen(getLinksFilenameFromHostname(hostname), "r");
    char* line = NULL;
    int len = 0;
    int read;
    
    int link_count = 0;
    while (link_count < MAX_BREADTH) {
        read = getline(&line, &len, fp_read);
        if(read == -1 ) {
            break;
        }
        recursiveCrawl(line, current_depth+1);
    }

    fclose(fp_read);
}

int main() {
    char* root = "iana.org";
    recursiveCrawl(root, 0);
}
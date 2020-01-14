#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
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
#define MAX_DEPTH 10
#define MAX_BREADTH 20

char links[10000];

bool isEndOfHtml(char* buffer, int size) {
    char* tag = "</html>";
    char* moved = "cation:";

    for(int i = 0; i < size - 7; i++) {
        if( strncmp(buffer+i, tag, 7) == 0 || strncmp(buffer+i, moved, 7) == 0) {
            return true;
        }
    }

    return false;
}

char* getFilenameFromHostname(char* hostname) {
    char* filename = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    char* dir = "indexed/";
    char* ext = ".txt";
    strcat(filename, dir);
    strcat(filename, hostname);
    strcat(filename, ext);

    return filename;
}

char* getLinksFilenameFromHostname(char* hostname) {
    char* filename = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    char* dir = "indexed/";
    char* ext = "-links.txt";
    strcat(filename, dir);
    strcat(filename, hostname);
    strcat(filename, ext);

    return filename;
}

void lineLinksToFile(char* line, int len, FILE * fp_write) {
    char* tag = "www.";
    for(int i = 0; i < len - 4; i++) {
        
        if( strncmp(line+i, tag, 4) == 0) {
            int j = i + 4;
            // 34 -> " 47 -> /
            char* temp = (char*)malloc(sizeof(char) * len);
            while(*(line + j) != 34 && *(line + j) != 47) { 
                *(temp+j-i-4) = *(line + j);
                j++;
            }
            write(fileno(fp_write), tag, 4);
            write(fileno(fp_write), temp, j-i-4);
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
    size_t len = 0;
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

int httpGetToFile(char* hostname) {
    printf("%s\n", hostname);
    char* filename = getFilenameFromHostname(hostname);
    FILE * fp = fopen(filename, "w+");

    char * buffer = (char*)malloc(sizeof(char) * BUF_SIZE);
    char request[MAX_REQUEST_LEN];
    char request_template[] = "GET / HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X x.y; rv:42.0) Gecko/20100101 Firefox/42.0\r\n\r\n";
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
    
    struct hostent *he;
    he = gethostbyname(hostname);
    if(he == NULL) {
        return 0;
    }

    memcpy(&sockaddr_in.sin_addr, he->h_addr_list[0], he->h_length);
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    int connection_succeeded = connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in));
    printf("%d\n", connection_succeeded);
    if(connection_succeeded == -1) {
        return 0;
    }

    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        nbytes_total += nbytes_last;
    }

    html_end = false;

    int i = 0;
    while (!html_end && i++ < 100) {
        nbytes_total = read(socket_file_descriptor, buffer, BUF_SIZE);
        if(nbytes_total <= 0) {
            break;
        }
        write(fileno(fp), buffer, nbytes_total);

        html_end = isEndOfHtml(buffer, BUF_SIZE);
    }

    close(socket_file_descriptor);
    if (buffer)
        free(buffer);
    if (filename)
        free(filename);
    fclose(fp);
    return i;
}

void recursiveCrawl(char * hostname, int current_depth) {
    printf("Crawling: %s at depth: %d\n", hostname, current_depth);
    if(current_depth >= MAX_DEPTH) {
        return;
    }

    int i = httpGetToFile(hostname);
    printf("%d\n", i);
    if(i < 5) {
        return;
    }
    linksToFile(hostname);

    FILE * fp_read = fopen(getLinksFilenameFromHostname(hostname), "r");
    char* line = NULL;
    size_t len = 0;
    int read;
    
    int link_count = 0;
    while (link_count++ < MAX_BREADTH) {
        for(int i = MAX_DEPTH; i >= current_depth; i--) {
            read = getline(&line, &len, fp_read);
            if ((line)[read - 1] == '\n') {
                (line)[read - 1] = '\0';
            }
            if(read == -1 ) {
                break;
            }
            if (strstr(links, line) == NULL) {
                strcat(links, line);
                recursiveCrawl(line, i);
            } else {
                break;
            }
        }
    }

    fclose(fp_read);
}

int main() {
    char* root = "patriotyczna.listastron.pl";
    recursiveCrawl(root, 0);
}
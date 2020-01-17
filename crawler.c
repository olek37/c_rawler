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
#define MAX_LINES_READ 1024
#define BUF_SIZE 1024
#define MAX_HOSTNAME_LEN 256

#define MIN_SITE_CONTENT_LEN 5
#define MAX_CRAWL_DEPTH 5
#define MAX_CRAWL_BREADTH 10

#define MAX_LINKS_MEMORY 32768

char VISITED_LINKS[MAX_LINKS_MEMORY];

/* detect end of html to finish fetching site */
int end_of_html(char* buffer, int size) 
{
    char* closing_html_tag = "</html>";
    for(int i = 0; i < size - strlen(closing_html_tag); i++) {
        if(strncmp(buffer+i, closing_html_tag, strlen(closing_html_tag)) == 0) {
            return 1;
        }
    }
    return 0;
}

/* detect location header key to finish fetching site */
int moved_temporarily(char * buffer, int size) 
{
    char* location_header_key = "Location:";

    for(int i = 0; i < size - strlen(location_header_key); i++) {
        if(strncmp(buffer+i, location_header_key, strlen(location_header_key)) == 0) {
            return 1;
        }
    }
    return 0;
}

/* get filename from hostname with proper extension */
char* filename_from_hostname(char* hostname, char * extension) 
{
    char* filename = (char*)malloc(sizeof(char) * MAX_HOSTNAME_LEN);
    char* directory = "indexed/";
    strcat(filename, directory);
    strcat(filename, hostname);
    strcat(filename, extension);

    return filename;
}

/* write links from a site content file line to a separate file */
void line_links_to_file(char* line, int line_length, FILE * fd) 
{
    char* subdomain = "www.";
    int subdomain_length = strlen(subdomain);

    for(int i = 0; i < line_length - subdomain_length; i++) {

        if( strncmp(line+i, subdomain, subdomain_length) == 0) {
            int j = i + subdomain_length;
            char* temp = (char*)malloc(sizeof(char) * line_length);
            char c = *(line + j);
            while( (c >= 97 && c <= 122) || (c >= 65 && c <= 90) || (c >= 48 && c <= 57) || c == 45 || c == 46) { 
                *(temp+j-i-subdomain_length) = *(line + j);
                j++;
                c = *(line + j);
            }
            printf("%s\n", temp);
            write(fileno(fd), subdomain, subdomain_length);
            write(fileno(fd), temp, j-i-subdomain_length);
            write(fileno(fd), "\n", 1);

            i = j;
        }   
    }
    return;
}

/* process lines from site content file */
void links_to_file(char* hostname) {

    FILE * fd_read = fopen(filename_from_hostname(hostname, ".txt"), "r");
    FILE * fd_write = fopen(filename_from_hostname(hostname, ".links"), "w+");
    char * line = NULL;
    size_t len = 0;
    int read;

    while (true) {
        read = getline(&line, &len, fd_read);
        if(read == -1) {
            break;
        }
        line_links_to_file(line, read, fd_write);
    }

    fclose(fd_read);
    fclose(fd_write);

    return;
}

/* make request, fetch site content to a file */
int http_get_to_file(char* hostname) 
{
    printf("Getting: %s\n", hostname);

    char* filename = filename_from_hostname(hostname, ".txt");
    FILE * fd_content = fopen(filename, "w+");

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

    unsigned short server_port = 80;

    // prepare request
    request_len = snprintf(request, MAX_REQUEST_LEN, request_template, hostname);

    protoent = getprotobyname("tcp");
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    
    // if the host is not resolved - return with no lines written
    struct hostent *he;
    he = gethostbyname(hostname);
    if(he == NULL) {
        printf("Cannot resolve host\n");
        return 0;
    }

    memcpy(&sockaddr_in.sin_addr, he->h_addr_list[0], he->h_length);
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    // if the connection fails - return with no lines written
    int connection_succeeded = connect(socket_file_descriptor, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in));
    if(connection_succeeded == -1) {
        printf("Cannot connect to host\n");
        return 0;
    }

    nbytes_total = 0;
    while (nbytes_total < request_len) {
        nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
        nbytes_total += nbytes_last;
    }

    int html_end_flag = 0;
    int moved_temporarily_flag = 0;
    int lines_read_count = 0;

    // read from socket while the explicit conditions are met
    while (!html_end_flag && !moved_temporarily_flag && lines_read_count++ < MAX_LINES_READ) {
        nbytes_total = read(socket_file_descriptor, buffer, BUF_SIZE);
        if(nbytes_total <= 0) {
            break;
        }
        write(fileno(fd_content), buffer, nbytes_total);

        html_end_flag = end_of_html(buffer, BUF_SIZE);
        moved_temporarily_flag = moved_temporarily(buffer, BUF_SIZE);
    }

    // free the allocated memory
    close(socket_file_descriptor);
    if (buffer)
        free(buffer);
    if (filename)
        free(filename);
    fclose(fd_content);
    
    return lines_read_count;
}

// recursive crawl with depth and breadth limits
void crawl(char * hostname, int current_depth) 
{
    printf("Crawling: %s at depth: %d\n", hostname, current_depth);

    if(current_depth >= MAX_CRAWL_DEPTH) {
        printf("Crawl depth reached\n");
        return;
    }

    int lines_read = http_get_to_file(hostname);

    if(lines_read < MIN_SITE_CONTENT_LEN) {
        printf("Site content too short for indexing\n");
        return;
    }

    links_to_file(hostname);

    FILE * fd_links = fopen(filename_from_hostname(hostname, ".links"), "r");
    char* line = NULL;
    size_t len = 0;
    int read;
    
    int link_count = 0;
    
    while (link_count++ < MAX_CRAWL_BREADTH) {
        for(int i = MAX_CRAWL_DEPTH; i >= current_depth; i--) {

            read = getline(&line, &len, fd_links);
            if ((line)[read - 1] == '\n') {
                (line)[read - 1] = '\0';
            }

            if(read == -1 ) {
                break;
            }

            if (strstr(VISITED_LINKS, line) == NULL) {
                strcat(VISITED_LINKS, line);
                crawl(line, i);
            } else {
                break;
            }

        }
    }

    fclose(fd_links);
}

// the website of choice is one with a lot of non-https references
int main() 
{
    char* root = "patriotyczna.listastron.pl";
    crawl(root, 0);
}
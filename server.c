#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define PORT 5556
#define MAXMSG 1024
#define MESSAGE "Jest ok"
#define MAX_CMD_LEN 1024
#define MAX_RESP_LEN 1024


int make_socket(uint16_t port)
{
    int sock;
    struct sockaddr_in name;

    sock = socket(PF_INET, SOCK_STREAM, 0);

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr*)&name, sizeof(name));

    return sock;
}

void write_to_client (int filedes, char * response)
{
  printf("Found following results: \n%s\n\n", response);
  int nbytes = write (filedes, response, strlen (response) + 1);
  printf("Sent response to client\n");
}

FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

char* get_results (char * buffer) 
{
  FILE *cmd;
  char bash_command[MAX_CMD_LEN];
  char bash_template[] = "cd indexed; grep %s -l *.txt ";
  char bash_result[MAX_CMD_LEN];
  char * response = (char*)malloc(MAX_RESP_LEN * 100);
  snprintf(bash_command, MAX_CMD_LEN, bash_template, buffer);
  cmd = popen(bash_command, "r");
  while (fgets(bash_result, sizeof(bash_result), cmd)) {
      strcat(response, bash_result);
  }

  pclose(cmd);
  return response;
}

int read_from_client(int filedes)
{
  char buffer[MAXMSG];
  int nbytes;
  char * response;
  nbytes = read(filedes, buffer, MAXMSG);
  if(nbytes == 0) {
    return -1;
  } else {
    fprintf(stderr, "Client requested:\n%s\n\n", buffer);
    response = get_results(buffer);
    write_to_client(filedes, response);
    return 0;
  }
}

int main(void)
{
  int sock;
  fd_set active_fd_set, read_fd_set;
  int i;
  struct sockaddr_in clientname;
  size_t size;

  sock = make_socket(PORT);
  listen(sock, 1);

  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);

  while (1) {
    read_fd_set = active_fd_set;
    select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

    for (i = 0; i < FD_SETSIZE; ++i) {
      if (FD_ISSET(i, &read_fd_set)) {
        if (i == sock) {
          int new;
          size = sizeof(clientname);
          new = accept(sock, (struct sockaddr*)&clientname, (socklen_t *)&size);
          FD_SET(new, &active_fd_set);
        }
        else {
          if (read_from_client(i) < 0) {
            close(i);
            FD_CLR(i, &active_fd_set);
          }
        }
      }
    }
  }
}
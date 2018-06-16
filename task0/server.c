#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "line_parser.h"
#include "common.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <math.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define NAME_SIZE 100
#define BUFFER_SIZE 2048
#define PORT "2018"
#define TRUE 1
#define FALSE 0
#define BACKLOG 10
#define error(format, ...) fprintf(stdout, format, __VA_ARGS__)
#define debug(res, addr) if (debug_mode) { error("%s|Log: %s\n", addr, res); }

int debug_mode = FALSE;

client_state state = { .conn_state = IDLE, .client_id = NULL, .sock_fd = -1};

void assert(int ret, char *err){
    if (0 > ret){
        perror(err);
        exit(-1);
    }
}

void debug_lookup(cmd_line *cmd){
    for (int i = 0; i < cmd->arg_count; i++){
        if (0 == strcmp(cmd->arguments[i], "-d")){
            debug_mode = TRUE;
            break;
        }
    }
}

void nok(int fd, char *message){
    char to_send[BUFFER_SIZE];
    sprintf(to_send, "nok %s", message);
    assert(send(fd, to_send, sizeof(to_send), 0), "Error in send()");
}

int handle_client(int client_fd){
    char buffer[BUFFER_SIZE];
    
    // Client loop
    while (1){
        assert(recv(client_fd, buffer, BUFFER_SIZE, 0), "Error in recv()");
        cmd_line* cmd = parse_cmd_lines(buffer);
        debug_lookup(cmd);
        debug(buffer, state.server_addr);

        if (0 == strncmp(buffer, "hello", 5)){
            if (IDLE != state.conn_state){
                nok(client_fd, "state");
                state.conn_state = IDLE;
                close(client_fd);
                return -2;
            }

            state.conn_state = CONNECTED;
            state.client_id = "client1";
            printf("Client %s connected\n", state.client_id);
            char response[BUFFER_SIZE];
            sprintf(response, "hello %s", state.client_id);
            assert(send(client_fd, response, strlen(response), 0), "Error in send()");

        } else if (0 == strncmp(cmd->arguments[0], "bye", 3)){
            if (CONNECTED != state.conn_state){
                return -2;
            }
            printf("Client %s disconnected\n", state.client_id);
            state.client_id = NULL;
            state.conn_state = IDLE;
            close(client_fd);
            free_cmd_lines(cmd);
            return 0;
        } else {
            error("%s|ERROR: Unknown message %s\n", state.client_id, buffer);
        }
        free_cmd_lines(cmd);
    }
}

int main(int argc, char **argv){
    char hostname[NAME_SIZE];
    state.conn_state = gethostname(hostname, NAME_SIZE);

    struct addrinfo hints, *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;     
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    
    assert(getaddrinfo(state.server_addr, PORT, &hints, &servinfo), "Error in getaddrinfo()");
    state.sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    assert(state.sock_fd, "Error in socket()");
    assert(bind(state.sock_fd, servinfo->ai_addr, servinfo->ai_addrlen), "Error in bind()");
    assert(listen(state.sock_fd, BACKLOG), "Error in listen()");
    freeaddrinfo(servinfo);
    
    struct sockaddr_storage addr;
    socklen_t addr_size = sizeof(addr);

    while (1){
        printf("Main loop\n");
        int new_fd = accept(state.sock_fd, (struct sockaddr *)&addr, &addr_size);
        handle_client(new_fd);
    }
    return 0;
}
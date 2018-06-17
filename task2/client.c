#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "line_parser.h"
#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define ADDRESS_SIZE 100
#define BUFFER_SIZE 2048
#define LS_STATUS_SIZE 3
#define PORT "2018"
#define TRUE 1
#define FALSE 0
#define error(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
#define debug(res, addr) if (debug_mode) { error("%s|Log: %s\n", addr, res); }

int debug_mode = FALSE;

client_state state = { .conn_state = IDLE, .client_id = NULL, .sock_fd = -1};

int handle_response(char *res, char *addr){
    debug(res, addr);
    if (0 == strncmp(res, "nok", 3)){
        error("Error: %s\n", res + 4);
        strcpy(state.server_addr, "nil");
        state.conn_state = IDLE;
        state.client_id = NULL;
        state.sock_fd = -1;
        return -1;
    }
    return 0;
}

void assert(int ret, char *err){
    if (0 > ret){
        perror(err);
        exit(-1);
    }
}

int exec(cmd_line* cmd, char *original_cmd){
    if (0 == strcmp(cmd->arguments[0], "conn")){
        if (IDLE != state.conn_state){
            return -2;
        }
        state.conn_state = CONNECTING;

        struct addrinfo hints, *servinfo, *tmp;
        memset(&hints , 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int fd;

        if (0 != getaddrinfo(cmd->arguments[1], PORT, &hints, &servinfo)){
            perror( "Error in getaddrinfo()");
            exit(-1);
        }

        for (tmp = servinfo; tmp != NULL; tmp = tmp->ai_next){
            if (-1 == (fd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol))){
                continue;
            }
            if (-1 == connect(fd, tmp->ai_addr, tmp->ai_addrlen)){
                close(fd);
                continue;
            }
            break;
        }
        if (NULL == tmp){
            error("Counln't connect\n");
            exit(-1);
        }

        freeaddrinfo(servinfo);
        // assert(fd, "Error in socket()");
        // assert(connect(fd, servinfo->ai_addr, servinfo->ai_addrlen), "Error in connect()");
        
        assert(send(fd, original_cmd, sizeof(original_cmd), 0), "Error in send()");
        char buffer[BUFFER_SIZE];
        assert(recv(fd, buffer, BUFFER_SIZE, 0), "Error in recv()");
        
        if (0 > handle_response(buffer, cmd->arguments[1]) || 0 != strncmp(buffer, "hello ", 6)){
            error("Wrong response %s\n", buffer);
            return -1;
        }
        strcpy(state.client_id, buffer + 6);
        state.conn_state = CONNECTED;
        state.sock_fd = fd;
        strcpy(state.server_addr, cmd->arguments[1]);
        return 0;

    } else if (0 == strcmp(cmd->arguments[0], "bye")){
        if (CONNECTED != state.conn_state){
            return -2;
        }
        assert(send(state.sock_fd, "bye", sizeof("bye"), 0), "Error in send()");
        state.client_id = NULL;
        state.conn_state = IDLE;
        state.sock_fd = -1;
        strcpy(state.server_addr, "nil");
        return 0;

    } else if (0 == strcmp(cmd->arguments[0], "ls")){
        if (CONNECTED != state.conn_state){
            return -2;
        }
        assert(send(state.sock_fd, original_cmd, sizeof(original_cmd), 0), "Error in send()");
        char buffer[BUFFER_SIZE];
        assert(recv(state.sock_fd, buffer, LS_STATUS_SIZE, 0), "Error in recv()");

        if (0 == strcmp(buffer, "nok")){
            assert(recv(state.sock_fd, buffer + 3, LS_RESP_SIZE, 0), "Error in recv()");
            error("Server Error: %s\n", buffer);
        } else {
            // ok recieved
            buffer[2] = ' ';
            assert(recv(state.sock_fd, buffer + 3, LS_RESP_SIZE, 0), "Error in recv()");
        }
        if (0 > handle_response(buffer, state.server_addr)){
            error("Wrong response %s\n", buffer);
            return -1;
        }
        printf("Files list:\n%s\n", buffer + 2);
        return 0;

    } else if (0 == strcmp(cmd->arguments[0], "get")){
        if (CONNECTED != state.conn_state){
            return -2;
        }
        assert(send(state.sock_fd, original_cmd, sizeof(original_cmd), 0), "Error in send()");
        char buffer[BUFFER_SIZE];
        assert(recv(state.sock_fd, buffer, LS_STATUS_SIZE, 0), "Error in recv()");

        if (0 == strcmp(buffer, "nok")){
            assert(recv(state.sock_fd, buffer + 3, LS_RESP_SIZE, 0), "Error in recv()");
            error("Server Error: %s\n", buffer);
        } else {
            // ok recieved
            buffer[2] = ' ';
            assert(recv(state.sock_fd, buffer + 3, LS_RESP_SIZE, 0), "Error in recv()");
        }
        if (0 > handle_response(buffer, state.server_addr)){
            error("Wrong response %s\n", buffer);
            return -1;
        }
        // Getting the file size
        char *index = buffer + 3;
        long size = 0;
        while (*index){
            size = size * 10 + *index - '0';
            index++;
        }
        // Get the file content
        for (long i = 0 ; i < size ; i += BUFFER_SIZE){
            
        }
        return 0;

    }else if (0 == strcmp(cmd->arguments[0], "quit")){
        free(cmd);
        exit(0);
    } else {
        assert(send(state.sock_fd, original_cmd, strlen(original_cmd), 0), "Error in send()");
    }
    return 0;
}

void debug_lookup(cmd_line *cmd){
    for (int i = 0; i < cmd->arg_count; i++){
        if (0 == strcmp(cmd->arguments[i], "-d")){
            debug_mode = TRUE;
            break;
        }
    }
}

int main(int argc, char **argv){
    char str[BUFFER_SIZE];
    char address[ADDRESS_SIZE] = "nil";
    char id[ADDRESS_SIZE];
    state.server_addr = address;
    state.client_id = id;

    while (1){
        // Infinite loop until quit / error

        printf("server:%s>", state.server_addr);
        
        if (NULL == fgets(str, BUFFER_SIZE, stdin)){
            perror("Error in fgets()");
            exit(-1);
        }
        cmd_line* cmd = parse_cmd_lines(str);
        if (NULL == cmd){
            continue;
        }
        debug_lookup(cmd);

        int res = exec(cmd, str);
        if (-2 == res){
            printf("Exectution error\n");
        } else if (0 > res){
            free_cmd_lines(cmd);
            exit(-1);
        }
        free_cmd_lines(cmd);
    }
    return 0;
}
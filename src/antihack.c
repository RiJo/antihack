#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>         /* close() */
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <arpa/inet.h>

#include "antihack.h"
#include "daemon.h"

/*
    Todo:
        * support for udp
        * reload lua script on custom interrupt
        * fix chdir() in daemon.c
        * zombie process when killing child
        * daemonize after socket is created
*/

static lua_State *L;
static int sockfd;

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    int arg, daemon = 0, port = DEFAULT_PORT;
    while((arg = getopt(argc, argv, "dhp:v")) != EOF) {
        switch (arg) {
            case 'd':
                daemon = 1;
            break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            break;
            case 'p':
                port = atoi(optarg);
                DEBUG("port set to %d\n", port);
            break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
            break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
            break;
        }
    }

    // initialize lua
    L = lua_open();
    luaL_openlibs(L);
    (void)luaL_dofile(L, "script.lua");

    start_server(port, daemon);
    cleanup();
    return EXIT_SUCCESS;
}

void signal_handler(int signal) {
    DEBUG("received interrupt %d\n", signal);
    switch (signal) {
        case SIGINT:
        case SIGTERM:
        case SIGHUP:
            fprintf(stderr, "\nterminating %s...\n", PROGRAM_NAME);
            cleanup();
            exit(EXIT_SUCCESS);
        break;
    }
}

void print_help() {
    printf("Usage: %s [-dhv] [-p PORT]\n", PROGRAM_NAME);
    printf("Server for rejecting incoming connections on a specific port\n\n");
    printf("  -d                        run %s as a daemon\n", PROGRAM_NAME);
    printf("  -h                        display this help and exit\n");
    printf("  -p PORT                   specify which port to listen to\n");
    printf("  -v                        display version information and exit\n");
}

void print_usage() {
    printf("Usage: %s [-dhv] [-p PORT]\n", PROGRAM_NAME);
    printf("Try '%s --help' for more information.\n", PROGRAM_NAME);
}

void print_version() {
    printf("%s, version %s, released %s\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_DATE);
    printf("   compiled %s, %s\n\n", __DATE__, __TIME__);
    printf("Rikard Johansson, 2010\n");
}

void cleanup() {
    lua_close(L);
    DEBUG("lua script file closed\n");

    close(sockfd);
    DEBUG("server terminated\n");
}

void start_server(int port, int daemon) {
    struct sockaddr_in self;

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "Error: could not create socket\n");
        exit(EXIT_FAILURE);
    }

    bzero(&self, sizeof(self));
    self.sin_family = AF_INET;
    self.sin_port = htons(port);
    self.sin_addr.s_addr = INADDR_ANY;

    if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 ) {
        fprintf(stderr, "Error: could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    if ( listen(sockfd, 20) != 0 ) {
        fprintf(stderr, "Error: could not start listen on socket\n");
        exit(EXIT_FAILURE);
    }

    // Should be safe to cut pipes now...
    if (daemon) {
        daemonize(PID_FILE);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

        const long timestamp = time(NULL);
        const char *ip = inet_ntoa(client_addr.sin_addr);

        DEBUG("incoming connection from %s\n", ip);
        connection_established(timestamp, ip);

        if (send_reply()) {
            const char *reply = reply_message(timestamp, ip);
            send(clientfd, reply, strlen(reply), 0);
        }

        if (close_connection()) {
            close(clientfd);
            connection_closed(timestamp, ip);
            DEBUG("connection terminated\n");
        }
        else {
            if (fork() == 0) {
                char temp[BUFFER_SIZE];
                while(recv(clientfd, &temp, BUFFER_SIZE, 0) > 0) {
                    DEBUG("data received: %s\n", temp);
                    data_received(timestamp, ip, (const char *)&temp);
                }
                close(clientfd);
                connection_closed(timestamp, ip);
                DEBUG("client closed connection\n");
                exit(EXIT_SUCCESS);
            }
            DEBUG("connection forked and left to its destiny\n");
        }
    }
}

void connection_established(const long timestamp, const char *ip) {
    DEBUG("calling lua function %s()\n", LUA_FUN_ESTABLISHED);
    lua_getglobal(L, LUA_FUN_ESTABLISHED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_call(L, 2, 0);
}

void connection_closed(const long timestamp, const char *ip) {
    DEBUG("calling lua function %s()\n", LUA_FUN_CLOSED);
    lua_getglobal(L, LUA_FUN_CLOSED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_call(L, 2, 0);
}

int send_reply() {
    DEBUG("calling lua function %s()\n", LUA_FUN_SEND_REPLY);
    lua_getglobal(L, LUA_FUN_SEND_REPLY);
    lua_call(L, 0, 1);
    int send_reply = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return send_reply;
}

int close_connection() {
    DEBUG("calling lua function %s()\n", LUA_FUN_CLOSE_CONNECTION);
    lua_getglobal(L, LUA_FUN_CLOSE_CONNECTION);
    lua_call(L, 0, 1);
    int close_connection = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return close_connection;
}

const char *reply_message(const long timestamp, const char *ip) {
    DEBUG("calling lua function %s()\n", LUA_FUN_REPLY_MESSAGE);
    lua_getglobal(L, LUA_FUN_REPLY_MESSAGE);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_call(L, 2, 1);
    const char *reply_message = lua_tostring(L, -1);
    lua_pop(L, 1);
    return reply_message;
}

void data_received(const long timestamp, const char *ip, const char *data) {
    DEBUG("calling lua function %s()\n", LUA_FUN_DATA_RECEIVED);
    lua_getglobal(L, LUA_FUN_DATA_RECEIVED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushstring(L, data);
    lua_call(L, 3, 0);
}
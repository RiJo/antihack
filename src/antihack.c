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
*/

static lua_State *L;
static int sockfd;

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    int arg, daemon = 0, port = DEFAULT_PORT;
    while((arg = getopt(argc, argv, "dp:")) != EOF) {
        switch (arg) {
            case 'd':
                daemon = 1;
            break;
            case 'p':
                port = atoi(optarg);
                DEBUG("port set to %d\n", port);
            break;
        }
    }

    if (daemon) {
        daemonize(PID_FILE);
    }

    // initialize lua
    L = lua_open();
    luaL_openlibs(L);
    (void)luaL_dofile(L, "script.lua");

    start_server(port);

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

void cleanup() {
    lua_close(L);
    DEBUG("lua script file closed\n");

    close(sockfd);
    DEBUG("server terminated\n");
}

void start_server(int port) {
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

    while (1) {
        int clientfd;
        struct sockaddr_in client_addr;
        socklen_t addrlen=sizeof(client_addr);

        clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

        incoming(time(NULL), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (send_reply()) {
            const char *reply = reply_message();
            send(clientfd, reply, strlen(reply), 0);
        }

        close(clientfd);
    }
}

void incoming(long timestamp, char *ip, int port) {
    DEBUG("calling lua function %s()\n", LUA_FUN_INCOMING);
    lua_getglobal(L, LUA_FUN_INCOMING);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_call(L, 3, 0);
}

int send_reply() {
    DEBUG("calling lua function %s()\n", LUA_FUN_SEND_REPLY);
    lua_getglobal(L, LUA_FUN_SEND_REPLY);
    lua_call(L, 0, 1);
    int send_reply = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return send_reply;
}

const char *reply_message() {
    DEBUG("calling lua function %s()\n", LUA_FUN_REPLY_MESSAGE);
    lua_getglobal(L, LUA_FUN_REPLY_MESSAGE);
    lua_call(L, 0, 1);
    const char *reply_message = lua_tostring(L, -1);
    lua_pop(L, 1);
    return reply_message;
}
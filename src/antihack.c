#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>         /* close() */
#include <sys/wait.h>       /* wait() waitpid() */
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
        * chroot() on script to make it safe (specify run dir?)
        * fix chdir() in daemon.c (necessary to fix?)
        * ability to get info about client?
*/

static lua_State *L;
static int sockfd;
static char *script = NULL;
static int is_daemon = 0;
static int port = DEFAULT_PORT;

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGCHLD, signal_handler);
    signal(SIGUSR1, signal_handler);

    handle_arguments(argc, argv);

    L = lua_open();
    luaL_openlibs(L);
    load_lua_script(script);

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
            exit(signal);
        case SIGCHLD:
            DEBUG("waiting for terminated childs\n");
            wait(NULL); // kill zombie processes
        break;
        case SIGUSR1:
            load_lua_script(script);
        break;
    }
}

void handle_arguments(int argc, char **argv) {
    int arg;
    while((arg = getopt(argc, argv, "dhp:s:v")) != EOF) {
        switch (arg) {
            case 'd':
                is_daemon = 1;
            break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            break;
            case 'p':
                port = atoi(optarg);
                DEBUG("port set to %d\n", port);
            break;
            case 's':
                script = malloc(strlen(optarg) + 1);
                strcpy(script, optarg);
                script[strlen(optarg)] = '\0';
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
}

void print_help() {
    printf("Usage: %s [-dhsv] [-p PORT]\n", PROGRAM_NAME);
    printf("Server for rejecting incoming connections on a specific port\n\n");
    printf("  -d                        run %s as a daemon\n", PROGRAM_NAME);
    printf("  -h                        display this help and exit\n");
    printf("  -p PORT                   specify which port to listen to, default: %d\n", DEFAULT_PORT);
    printf("  -s SCRIPT                 the lua script to use for interpretation, default: \"%s\"\n", DEFAULT_LUA_SCRIPT_FILE);
    printf("  -v                        display version information and exit\n");
}

void print_usage() {
    printf("Usage: %s [-dhsv] [-p PORT]\n", PROGRAM_NAME);
    printf("Try '%s --help' for more information.\n", PROGRAM_NAME);
}

void print_version() {
    printf("%s, version %s, released %s\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_DATE);
    printf("   compiled %s, %s\n\n", __DATE__, __TIME__);
    printf("%s\n", PROGRAM_AUTHORS);
}

void cleanup() {
    lua_close(L);
    DEBUG("lua script file closed\n");

    close_socket(sockfd);

    DEBUG("%s terminated\n", PROGRAM_NAME);
}

void close_socket(int sockfd) {
    DEBUG("closing socket: %d\n", sockfd);
    shutdown(sockfd, SOCKET_SHUTDOWN_ALL);
    DEBUG("connection shutdown\n");
    close(sockfd);
    DEBUG("connection closed\n");
}

void load_lua_script(const char *script) {
    if (script) {
        (void)luaL_dofile(L, script);
        DEBUG("lua script file \"%s\" loaded\n", script);
    }
    else {
        (void)luaL_dofile(L, DEFAULT_LUA_SCRIPT_FILE);
        DEBUG("lua script file \"%s\" loaded\n", DEFAULT_LUA_SCRIPT_FILE);
    }
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

    // Should be safe to cut pipes now...
    if (is_daemon && !daemonize(PID_FILE)) {
        printf("Error: could not daemonize\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
        const char *ip = inet_ntoa(client_addr.sin_addr);
        const long timestamp = time(NULL);
        connection_established(timestamp, ip, port);
        DEBUG("incoming connection from %s:%d @ time: %ld\n", ip, port, timestamp);

        const char *message = welcome_message(time(NULL), ip, port);
        if (message) {
            send(clientfd, message, strlen(message), 0);
        }

        if (reject_connection(time(NULL), ip, port)) {
            close_socket(clientfd);
            connection_closed(time(NULL), ip, port, (time(NULL) - timestamp));
        }
        else {
            if (fork() == 0) {
                sockfd = clientfd; // to handle this connection in case of int
                char temp[BUFFER_SIZE];
                do {
                    memset(temp, '\0', BUFFER_SIZE);
                    if (recv(clientfd, &temp, BUFFER_SIZE, 0) <= 0) {
                        close_socket(clientfd);
                        connection_closed(time(NULL), ip, port, (time(NULL) - timestamp));
                        exit(EXIT_SUCCESS);
                    }
                    DEBUG("data received: \"%s\"\n", temp);
                    const char *reply = data_received(time(NULL), ip, port, (const char *)&temp);
                    if (reply) {
                        DEBUG("sending reply: \"%s\"\n", reply);
                        send(clientfd, reply, strlen(reply), 0);
                    }
                } while (wait_for_more_data(time(NULL), ip, port, (const char *)&temp));
                close_socket(clientfd);
                connection_closed(time(NULL), ip, port, (time(NULL) - timestamp));
                exit(EXIT_SUCCESS);
            }
            DEBUG("connection forked and left to its destiny\n");
        }
    }
}

void connection_established(const long timestamp, const char *ip, const int port) {
    DEBUG("calling lua function %s()\n", LUA_FUN_ESTABLISHED);
    lua_getglobal(L, LUA_FUN_ESTABLISHED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_call(L, 3, 0);
}

void connection_closed(const long timestamp, const char *ip, const int port, const long duration) {
    DEBUG("calling lua function %s()\n", LUA_FUN_CLOSED);
    lua_getglobal(L, LUA_FUN_CLOSED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_pushnumber(L, duration);
    lua_call(L, 4, 0);
}

int reject_connection(const long timestamp, const char *ip, const int port) {
    DEBUG("calling lua function %s()\n", LUA_FUN_REJECT_CONNECTION);
    lua_getglobal(L, LUA_FUN_REJECT_CONNECTION);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_call(L, 3, 1);
    int reject = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return reject;
}

const char *welcome_message(const long timestamp, const char *ip, const int port) {
    DEBUG("calling lua function %s()\n", LUA_FUN_WELCOME_MESSAGE);
    lua_getglobal(L, LUA_FUN_WELCOME_MESSAGE);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_call(L, 3, 1);
    const char *message = lua_tostring(L, -1);
    lua_pop(L, 1);
    return message;
}

int wait_for_more_data(const long timestamp, const char *ip, const int port, const char *last_incoming) {
    DEBUG("calling lua function %s()\n", LUA_FUN_WAIT_FOR_MORE_DATA);
    lua_getglobal(L, LUA_FUN_WAIT_FOR_MORE_DATA);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_pushstring(L, last_incoming);
    lua_call(L, 4, 1);
    int wait = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return wait;
}

const char *data_received(const long timestamp, const char *ip, const int port, const char *data) {
    DEBUG("calling lua function %s()\n", LUA_FUN_DATA_RECEIVED);
    lua_getglobal(L, LUA_FUN_DATA_RECEIVED);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    lua_pushstring(L, data);
    lua_call(L, 4, 1);
    const char *message = lua_tostring(L, -1);
    lua_pop(L, 1);
    return message;
}

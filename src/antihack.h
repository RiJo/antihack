#ifndef _ANTIHACK_H_
#define _ANTIHACK_H_ 1

#define PROGRAM_NAME                "antihack"
#define PROGRAM_VERSION             "1.8.0"
#define PROGRAM_DATE                "2010-04-01"
#define PROGRAM_AUTHORS             "Rikard Johansson, 2010"

#define PID_FILE                    "/var/run/" PROGRAM_NAME ".pid"

#define DEFAULT_PORT                22
#define DEFAULT_LUA_SCRIPT_FILE     "script.lua"

#define BUFFER_SIZE                 1024

#define SOCKET_SHUTDOWN_RECV        0
#define SOCKET_SHUTDOWN_SEND        1
#define SOCKET_SHUTDOWN_ALL         2

#define LUA_FUN_ESTABLISHED         "connection_established"
#define LUA_FUN_CLOSED              "connection_closed"
#define LUA_FUN_REJECT_CONNECTION   "reject_connection"
#define LUA_FUN_WELCOME_MESSAGE     "welcome_message"
#define LUA_FUN_WAIT_FOR_MORE_DATA  "wait_for_more_data"
#define LUA_FUN_DATA_RECEIVED       "data_received"

#ifdef _DEBUG_
    #define DEBUG printf("[debug] ");printf
#else
    #define DEBUG(arg1,...)
#endif

void signal_handler(int);
void handle_arguments(int, char **);
void close_socket(int);
void load_lua_script(const char *);
void start_server(int);
void print_help();
void print_usage();
void print_version();
void cleanup();

// lua wrappers
void connection_established(const long, const char *, const int);
void connection_closed(const long, const char *, const int, const long);
int reject_connection(const long, const char *, const int);
const char *welcome_message(const long, const char *, const int);
int wait_for_more_data(const long, const char *, const int, const char *);
const char *data_received(const long, const char *, const int, const char *);

#endif

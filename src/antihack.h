#ifndef _ANTIHACK_H_
#define _ANTIHACK_H_ 1

#define PROGRAM_NAME                "antihack"
#define PROGRAM_VERSION             "1.5.0"
#define PROGRAM_DATE                "2010-03-26"
#define PROGRAM_AUTHORS             "Rikard Johansson, 2010"

#define PID_FILE                    "/var/run/" PROGRAM_NAME ".pid"

#define DEFAULT_PORT                22
#define DEFAULT_LUA_SCRIPT_FILE     "script.lua"

#define BUFFER_SIZE                 1024

#define LUA_FUN_ESTABLISHED         "connection_established"
#define LUA_FUN_CLOSED              "connection_closed"
#define LUA_FUN_SEND_REPLY          "send_reply"
#define LUA_FUN_CLOSE_CONNECTION    "close_connection"
#define LUA_FUN_REPLY_MESSAGE       "reply_message"
#define LUA_FUN_DATA_RECEIVED       "data_received"

#ifdef _DEBUG_
    #define DEBUG printf("[debug] ");printf
#else
    #define DEBUG(arg1,...)
#endif

void signal_handler(int);
void start_server(int, int);
void print_help();
void print_usage();
void print_version();
void cleanup();

// lua wrappers
void connection_established(const long, const char *, const int);
void connection_closed(const long, const char *, const int, const long);
int send_reply(const long, const char *, const int);
int close_connection(const long, const char *, const int);
const char *reply_message(const long, const char *, const int);
const char *data_received(const long, const char *, const int, const char *);

#endif

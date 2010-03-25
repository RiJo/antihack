#ifndef _ANTIHACK_H_
#define _ANTIHACK_H_ 1

#define PROGRAM_NAME "antihack"
#define PROGRAM_VERSION "1.1.0"
#define PROGRAM_DATE "2010-03-25"

#define PID_FILE "/var/run/" PROGRAM_NAME ".pid"

#define DEFAULT_PORT 22
#define SOCKET_BUFFER_SIZE 1024

#define LUA_FUN_INCOMING "incoming_connection"
#define LUA_FUN_SEND_REPLY "send_reply"
#define LUA_FUN_CLOSE_CONNECTION "close_connection"
#define LUA_FUN_REPLY_MESSAGE "reply_message"

#ifdef _DEBUG_
#define DEBUG printf("[debug] ");printf
#else
#define DEBUG(arg1,...)
#endif

void signal_handler(int);

void incoming(long, char *, int);
int send_reply();
int close_connection();
const char *reply_message();

void start_server(int);
void cleanup();

#endif

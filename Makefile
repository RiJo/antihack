CC       = gcc
CCFLAGS  = -O2 -march=core2 -Wall
DEBUG    = -g -D=_DEBUG_
INCLUDES = -I/usr/include
LDPATH   = -L/usr/lib64
LIBS     = -llua -lm

HEADERS  = src/antihack.h   \
           src/daemon.h

OBJS     = antihack.o       \
           daemon.o

NAME     = antihack

all: $(NAME)

debug: CCFLAGS += $(DEBUG)
debug: $(NAME)

clean:
	rm -f $(NAME) $(OBJS)

$(NAME): Makefile $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDPATH) $(LIBS)

$(OBJS): %.o: ./src/%.c $(HEADERS)
	$(CC) $(CFLAGS) $(CCFLAGS) $(INCLUDES) -c $< -o $@

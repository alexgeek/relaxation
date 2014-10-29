CC=gcc
CFLAGS= -Wall -g -std=c99
LIBS=-lpthread -lm
MAIN=relax
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: $(MAIN)
	@echo "Compiled $(MAIN)"
 
$(MAIN) : $(OBJS)
	${CC} ${CFLAGS} -o $(MAIN) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
 
clean:
	${RM} *.o *.~ $(MAIN)

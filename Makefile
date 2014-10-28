CC=gcc
CFLAGS= -Wall -g
LIBS=-lpthread
MAIN=relax
SRCS = *.c
OBJS = $(SRCS:.c=.o)

all: $(MAIN)
	@echo "Compiled $(MAIN)"
 
$(MAIN) : $(OBJS)
	${CC} ${CFLAGS} -o $(MAIN) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@
 
clean:
	${RM} *.o *.~ $(MAIN)

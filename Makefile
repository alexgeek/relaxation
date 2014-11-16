CC=gcc
CFLAGS= -Wall -g
LIBS=-lpthread -lm
MAIN=relax
SRCS=grid.c thread.c bmpfile.c relax.c
OBJS=$(SRCS:.c=.o)

all: $(MAIN)
	@echo "Compiled $(MAIN)"

$(MAIN) : $(OBJS)
	${CC} ${CFLAGS} -o $(MAIN) $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	${RM} *.o *.~ $(MAIN) array.bmp

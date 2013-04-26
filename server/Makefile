CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -g
LDFLAGS = -lpthread -lrt
TARGETS = httpdnsd

all: $(TARGETS)

httpdnsd: http.o httpdnsd.o httpserver.o socket.o string.o thread.o url.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	-$(RM) $(TARGETS)
	-$(RM) *.o
	-$(RM) *~

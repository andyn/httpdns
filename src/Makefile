CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -g
LDFLAGS = -lpthread
TARGETS = httpdnsd

all: $(TARGETS)

httpdnsd: http.o httpdnsd.o httpserver.o socket.o string.o thread.o url.o

clean:
	-$(RM) $(TARGETS)
	-$(RM) *.o
	-$(RM) *~

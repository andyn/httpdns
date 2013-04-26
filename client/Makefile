CCC = g++
CCFLAGS = -std=c++98 -pedantic -Wall -Wextra -I./include -g

all: httpdnsc

httpdnsc: httpdnsc.oo filedescriptor.oo url.oo
	$(CCC) $(CCFLAGS) -o $@ $^

%.oo : %.cc
	$(CCC) $(CCFLAGS) -o $@ -c $<

clean:
	# Removing temporary source code files
	-rm -f *~
	# Removing temporary object code files
	-rm -f *.oo
	-rm -f *.o
	# Removing executables
	-rm httpdnsc
	
.PHONY: all clean
	

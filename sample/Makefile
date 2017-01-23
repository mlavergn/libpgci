CXX=g++
CFLAGS=-I/usr/include -I/usr/include/pgsql -I/usr/local/lib -Qunused-arguments -Wno-write-strings
LDFLAGS=-L/usr/local/lib -lpq -lpgci

OSNAME := $(shell uname -s)
ifeq ($(OSNAME), Darwin)
CFLAGS:=$(CFLAGS) -I/Applications/Postgres.app/Contents/Versions/9.6/include
LDFLAGS:=$(LDFLAGS) -L/Applications/Postgres.app/Contents/Versions/9.6/lib
endif

all: sample

sample: sample.o
	$(CXX) $(LDFLAGS) sample.o -o $@

sample.o: sample1.cpp
	$(CXX) $(CFLAGS) -o $@ -c sample1.cpp

clean:
	@echo "Cleaning up source directory"
	rm -f sample *.o  *~ *.*~
